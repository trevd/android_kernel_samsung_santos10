/* drivers/staging/mrst/drv/cmc624.c
*
 * Copyright (C) 2013 Samsung Electronics Co, Ltd.
*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/uaccess.h>
#include <video/cmc624.h>
#include <linux/backlight.h>
#include <psb_drv.h>
/*
 * DEFINE
 */
#define CMC624_MAX_SETTINGS	 100
#define SUB_TUNE	0
#define MAIN_TUNE	1
#define BROW_TUNE	2

#define BROWSER_COLOR_TONE_MIN	40
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/tuning/"

#define MAX_BRIGHTNESS_LEVEL	255
#define DEF_BRIGHTNESS_LEVEL	134
#define MIN_BRIGHTNESS_LEVEL	10

#define IS_SCENARIO_VALID(_val) \
	((_val) < MAX_mDNIe_MODE && (_val) >= 0)

#define NUM_SCR_BUF	9
#define ADDR_SCR_RRCR	0x0071
#define ADDR_SCR_BBYB	0x0079
#define IS_ADDR_SCR(_addr) \
	((_addr) >= ADDR_SCR_RRCR && (_addr) <= ADDR_SCR_BBYB)

/*
 * STRUCTURE
 */
struct cmc624_state_type {
	enum tune_menu_cabc cabc_mode;
	enum tune_menu_cabc tmp_cabc;
	unsigned int suspended;
	enum tune_menu_app scenario;
	enum tune_menu_mode background;
	enum tune_menu_bypass bypass;

	/*This value must reset to 0 (standard value) when change scenario*/
	enum tune_menu_negative negative;
	enum tune_menu_accessibility accessibility;

	enum tune_menu_siop siop_mode;
};

struct cmc624_info {
	struct device *dev;
	struct device *lcd_dev;
	struct i2c_client *client;
	struct cmc624_panel_data *pdata;
	struct cmc624_register_set cmc624_tune_seq[CMC624_MAX_SETTINGS];
	struct class *mdnie_class;
	struct class *lcd_class;
	struct cmc624_state_type state;
	struct mutex power_lock;
	struct mutex tune_lock;

	int init_tune_flag[3];
	unsigned long last_cmc624_Bank;
	unsigned long last_cmc624_Algorithm;
	char tuning_filename[MAX_FILE_NAME];

	enum power_lut_level pwrlut_lev;
	u16 force_pwm_cnt;
};
static struct cmc624_info *g_cmc624_info;

static LIST_HEAD(tune_data_list);
static DEFINE_MUTEX(tune_data_list_mutex);

static int cmc624_tune_restore(struct cmc624_info *info);

/*
 * FUNCTIONS
 */

static int cmc624_reg_write(struct cmc624_info *info, u8 reg, u16 value)
{
	int ret;
	struct i2c_client *client;

	if (!info->client) {
		dev_err(info->dev, "%s: client is NULL\n", __func__);
		return -ENODEV;
	}

	client = info->client;

	if (info->state.suspended) {
		dev_err(info->dev, "skip to set cmc624 if LCD is off(%x,%x)\n",
								reg, value);
		return 0;
	}

	if (reg == 0x0000) {
		if (value == info->last_cmc624_Bank)
			return 0;
		info->last_cmc624_Bank = value;
	} else if (reg == 0x0001)
		info->last_cmc624_Algorithm = value;

	value = swab16(value);
	ret = i2c_smbus_write_word_data(client, reg, value);
	if (ret < 0) {
		dev_err(info->dev, "%s: err(%d), reg=0x%x,value=0x%x\n",
				__func__, ret, reg, value);
		return ret;
	}

	dev_dbg(info->dev, "%s: reg:0x%x, val:0x%x\n", __func__, reg, value);

	return 0;
}

static u16 cmc624_reg_read(struct cmc624_info *info, u8 reg)
{
	struct i2c_client *client;
	int value;

	if (!info->client) {
		dev_err(info->dev, "%s: client is NULL\n", __func__);
		return -ENODEV;
	}

	client = info->client;

	if (reg == 0x01) {
		value = info->last_cmc624_Algorithm;
		return 0;
	}

	value = i2c_smbus_read_word_data(client, reg);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	value = swab16(value);

	dev_dbg(info->dev, "%s: reg:0x%x, val:0x%x\n", __func__, reg, value);

	return value;
}

static struct tune_data *cmc624_get_tune_data(u32 id, u32 mask)
{
	struct tune_data *data;
	u32 target_id;

	target_id = id & mask;

	list_for_each_entry(data, &tune_data_list, list) {
		if ((data->id & mask) == target_id)
			goto found;
	}

	pr_err("%s: fail to find tune data, id=0x%x, mask=0x%x, target=0x%x\n",
						__func__, id, mask, target_id);
	return NULL;

found:
	pr_info("%s: data = %pF\n", __func__, data->value);
	return data;
}

int cmc624_register_tune_data(u32 id, struct cmc624_register_set *cmc_set,
								u32 tune_size)
{
	struct tune_data *data;

	list_for_each_entry(data, &tune_data_list, list) {
		if (data->id == id)
			goto dupulicate;
	}

	data = kzalloc(sizeof(struct tune_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->id = id;
	data->value = cmc_set;
	data->size = tune_size;

	list_add(&data->list, &tune_data_list);

	return 0;

dupulicate:
	pr_warning("%s: dupulication(%pF) ,id=%x\n", __func__, data->value, id);
	return -EINVAL;
}
EXPORT_SYMBOL(cmc624_register_tune_data);

static int cmc624_send_cmd(struct cmc624_register_set *value,
						 unsigned int array_size)
{
	struct cmc624_info *info = g_cmc624_info;
	int i;
	int ret = 0;

	if (!value) {
		dev_err(info->dev, "%s: tune data is NULL!\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&info->tune_lock);

	for (i = 0; i < array_size; i++) {
		if (value[i].reg_addr == SLEEPMSEC) {
			usleep_range(value[i].data*1000,
						value[i].data*1000+100);
			continue;
		}

		ret = cmc624_reg_write(info, value[i].reg_addr, value[i].data);

		if (ret < 0) {
			dev_err(info->dev, "%s: failed to send cmd(%d)\n",
								__func__, ret);
			goto set_error;
		}

	}

set_error:
	mutex_unlock(&info->tune_lock);
	return ret;
}

void cmc624_set_auto_brightness(enum auto_brt_val auto_brt)
{
	struct cmc624_info *info = g_cmc624_info;
	enum tune_menu_cabc new_cabc;

	/* select cmc624 power lut table to control pwm with  cabc-on mode */
	if (auto_brt < AUTO_BRIGHTNESS_VAL3)
		info->pwrlut_lev = PWRLUT_LEV_INDOOR;
	else if (auto_brt < AUTO_BRIGHTNESS_VAL5)
		info->pwrlut_lev = PWRLUT_LEV_OUTDOOR1;
	else
		info->pwrlut_lev = PWRLUT_LEV_OUTDOOR2;

	/* if auto brightness value is 0, turn off cabc */
	if (auto_brt)
		new_cabc = MENU_CABC_ON;
	else
		new_cabc = MENU_CABC_OFF;

	/* Skip to set CABC-ON with "CAMERA" scenario */
	if (info->state.scenario == MENU_APP_CAMERA) {
		info->state.tmp_cabc = new_cabc;
		dev_info(info->dev, "%s: skip to set cabc(=%d) with (CAMERA)\n",
				__func__, new_cabc);
		return;
	}

	if (new_cabc == info->state.cabc_mode) {
		dev_dbg(info->dev, "%s: skip to set cabc mode\n", __func__);
		return;
	}

	info->state.cabc_mode = new_cabc;

	if (info->state.suspended == true) {
		dev_info(info->dev, "%s: skip to set cabc mode\n", __func__);
		return;
	}

	cmc624_tune_restore(info);
}
EXPORT_SYMBOL(cmc624_set_auto_brightness);

/*
 * CMC624 PWM control
 */

static struct cmc624_register_set pwm_cabcoff[] = {
	{0x00, 0x0001}, /* BANK 1 */
	{0xF8, 0x0011}, /* PWM HIGH ACTIVE, USE REGISTER VALUE */
	{0xF9, },	/* PWM Counter */

	{0x00, 0x0000}, /* BANK 0 */
	{0xFD, 0xFFFF}, /* MODULE REG MASK RELEASE */
	{0xFE, 0xFFFF}, /* MODULE REG MASK RELEASE */
	{0xFF, 0x0000}, /* MASK RELEASE */
};

static struct cmc624_register_set pwm_cabcon[] = {
	{0x00, 0x0001}, /* BANK 1 */
	{0xF8, 0x0010}, /* PWM HIGH ACTIVE, USE CABC POWER RATE VALUE */
	{0xBB, },	/* POWERLUT_0 */
	{0xBC, },	/* POWERLUT_1 */
	{0xBD, },	/* POWERLUT_2 */
	{0xBE, },	/* POWERLUT_3 */
	{0xBF, },	/* POWERLUT_4 */
	{0xC0, },	/* POWERLUT_5 */
	{0xC1, },	/* POWERLUT_6 */
	{0xC2, },	/* POWERLUT_7 */

	{0x00, 0x0000}, /* BANK 0 */
	{0xFD, 0xFFFF}, /* MODULE REG MASK RELEASE */
	{0xFE, 0xFFFF}, /* MODULE REG MASK RELEASE */
	{0xFF, 0x0000}, /* MASK RELEASE */
};

static u16 cmc624_get_pwm_val(int intensity, u16 max, u16 def, u16 min)
{
	u16 val;

	if (intensity > DEF_BRIGHTNESS_LEVEL)
		val = (intensity - DEF_BRIGHTNESS_LEVEL) * (max - def) /
			(MAX_BRIGHTNESS_LEVEL - DEF_BRIGHTNESS_LEVEL) + def;
	else if (intensity > MIN_BRIGHTNESS_LEVEL)
		val = (intensity - MIN_BRIGHTNESS_LEVEL) * (def - min) /
			(DEF_BRIGHTNESS_LEVEL - MIN_BRIGHTNESS_LEVEL) + min;
	else
		val = intensity * min / MIN_BRIGHTNESS_LEVEL;

	pr_debug("%s: val=%d, intensity=%d, max=%d, def=%d, min=%d\n",
				__func__, val, intensity, max, def, min);

	return val;
}
static int _cmc624_set_pwm(struct cmc624_info *info, int intensity)
{
	enum power_lut_mode mode;
	enum power_lut_level level;
	struct cabcoff_pwm_cnt_tbl *pwm_tbl = info->pdata->pwm_tbl;
	struct cabcon_pwr_lut_tbl *pwr_luts = info->pdata->pwr_luts;
	enum tune_menu_cabc cabc;
	int i;
	int ret;

	if (info->state.suspended == true) {
		pr_info("%s: skip to set brightness\n", __func__);
		return -1;
	}

	if (info->force_pwm_cnt) {
		pwm_cabcoff[2].data = info->force_pwm_cnt;
		pr_info("%s: force to set CMC624 pwm cnt = %d\n",
				__func__, info->force_pwm_cnt);
		cmc624_send_cmd(pwm_cabcoff, ARRAY_SIZE(pwm_cabcoff));
		return 0;
	}

	if (info->state.siop_mode == MENU_SIOP_ON &&
			info->state.scenario != MENU_APP_CAMERA)
		cabc = MENU_CABC_ON;
	else
		cabc = info->state.cabc_mode;

	if (cabc == MENU_CABC_ON) {
		if (info->state.scenario >= MENU_APP_VIDEO &&
			info->state.scenario <= MENU_APP_VIDEO_COLD)
			mode = PWRLUT_MODE_VIDEO;
		else
			mode = PWRLUT_MODE_UI;
		level = info->pwrlut_lev;

		pr_info("%s: cabc on: intensity=%d\n", __func__, intensity);
		/* set power lut value for cabc-on pwm */
		for (i = 0; i < NUM_PWRLUT_REG; i++) {
			pwm_cabcon[i + 2].data = cmc624_get_pwm_val(intensity,
						pwr_luts->max[level][mode][i],
						pwr_luts->def[level][mode][i],
						pwr_luts->min[level][mode][i]);
			pr_debug("pwr lut=%d\n", pwm_cabcon[i + 2].data);
		}

		ret = cmc624_send_cmd(pwm_cabcon, ARRAY_SIZE(pwm_cabcon));
	} else {
		/* set pwm counter value for cabc-off pwm */
		pwm_cabcoff[2].data = cmc624_get_pwm_val(intensity,
				pwm_tbl->max, pwm_tbl->def, pwm_tbl->min);
		pr_info("%s: cabc off: intensity=%d,  pwm cnt=%d\n",
				__func__, intensity, pwm_cabcoff[2].data);

		ret = cmc624_send_cmd(pwm_cabcoff, ARRAY_SIZE(pwm_cabcoff));
	}

	if (unlikely(ret < 0)) {
		pr_err("%s: failed to set brightness(%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

int cmc624_set_pwm(int intensity)
{
	struct cmc624_info *info;
	if (!g_cmc624_info)
		return -1;
	else
		info = g_cmc624_info;
	return _cmc624_set_pwm(info, intensity);
}
EXPORT_SYMBOL(cmc624_set_pwm);

static int cmc624_set_tune_value(u32 id, u32 mask)
{
	struct tune_data *data;
	int ret;

	/* get tune_data */
	data = cmc624_get_tune_data(id, mask);
	if (!data) {
		pr_err("%s: no cmc624 data, id=0x%x, mask=0x%x\n",
							__func__, id, mask);
		return -EINVAL;
	}

	/* set tune_value */
	ret = cmc624_send_cmd(data->value, data->size);
	if (ret < 0)
		return ret;

	pr_info("set tune data (%pF)\n", data->value);
	return 0;
}

static int apply_main_tune_value(struct cmc624_info *info,
				enum tune_menu_mode bg, enum tune_menu_app ui,
				int force)
{
	enum tune_menu_cabc cabc;
	u32 id;
	int ret;

	if (info->state.siop_mode == MENU_SIOP_ON &&
			info->state.scenario != MENU_APP_CAMERA)
		cabc = MENU_CABC_ON;
	else
		cabc = info->state.cabc_mode;


	if ((!force) && (info->state.scenario == ui) &&
			(info->state.background == bg)) {
		dev_info(info->dev, "%s: dupulication setting(%d, %d)\n",
							__func__, ui, bg);
		return 0;
	}

	if (info->state.negative || info->state.accessibility) {
		dev_info(info->dev, "skip tuning: negative=%d, accessibility=%d",
			info->state.negative, info->state.accessibility);
		goto skip_main_tune;
	}

	TUNE_DATA_ID(id, MENU_CMD_TUNE, bg, ui, MENU_SKIP, MENU_SKIP,
		cabc, MENU_SKIP, MENU_SKIP);
	ret = cmc624_set_tune_value(id, BITMASK_MAIN_TUNE);
	if (ret < 0) {
		dev_err(info->dev, "%s: failed to set main tune\n", __func__);
		return ret;
	}

skip_main_tune:
	info->state.background = bg;
	info->state.scenario = ui;

	return 0;
}

static int apply_tune_value(struct cmc624_info *info,
		enum tune_menu_mode bg, enum tune_menu_app ui, int force)
{
	int app;
	int ret;

	pr_info("%s: bg=%d, ui=%d, force=%d\n", __func__, bg, ui, force);

	/* set main tune */
	ret = apply_main_tune_value(info, bg, ui, force);
	if (ret < 0) {
		dev_err(info->dev, "%s: failed to set main tune\n", __func__);
		return ret;
	}

	return 0;
}

static int apply_negative_tune_value(struct cmc624_info *info)
{
	u32 id;
	int ret;
	enum tune_menu_cabc cabc;

	if (info->state.siop_mode == MENU_SIOP_ON &&
			info->state.scenario != MENU_APP_CAMERA)
		cabc = MENU_CABC_ON;
	else
		cabc = info->state.cabc_mode;

	pr_info("%s: set negative mode\n", __func__);

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_SKIP, MENU_NEGATIVE_ON, cabc, MENU_SKIP,
		MENU_SKIP);
	ret = cmc624_set_tune_value(id,
		BITMASK_CMD | BITMASK_NEGATIVE | BITMASK_CABC);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to set negative tune\n", __func__);
		return ret;
	}

	return 0;
}

static int apply_bypass_tune_value(struct cmc624_info *info,
						enum tune_menu_bypass bypass)
{
	u32 id;
	int ret;

	pr_info("%s: bypass = %d\n", __func__, bypass);

	info->state.bypass = bypass;

	if (bypass) {
		TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
			MENU_BYPASS_ON, MENU_SKIP, MENU_SKIP, MENU_SKIP,
			MENU_SKIP);
		ret = cmc624_set_tune_value(id, BITMASK_CMD | BITMASK_BYPASS);
		if (ret < 0) {
			dev_err(info->dev, "%s:failed to set bypass tune\n",
								__func__);
			info->state.bypass = !bypass;
			return ret;
		}
	} else {
		/* release bypass mode */
		ret = apply_tune_value(info, info->state.background,
						info->state.scenario, 1);
		if (ret < 0) {
			dev_err(info->dev, "%s: failed to restore tune\n",
								__func__);
			info->state.bypass = !bypass;
			return ret;
		}
	}


	return 0;
}

static int cmc624_parse_text(struct cmc624_info *info, char *src, int len)
{
	int i, count, ret;
	int index = 0;
	char *str_line[CMC624_MAX_SETTINGS];
	char *sstart;
	char *c;
	unsigned int data1;
	unsigned int data2;

	c = src;
	count = 0;
	sstart = c;

	for (i = 0; i < len; i++, c++) {
		if (*c == '\r' || *c == '\n') {
			if (c > sstart) {
				str_line[count] = sstart;
				count++;
			}
			*c = '\0';
			sstart = c + 1;
		}
	}

	if (c > sstart) {
		str_line[count] = sstart;
		count++;
	}

	for (i = 0; i < count; i++) {
		ret = sscanf(str_line[i], "0x%x,0x%x\n", &data1, &data2);
		dev_dbg(info->dev, "%s: Result => [0x%2x 0x%4x] %s\n",
					__func__, data1, data2,
					(ret == 2) ? "Ok" : "Not available");
		if (ret == 2) {
			info->cmc624_tune_seq[index].reg_addr = data1;
			info->cmc624_tune_seq[index++].data = data2;
		}
	}
	return index;
}

static int cmc624_load_tuning_data(struct cmc624_info *info)
{
	char *filename = info->tuning_filename;
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	int cmc624_tune_seq_len = 0;
	mm_segment_t fs;

	dev_dbg(info->dev, "%s: called loading file name : %s\n",
							__func__, filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		dev_err(info->dev, "%s: failed to open file(%s)\n",
							__func__, filename);
		ret = -ENODEV;
		goto err_file_open;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	dev_dbg(info->dev, "%s: File Size : %ld(bytes)", __func__, l);

	dp = kzalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		dev_err(info->dev, "%s: failed memory allocation\n", __func__);
		ret = -ENOMEM;
		goto err_kzalloc;
	}
	pos = 0;

	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		dev_err(info->dev, "%s: failed to read vfs(%d)\n",
							__func__, ret);
		ret = -ENODEV;
		goto err_vfs_read;
	}

	cmc624_tune_seq_len = cmc624_parse_text(info, dp, l);

	if (!cmc624_tune_seq_len) {
		dev_err(info->dev, "%s: failed to parse\n", __func__);
		kfree(dp);
		return -ENODEV;

	}

	dev_dbg(info->dev, "%s: Loading Tuning Value's Count = %d\n",
						__func__, cmc624_tune_seq_len);

	/* set tune_value */
	ret = cmc624_send_cmd(info->cmc624_tune_seq, cmc624_tune_seq_len);

	ret = cmc624_tune_seq_len;

err_vfs_read:
	kfree(dp);
err_kzalloc:
	filp_close(filp, current->files);
err_file_open:
	set_fs(fs);
	return ret;
}

/**************************************************************************
 * SYSFS Start
 ***************************************************************************/
static ssize_t scenario_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);

	dev_dbg(dev, "%s called\n", __func__);
	return sprintf(buf, "Current Scenario Mode : %d\n",
		info->state.scenario);
}

static ssize_t scenario_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	struct backlight_device *psb_backlight_device =
					psb_get_backlight_device();
	int ret;
	int value;
	int browser_mode;
	int prev_scenario;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	dev_info(dev, "%s: scenario = %d\n", __func__, value);

	if (!IS_SCENARIO_VALID(value)) {
		dev_err(dev, "%s: wrong scenario mode value : %d\n",
							__func__, value);
		return size;
	}

	if (info->state.suspended == true) {
		info->state.scenario = value;
		return size;
	}

	prev_scenario = info->state.scenario;

	if (value == MENU_APP_CAMERA &&
		info->state.cabc_mode == MENU_CABC_ON) {
		info->state.tmp_cabc = info->state.cabc_mode;
		info->state.cabc_mode = MENU_CABC_OFF;
		dev_info(dev, "set cabc-off mode with CAMERA tuning\n");

		if (unlikely(!psb_backlight_device)) {
			pr_err("%s: ERR: backlight device is NULL\n", __func__);
			return -ENODEV;
		}
		backlight_update_status(psb_backlight_device);
	}

	ret = apply_tune_value(info, info->state.background, value, 0);

	if (ret != 0)
		dev_err(dev, "%s: failed to set tune value, value=%d, ret=%d\n",
							__func__, value, ret);

	if (value != MENU_APP_CAMERA && prev_scenario == MENU_APP_CAMERA) {
		info->state.cabc_mode = info->state.tmp_cabc;
		dev_info(dev, "restore cabc mode(%d) without CAMERA tuning\n",
				info->state.cabc_mode);
		if (unlikely(!psb_backlight_device)) {
			pr_err("%s: ERR: backlight device is NULL\n", __func__);
			return -ENODEV;
		}
		backlight_update_status(psb_backlight_device);
	}

	return size;
}

static DEVICE_ATTR(scenario, S_IRUGO|S_IWUSR|S_IWGRP,
	scenario_show, scenario_store);

static ssize_t negative_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", info->state.negative);
}

static ssize_t negative_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	enum tune_menu_negative prev_mode = info->state.negative;
	int ret;
	int value;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	dev_info(dev, "%s: set negative : %d\n", __func__, value);

	if (value != 0 && value != 1) {
		dev_warn(dev, "%s: invalid negative value(%d)\n",
							__func__, value);
		return size;
	}

	/* Do not skip negative duplicatoin process
	 * because ccessibility menu has also negative mode...
	 */

	info->state.negative = value;
	if (info->state.suspended == true)
		goto skip_set;

	if (value == MENU_NEGATIVE_ON) {
		ret = apply_negative_tune_value(info);
		if (unlikely(ret < 0))
			goto err_set;
	} else {
		/* release negative mode */
		ret = apply_tune_value(info, info->state.background,
						info->state.scenario, 1);
		if (unlikely(ret < 0))
			goto err_set;
	}

skip_set:
	return size;

err_set:
	dev_err(info->dev, "%s: failed to %s negative mode (prev=%d)\n",
			__func__,
			(value == MENU_NEGATIVE_ON) ? "set" : "release",
			prev_mode);
	info->state.negative = prev_mode;
	return ret;
}

static DEVICE_ATTR(negative, S_IRUGO|S_IWUSR|S_IWGRP,
	negative_show, negative_store);

static ssize_t accessibility_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	enum tune_menu_cabc cabc;
	struct tune_data *color_blind_data;
	int i;
	u32 id;
	char *pos = buf;

	if (info->state.siop_mode == MENU_SIOP_ON &&
			info->state.scenario != MENU_APP_CAMERA)
		cabc = MENU_CABC_ON;
	else
		cabc = info->state.cabc_mode;


	pos += sprintf(pos, "%d, ", info->state.accessibility);
	if (info->state.accessibility == MENU_ACC_COLOR_BLIND) {
		TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
				MENU_SKIP, MENU_SKIP, cabc, MENU_SKIP,
				MENU_ACC_COLOR_BLIND);
		color_blind_data = cmc624_get_tune_data(id,
					BITMASK_CMD | BITMASK_ACCESSIBILITY |
					BITMASK_CABC);
		if (!color_blind_data) {
			dev_err(dev, "no color_blind_data, id=0x%x\n", id);
			goto skip_color_blind;
		}

		for (i = 0; i < color_blind_data->size; i++) {
			if (IS_ADDR_SCR(color_blind_data->value[i].reg_addr))
				pos += sprintf(pos, "0x%04x, ",
					color_blind_data->value[i].data);

			dev_info(dev, "color blind data: reg:0x%x, data:0x%x\n",
					color_blind_data->value[i].reg_addr,
					color_blind_data->value[i].data);
		}
	}

skip_color_blind:
	pos += sprintf(pos, "\n");
	return pos - buf;
}

static int store_color_blind_tune_value(struct cmc624_info *info, u32 *scr)
{
	struct tune_data *color_blind_data;
	enum tune_menu_cabc cabc;
	u32 id;
	int i;
	int j;
	int ret;

	if (info->state.siop_mode == MENU_SIOP_ON &&
			info->state.scenario != MENU_APP_CAMERA)
		cabc = MENU_CABC_ON;
	else
		cabc = info->state.cabc_mode;


	/* store color_blind tuning data */
	dev_info(info->dev, "store color blind data\n");
	for (i = 0; i < NUM_SCR_BUF; i++)
		dev_info(info->dev, "scr buf[%d]: 0x%x\n", i, scr[i]);

	for (cabc = MENU_CABC_OFF; cabc < MAX_CABC_MODE; cabc++) {
		TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP, MENU_SKIP,
			MENU_SKIP, cabc, MENU_SKIP, MENU_ACC_COLOR_BLIND);
		color_blind_data = cmc624_get_tune_data(id,
				BITMASK_CMD | BITMASK_ACCESSIBILITY |
				BITMASK_CABC);
		if (!color_blind_data) {
			dev_err(info->dev, "no color_blind_data,id=0x%x\n", id);
			return -EINVAL;
		}

		j = 0;
		for (i = 0; i < color_blind_data->size, j < NUM_SCR_BUF; i++) {
			if (IS_ADDR_SCR(color_blind_data->value[i].reg_addr)) {
				color_blind_data->value[i].data = scr[j++];
				dev_info(info->dev, "%s: reg:0x%x, data:0x%x\n",
					__func__,
					color_blind_data->value[i].reg_addr,
					color_blind_data->value[i].data);
			}
		}
	}

	return 0;
}

static int apply_color_blind_tune_value(struct cmc624_info *info)
{
	enum tune_menu_cabc cabc;
	u32 id;
	int ret;

	if (info->state.siop_mode == MENU_SIOP_ON &&
			info->state.scenario != MENU_APP_CAMERA)
		cabc = MENU_CABC_ON;
	else
		cabc = info->state.cabc_mode;

	/* set color blind mode */
	dev_info(info->dev, "%s: set color blind mode\n", __func__);

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_SKIP, MENU_SKIP, cabc, MENU_SKIP,
		MENU_ACC_COLOR_BLIND);
	ret = cmc624_set_tune_value(id,
		BITMASK_CMD | BITMASK_ACCESSIBILITY | BITMASK_CABC);
	if (ret < 0) {
		dev_err(info->dev, "fail to set color blind\n");
		return ret;
	}

	return 0;

}

static ssize_t accessibility_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	enum tune_menu_accessibility prev_mode = info->state.accessibility;
	int ret;
	u32 value;
	u32 scr[NUM_SCR_BUF];

	memset(scr, 0, NUM_SCR_BUF);
	ret = sscanf(buf, "%d %x %x %x %x %x %x %x %x %x",
			&value, &scr[0], &scr[1], &scr[2], &scr[3],
			&scr[4], &scr[5], &scr[6], &scr[7], &scr[8]);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	dev_info(dev, "%s: set accessibility: %d\n", __func__, value);

	/* We should save accessibility mode in global structure here
	 * because apply_tune_value needs current accessibility setting.
	 */
	info->state.accessibility = value;

	if (info->state.suspended == true)
		return size;

	switch (value) {
	case MENU_ACC_OFF:
		/* try to release accessibility mode */
		ret = apply_tune_value(info, info->state.background,
						info->state.scenario, 1);
		if (unlikely(ret < 0))
			goto err_set;
		break;
	case MENU_ACC_NEGATIVE:
		ret = apply_negative_tune_value(info);
		if (unlikely(ret < 0))
			goto err_set;
		break;
	case MENU_ACC_COLOR_BLIND:
		ret = store_color_blind_tune_value(info, scr);
		if (unlikely(ret < 0))
			goto err_set;
		ret = apply_color_blind_tune_value(info);
		if (unlikely(ret < 0))
			goto err_set;
		break;
	default:
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;

	}

	return size;

err_set:
	info->state.accessibility = prev_mode;
	dev_err(info->dev, "%s: fail to %s accessibility (%d)\n", __func__,
			(value == MENU_ACC_OFF) ? "release" : "set", value);
	return ret;
}

static DEVICE_ATTR(accessibility, S_IRUGO|S_IWUSR|S_IWGRP,
			accessibility_show, accessibility_store);

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	const char *name[MAX_BACKGROUND_MODE] = {
		"DYNAMIC",
		"STANDARD",
		"MOVIE",
		"AUTO",
	};

	return sprintf(buf, "Current Background Mode : %s\n",
						name[info->state.background]);
}

static ssize_t mode_store(struct device *dev,
			  struct device_attribute *attr, const char *buf,
			  size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	int ret;
	int value;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	dev_info(dev, "%s: set background mode : %d\n", __func__,  value);

	if (value < MENU_MODE_DYNAMIC || value >= MAX_BACKGROUND_MODE) {
		dev_err(dev, "%s: invalid backgound mode value : %d\n",
							__func__, value);
		return size;
	}

	if (value == info->state.background) {
		dev_dbg(dev, "%s: duplicate setting: %d\n", __func__, value);
		return size;
	}

	if (info->state.suspended == true) {
		info->state.background = value;
		return size;
	}
	ret = apply_tune_value(info, value, info->state.scenario, 0);
	if (ret != 0)
		dev_err(dev, "%s: failed to set main tune value(%d), ret=%d\n",
				__func__, value, ret);
	return size;
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR | S_IWGRP, mode_show, mode_store);

static ssize_t tuning_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	int ret = 0;
	ret = sprintf(buf, "Tunned File Name : %s\n",
		info->tuning_filename);

	return ret;
}

static ssize_t tuning_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	int ret;

	memset(info->tuning_filename, 0, sizeof(info->tuning_filename));
	sprintf(info->tuning_filename, "%s%s", TUNING_FILE_PATH, buf);
	info->tuning_filename[strlen(info->tuning_filename)-1] = 0;

	dev_dbg(dev, "%s: file name = %s\n", __func__, info->tuning_filename);

	ret = cmc624_load_tuning_data(info);
	if (ret <= 0)
		dev_err(dev, "%s: failed to load tuning data(%d)\n",
								__func__, ret);
		return size;

	return size;
}

static DEVICE_ATTR(tuning, S_IRUGO|S_IWUSR|S_IWGRP, tuning_show, tuning_store);

static ssize_t pwm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	struct cabcoff_pwm_cnt_tbl *pwm_tbl = info->pdata->pwm_tbl;
	struct cabcon_pwr_lut_tbl *pwr_luts = info->pdata->pwr_luts;
	int i;
	int j;
	int k;

	dev_info(dev, "%s: pwm counter: max=0x%04x, def=0x%04x, min=0x%04x\n",
			__func__, pwm_tbl->max, pwm_tbl->def, pwm_tbl->min);

	for (i = 0; i < MAX_PWRLUT_LEV; i++) {
		for (j = 0; j < MAX_PWRLUT_MODE; j++) {
			for (k = 0; k < NUM_PWRLUT_REG; k++) {
				dev_info(dev, "max=%04x, def=%04x, min=%04x\n",
						pwr_luts->max[i][j][k],
						pwr_luts->def[i][j][k],
						pwr_luts->min[i][j][k]);
			}
		}
	}

	return sprintf(buf, "max=%d,def=%d,min=%d",
			pwm_tbl->max, pwm_tbl->def, pwm_tbl->min);
}

static DEVICE_ATTR(pwm, S_IRUGO, pwm_show, NULL);

static ssize_t cabc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s: cabc mode = %d\n", __func__, info->state.cabc_mode);

	return sprintf(buf, "%d\n", info->state.cabc_mode);
}

static ssize_t cabc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	struct backlight_device *psb_backlight_device =
					psb_get_backlight_device();
	int value;
	enum tune_menu_cabc new_cabc;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	if (value)
		new_cabc = MENU_CABC_ON;
	else
		new_cabc = MENU_CABC_OFF;

	dev_info(dev, "%s: cabc mode = %d -> %d\n",
				__func__, info->state.cabc_mode, new_cabc);

	/* Skip to set CABC-ON with "CAMERA" scenario */
	if (info->state.scenario == MENU_APP_CAMERA) {
		info->state.tmp_cabc = new_cabc;
		dev_info(info->dev, "%s: skip to set cabc(=%d) with (CAMERA)\n",
				__func__, new_cabc);
		goto skip;
	}

	if (new_cabc == info->state.cabc_mode) {
		dev_dbg(info->dev, "%s: skip to set cabc mode\n", __func__);
		goto skip;
	}

	info->state.cabc_mode = new_cabc;

	if (info->state.suspended == true) {
		dev_info(info->dev, "%s: skip to set cabc mode\n", __func__);
		goto skip;
	}

	cmc624_tune_restore(info);

	if (unlikely(!psb_backlight_device)) {
		pr_err("%s: ERR: backlight device is NULL\n", __func__);
		goto skip;
	}
	backlight_update_status(psb_backlight_device);


skip:
	return size;
}

static DEVICE_ATTR(cabc, S_IRUGO|S_IWUSR|S_IWGRP, cabc_show, cabc_store);

static ssize_t bypass_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", info->state.bypass);
}

static ssize_t bypass_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	struct backlight_device *psb_backlight_device =
					psb_get_backlight_device();
	int value;
	enum tune_menu_bypass new_bypass;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value);

	if (value >= MAX_BYPASS_MODE)
		new_bypass = MENU_BYPASS_OFF;
	else
		new_bypass = (value) ? MENU_BYPASS_ON : MENU_BYPASS_OFF;

	info->state.bypass = new_bypass;

	cmc624_tune_restore(info);

	if (unlikely(!psb_backlight_device)) {
		pr_err("%s: ERR: backlight device is NULL\n", __func__);
		return -ENODEV;
	}
	backlight_update_status(psb_backlight_device);


	return count;
}

static DEVICE_ATTR(bypass, S_IRUGO|S_IWUSR|S_IWGRP, bypass_show, bypass_store);

static ssize_t reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	int bank;
	int reg;
	u16 val;

	pr_info("CMC624 Register Value\n");
	for (bank = 0; bank < 6; bank++) {
		pr_info("BANK %d\n", bank);
		cmc624_reg_write(info, 0x00, bank);

		for (reg = 0; reg < 0x100; reg++) {
			val = cmc624_reg_read(info, reg);
			pr_info("REG: 0x%02x, VAL: 0x%04x\n", reg, val);
		}
	}

	return sprintf(buf, "%d\n", info->state.cabc_mode);
}

static DEVICE_ATTR(reg, S_IRUGO, reg_show, NULL);

static ssize_t force_pwm_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s: force_pwm_cnt=%d\n", __func__, info->force_pwm_cnt);

	return sprintf(buf, "%d\n", info->force_pwm_cnt);
}

static ssize_t force_pwm_cnt_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	u16 value;
	int ret;

	ret = kstrtou16(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	info->force_pwm_cnt = value;
	dev_info(dev, "%s: force_pwm_cnt=%d\n", __func__, info->force_pwm_cnt);

	cmc624_set_pwm(DEF_BRIGHTNESS_LEVEL);

	return size;
}

static DEVICE_ATTR(force_pwm_cnt, S_IRUGO|S_IWUSR|S_IWGRP,
			force_pwm_cnt_show, force_pwm_cnt_store);

static struct attribute *manual_cmc624_attributes[] = {
	&dev_attr_scenario.attr,
	&dev_attr_negative.attr,
	&dev_attr_accessibility.attr,
	&dev_attr_mode.attr,
	&dev_attr_tuning.attr,
	&dev_attr_pwm.attr,
	&dev_attr_cabc.attr,
	&dev_attr_reg.attr,
	&dev_attr_bypass.attr,
	&dev_attr_force_pwm_cnt.attr,
	NULL,
};

static const struct attribute_group manual_cmc624_group = {
	.attrs = manual_cmc624_attributes,
};

static ssize_t lcd_type_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", info->pdata->lcd_name);
}

static DEVICE_ATTR(lcd_type, S_IRUGO|S_IWUSR|S_IWGRP, lcd_type_show, NULL);

static ssize_t siop_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cmc624_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s: siop mode = %d\n", __func__, info->state.siop_mode);

	return sprintf(buf, "%d\n", info->state.siop_mode);
}

static ssize_t siop_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cmc624_info *info = dev_get_drvdata(dev);
	struct backlight_device *psb_backlight_device =
					psb_get_backlight_device();
	enum tune_menu_siop new_siop;
	int value;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	if (value)
		new_siop = MENU_SIOP_ON;
	else
		new_siop = MENU_SIOP_OFF;

	dev_info(dev, "%s: siop mode = %d -> %d\n",
				__func__, info->state.siop_mode, new_siop);

	if (new_siop == info->state.siop_mode) {
		dev_dbg(info->dev, "%s: skip to set siop mode\n", __func__);
		goto skip;
	}

	info->state.siop_mode = new_siop;

	if (info->state.suspended == true) {
		dev_info(info->dev, "%s: skip to set siop mode\n", __func__);
		goto skip;
	}

	cmc624_tune_restore(info);

	if (unlikely(!psb_backlight_device)) {
		pr_err("%s: ERR: backlight device is NULL\n", __func__);
		goto skip;
	}
	backlight_update_status(psb_backlight_device);

skip:
	return size;
}

static DEVICE_ATTR(siop_enable, S_IRUGO|S_IWUSR|S_IWGRP,
			siop_show, siop_store);

static struct attribute *manual_lcd_attributes[] = {
	&dev_attr_lcd_type.attr,
	&dev_attr_siop_enable.attr,
	NULL,
};

static const struct attribute_group manual_lcd_group = {
	.attrs = manual_lcd_attributes,
};

/**************************************************************************
 * SYSFS -- END
 ***************************************************************************/

static int cmc624_tune_restore(struct cmc624_info *info)
{
	int ret;

	pr_info("%s:bg=%d, app=%d, negative=%d, color=%d, cabc=%d, bypass=%d siop=%d\n",
			__func__, info->state.background, info->state.scenario,
			info->state.negative, info->state.accessibility,
			info->state.cabc_mode, info->state.bypass,
			info->state.siop_mode);

	if (info->state.negative ||
			info->state.accessibility == MENU_ACC_NEGATIVE)
		ret = apply_negative_tune_value(info);
	else if (info->state.accessibility == MENU_ACC_COLOR_BLIND)
		ret = apply_color_blind_tune_value(info);
	else if (info->state.bypass)
		ret = apply_bypass_tune_value(info, 1);
	else
		ret = apply_tune_value(info, info->state.background,
						info->state.scenario, 1);

	return ret;
}

static int cmc624_power_off_seq(struct cmc624_info *info)
{
	int ret;

	/* 1.2V/1.8V/3.3V may be on */

	/* CMC623[0x07] := 0x0004 */
	mutex_lock(&info->tune_lock);
	cmc624_reg_write(info, 0x07, 0x0004);
	mutex_unlock(&info->tune_lock);

	info->state.suspended = true;

	/* power off sequence */
	gpio_set_value(info->pdata->gpio_ima_cmc_en, 0);
	udelay(200);
	gpio_set_value(info->pdata->gpio_ima_nrst, 1);
	udelay(200);
	gpio_set_value(info->pdata->gpio_ima_sleep, 1);
	udelay(200);
#if 0
	info->pdata->power_lcd(0);
	udelay(200);
#endif

	/* power off */
	ret = info->pdata->power_vima_1_8V(0);
	if (ret) {
		dev_err(&info->client->dev, "%s: fail to control V_IMA_1.8V\n",
								__func__);
		goto out;
	}
	mdelay(20);

	ret = info->pdata->power_vima_1_1V(0);
	if (ret) {
		dev_err(&info->client->dev, "%s: fail to control V_IMA_1.1V\n",
								__func__);
		goto out;
	}
	udelay(100);

	msleep(200);

out:
	return ret;
}

static int cmc624_power_on_seq(struct cmc624_info *info)
{
	struct tune_data *data;
	struct backlight_device *psb_backlight_device =
					psb_get_backlight_device();
	u32 id;

	info->state.suspended = false;
	if (!info->pdata->skip_init) {
		/* set registers using I2C */
		TUNE_DATA_ID(id, MENU_CMD_INIT, MENU_SKIP,
				MENU_SKIP, MENU_SKIP, MENU_SKIP,
				MENU_SKIP, MENU_SKIP, MENU_SKIP);
		data = cmc624_get_tune_data(id, BITMASK_CMD);
		if (!data)
			return -ENODEV;
		cmc624_send_cmd(data->value, data->size);

		if (unlikely(!psb_backlight_device)) {
			pr_err("%s: ERR: backlight device is NULL\n", __func__);
			return -ENODEV;
		}
		backlight_update_status(psb_backlight_device);
	}

	if (!info->pdata->skip_init && !info->pdata->skip_ldi_cmd) {
		/* LDI sequence */
		TUNE_DATA_ID(id, MENU_CMD_INIT_LDI, MENU_SKIP,
				MENU_SKIP, MENU_SKIP, MENU_SKIP,
				MENU_SKIP, MENU_SKIP, MENU_SKIP);
		data = cmc624_get_tune_data(id, BITMASK_CMD);
		if (!data)
			return -ENODEV;
		cmc624_send_cmd(data->value, data->size);
	}

	info->pdata->skip_init = false;

	return 0;

}

static int cmc624_enable(void)
{
	struct cmc624_info *info = g_cmc624_info;
	struct cmc624_panel_data *pdata;
	int ret = 0;

	mutex_lock(&info->power_lock);
	dev_info(info->dev, "CMC624 enable\n");

	if (!(info && info->pdata)) {
		dev_err(info->dev, "%s: cmc624 is not initialized\n",
								__func__);
		ret = -ENODEV;
		goto err_nodev;
	}

	pdata = info->pdata;

	/* Delay recommended by panel DATASHEET */
	mdelay(4);

	if (!pdata->skip_init) {
		/* Set power */
		gpio_set_value(pdata->gpio_ima_nrst, 1);
		gpio_set_value(pdata->gpio_ima_cmc_en, 0);
		gpio_set_value(pdata->gpio_ima_sleep, 1);
		udelay(200);
	}

	/* V_IMA_1.1V EN*/
	ret = pdata->power_vima_1_1V(1);
	if (ret) {
		dev_err(info->dev, "%s: failed to control V_IMA_1.1V (%d)\n",
								__func__, ret);
		goto err_power_vima_1_1V;
	}
	udelay(100);

	/* V_IMA_1.8V, LCD VDD EN */
	ret = pdata->power_vima_1_8V(1);
	if (ret) {
		dev_err(info->dev, "%s failed to control V_IMA_1.8V (%d)\n",
								__func__, ret);
		goto err_power_vima_1_8V;
	}
	udelay(100);

	if (!pdata->skip_init) {
		/* FAIL_SAFEB HIGH */
		gpio_set_value(pdata->gpio_ima_cmc_en, 1);
		udelay(25);

		/* RESETB LOW */
		gpio_set_value(pdata->gpio_ima_nrst, 0);
		usleep_range(4100, 4410);

		/* RESETB HIGH */
		gpio_set_value(pdata->gpio_ima_nrst, 1);
	}

	cmc624_power_on_seq(info);
	cmc624_tune_restore(info);

	mutex_unlock(&info->power_lock);

	return 0;

err_power_vima_1_8V:
	pdata->power_vima_1_1V(0);
err_power_vima_1_1V:
	gpio_set_value(pdata->gpio_ima_nrst, 0);
	gpio_set_value(pdata->gpio_ima_cmc_en, 0);
	gpio_set_value(pdata->gpio_ima_sleep, 0);
err_nodev:
	mutex_unlock(&info->power_lock);
	return ret;
}

static void cmc624_disable(void)
{
	struct cmc624_info *info = g_cmc624_info;

	mutex_lock(&info->power_lock);
	dev_info(info->dev, "CMC624 disable\n");

	mdelay(4);

	cmc624_power_off_seq(info);

	mutex_unlock(&info->power_lock);
}

void cmc624_power(int on)
{
	if (on)
		cmc624_enable();
	else
		cmc624_disable();
}
EXPORT_SYMBOL(cmc624_power);

static int cmc624_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct cmc624_info *info;
	struct cmc624_panel_data *pdata = client->dev.platform_data;
	int i;
	int ret = 0;

	info = kzalloc(sizeof(struct cmc624_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "Failed to allocate memory for info\n");
		return -ENOMEM;
	}


	mutex_init(&info->tune_lock);
	mutex_init(&info->power_lock);

	info->client = client;

	/* set initial cmc624 state */
	info->state.cabc_mode = MENU_CABC_OFF;
	info->state.suspended = 0;
	info->state.scenario = MENU_APP_UI;
	info->state.background = MENU_MODE_STANDARD;
	info->state.negative = MENU_NEGATIVE_OFF;
	info->state.accessibility = MENU_ACC_OFF;
	info->state.bypass = MENU_BYPASS_OFF;
	info->state.siop_mode = MENU_SIOP_OFF;

	info->force_pwm_cnt = 0;

	for (i = 0; i < 3; i++)
		info->init_tune_flag[i] = 0;

	info->last_cmc624_Algorithm = 0xFFFF;
	info->last_cmc624_Bank = 0xFFFF;
	info->pdata = pdata;
	i2c_set_clientdata(client, info);

	if (!info->pdata) {
		dev_err(&client->dev, "%s: no panel data\n", __func__);
		ret = -ENODEV;
		goto no_panel_data;
	}

	if (!info->pdata->init_tune_list ||
			!info->pdata->power_vima_1_1V ||
			!info->pdata->power_vima_1_8V) {
		dev_err(&client->dev, "%s: no panel callback functions\n",
								__func__);
		ret = -ENODEV;
		goto no_panel_data;
	}

	/* set global cmc624 data */
	g_cmc624_info = info;


	/* register tune data list */
	ret = info->pdata->init_tune_list();
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to register tune list\n",
								__func__);
		goto cmc624_init_tune_list;
	}

	/* init V_IMA_1.1, V_IMA_1.8 */
	if (info->pdata->init_power) {
		ret = info->pdata->init_power();
		if (ret) {
			dev_err(&client->dev, "%s: failed to init power\n",
								__func__);
			goto cmc624_init_power;
		}
	}

	/* sysfs */
	info->mdnie_class = class_create(THIS_MODULE, "mdnie");
	if (IS_ERR(info->mdnie_class)) {
		dev_err(&client->dev, "%s: failed to create mdnie class\n",
								__func__);
		ret = -1;
		goto mdnie_class_create_fail;
	}

	info->lcd_class = class_create(THIS_MODULE, "lcd");
	if (IS_ERR(info->lcd_class)) {
		dev_err(&client->dev, "%s: failed to create lcd class\n",
								__func__);
		ret = -1;
		goto lcd_class_create_fail;
	}

	info->dev = device_create(info->mdnie_class, NULL, 0, NULL, "mdnie");
	if (IS_ERR(info->dev)) {
		dev_err(&client->dev, "%s: failed to create mdnie file\n",
								__func__);
		ret = -1;
		goto mdnie_device_create_fail;
	}

	info->lcd_dev = device_create(info->lcd_class, NULL, 0, NULL, "panel");
	if (IS_ERR(info->dev)) {
		dev_err(&client->dev, "%s: failed to create panel file\n",
								__func__);
		ret = -1;
		goto lcd_device_create_fail;
	}

	dev_set_drvdata(info->dev, info);
	dev_set_drvdata(info->lcd_dev, info);

	ret = sysfs_create_group(&info->dev->kobj, &manual_cmc624_group);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to create sysfs group\n",
								__func__);
		goto mdnie_sysfs_create_group_fail;
	}

	ret = sysfs_create_group(&info->lcd_dev->kobj, &manual_lcd_group);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to create lcd sysfs group\n",
								__func__);
		goto lcd_sysfs_create_group_fail;
	}

	return 0;

lcd_sysfs_create_group_fail:
	sysfs_remove_group(&info->dev->kobj, &manual_cmc624_group);
mdnie_sysfs_create_group_fail:
	device_destroy(info->lcd_class, 0);
lcd_device_create_fail:
	device_destroy(info->mdnie_class, 0);
mdnie_device_create_fail:
	class_destroy(info->lcd_class);
lcd_class_create_fail:
	class_destroy(info->mdnie_class);
mdnie_class_create_fail:
no_panel_data:
cmc624_init_power:
cmc624_init_tune_list:
	mutex_destroy(&info->tune_lock);
	mutex_destroy(&info->power_lock);
	kfree(info);

	return 0;
}

static int __devexit cmc624_i2c_remove(struct i2c_client *client)
{
	struct cmc624_info *info = i2c_get_clientdata(client);

	i2c_set_clientdata(client, NULL);
	device_destroy(info->mdnie_class, 0);
	class_destroy(info->mdnie_class);

	mutex_destroy(&info->tune_lock);
	mutex_destroy(&info->power_lock);
	kfree(info);

	return 0;
}

static const struct i2c_device_id sec_cmc624_ids[] = {
	{ "sec_cmc624_i2c", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, sec_cmc624_ids);

struct i2c_driver sec_cmc624_i2c_driver = {
	.driver	= {
		.name	= "sec_cmc624_i2c",
		.owner = THIS_MODULE,
	},
	.probe	= cmc624_i2c_probe,
	.remove	= __devexit_p(cmc624_i2c_remove),
	.id_table	= sec_cmc624_ids,
};

static int __init cmc624_init(void)
{
	return i2c_add_driver(&sec_cmc624_i2c_driver);
}
module_init(cmc624_init);

static void __exit cmc624_exit(void)
{
	i2c_del_driver(&sec_cmc624_i2c_driver);
}
module_exit(cmc624_exit);
