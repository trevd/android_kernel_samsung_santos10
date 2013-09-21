/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/printk.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include <linux/platform_data/sec_ts.h>
#include <linux/input/atmel_mxt1188s.h>

#define MXT_DEV_NAME			"mxt1188s_ts"

#define MXT_BACKUP_TIME			25	/* msec */
#define MXT_RESET_INTEVAL_TIME		50	/* msec */
#define MXT_SW_RESET_TIME		300	/* msec */
#define MXT_HW_RESET_TIME		80	/* msec */

#define MXT_OBJECT_TABLE_ELEMENT_SIZE	6
#define MXT_OBJECT_TABLE_START_ADDRESS	7

#define MXT_MAX_FINGER			10
#define MXT_AREA_MAX			255
#define MXT_AMPLITUDE_MAX		255

/*
 * If the firmware can support dynamic configuration,
 * 0x55 : backup configuration data and resume event handling.
 * 0x44 : restore configuration data
 * 0x33 : restore configuration data and stop event handiling.
 *
 * if the firmware did not support dynamic configuration,
 * 0x33, 0x44 is not affect.
 */
#define MXT_BACKUP_VALUE		0x55
#define MXT_RESTORE_VALUE		0x44
#define MXT_DISALEEVT_VALUE		0x33

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0
#define MXT_WAITING_FRAME_DATA		0x80
#define MXT_FRAME_CRC_CHECK		0x02
#define MXT_FRAME_CRC_FAIL		0x03
#define MXT_FRAME_CRC_PASS		0x04
#define MXT_APP_CRC_FAIL		0x40
#define MXT_BOOT_STATUS_MASK		0x3f

/* MXT_GEN_COMMAND_T6 Field */
enum {
	MXT_COMMAND_RESET = 0,
	MXT_COMMAND_BACKUPNV,
	MXT_COMMAND_CALIBRATE,
	MXT_COMMAND_REPORTALL,
	MXT_COMMAND_DEBUGCTL,
	MXT_COMMAND_DIAGNOSTIC,
};

enum {
	MXT_STATE_INACTIVE = 0,
	MXT_STATE_RELEASE,
	MXT_STATE_PRESS,
	MXT_STATE_MOVE,
};

enum {
	MXT_RESERVED_T0 = 0,
	MXT_RESERVED_T1,
	MXT_DEBUG_DELTAS_T2,
	MXT_DEBUG_REFERENCES_T3,
	MXT_DEBUG_SIGNALS_T4,
	MXT_GEN_MESSAGEPROCESSOR_T5,
	MXT_GEN_COMMANDPROCESSOR_T6,
	MXT_GEN_POWERCONFIG_T7,
	MXT_GEN_ACQUISITIONCONFIG_T8,
	MXT_TOUCH_MULTITOUCHSCREEN_T9,
	MXT_TOUCH_SINGLETOUCHSCREEN_T10,
	MXT_TOUCH_XSLIDER_T11,
	MXT_TOUCH_YSLIDER_T12,
	MXT_TOUCH_XWHEEL_T13,
	MXT_TOUCH_YWHEEL_T14,
	MXT_TOUCH_KEYARRAY_T15,
	MXT_PROCG_SIGNALFILTER_T16,
	MXT_PROCI_LINEARIZATIONTABLE_T17,
	MXT_SPT_COMCONFIG_T18,
	MXT_SPT_GPIOPWM_T19,
	MXT_PROCI_GRIPFACESUPPRESSION_T20,
	MXT_RESERVED_T21,
	MXT_PROCG_NOISESUPPRESSION_T22,
	MXT_TOUCH_PROXIMITY_T23,
	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24,
	MXT_SPT_SELFTEST_T25,
	MXT_DEBUG_CTERANGE_T26,
	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
	MXT_SPT_CTECONFIG_T28,
	MXT_SPT_GPI_T29,
	MXT_SPT_GATE_T30,
	MXT_TOUCH_KEYSET_T31,
	MXT_TOUCH_XSLIDERSET_T32,
	MXT_RESERVED_T33,
	MXT_GEN_MESSAGEBLOCK_T34,
	MXT_SPT_GENERICDATA_T35,
	MXT_RESERVED_T36,
	MXT_DEBUG_DIAGNOSTIC_T37,
	MXT_SPT_USERDATA_T38,
	MXT_SPARE_T39,
	MXT_PROCI_GRIPSUPPRESSION_T40,
	MXT_SPARE_T41,
	MXT_PROCI_TOUCHSUPPRESSION_T42,
	MXT_SPT_DIGITIZER_T43,
	MXT_SPARE_T44,
	MXT_SPARE_T45,
	MXT_SPT_CTECONFIG_T46,
	MXT_PROCI_STYLUS_T47,
	MXT_PROCG_NOISESUPPRESSION_T48,
	MXT_SPARE_T49,
	MXT_SPARE_T50,
	MXT_SPARE_T51,
	MXT_TOUCH_PROXIMITY_KEY_T52,
	MXT_GEN_DATASOURCE_T53,
	MXT_SPARE_T54,
	MXT_ADAPTIVE_T55,
	MXT_PROCI_SHIELDLESS_T56,
	MXT_PROCI_EXTRATOUCHSCREENDATA_T57,
	MXT_SPARE_T58,
	MXT_SPARE_T59,
	MXT_SPARE_T60,
	MXT_SPT_TIMER_T61,
	MXT_PROCG_NOISESUPPRESSION_T62,
	MXT_PROCI_ACTIVESTYLUS_T63,
	MXT_SPARE_T64,
	MXT_PROCI_LENSBENDING_T65,
	MXT_SPT_GOLDENREFERENCES_T66,
	MXT_SPARE_T67,
	MXT_SPARE_T68,
	MXT_PROCI_PALMGESTUREPROCESSOR_T69,
	MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70,
	MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71,
	MXT_PROCG_NOISESUPPRESSION_T72,
	MXT_RESERVED_T255 = 255,
};

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;

	/* added for mapping obj to report id*/
	u8 max_reportid;
};

struct mxt_id_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_message {
	u8 reportid;
	u8 message[8];
};

/*
 * struct mxt_finger - Represents fingers.
 * @ x: x position.
 * @ y: y position.
 * @ w: size of touch.
 * @ z: touch amplitude(sum of measured deltas).
 * @ state: finger status.
 * @ type: type of touch. if firmware can support.
 * @ mcount: moving counter for debug.
 */
struct mxt_finger {
	u16 x;
	u16 y;
	u16 w;
	u16 z;
	u8 state;
	u8 type;
	u8 event;
	u16 mcount;
};

struct mxt_reportid {
	u8 type;
	u8 index;
};

/*
 * struct mxt_cfg_data - Represents a configuration data item.
 * @ type: Type of object.
 * @ instance: Instance number of object.
 * @ size: Size of object.
 * @ register_val: Series of register values for object.
 */
struct mxt_cfg_data {
	u8 type;
	u8 instance;
	u8 size;
	u8 register_val[0];
} __packed;

/*
 * struct mxt_fw_image - Represents a firmware file.
 * @ magic_code: Identifier of file type.
 * @ hdr_len: Size of file header (struct mxt_fw_image).
 * @ cfg_len: Size of configuration data.
 * @ fw_len: Size of firmware data.
 * @ cfg_crc: CRC of configuration settings.
 * @ fw_ver: Version of firmware.
 * @ build_ver: Build version of firmware.
 * @ data: Configuration data followed by firmware image.
 */
struct mxt_fw_image {
	__le32 magic_code;
	__le32 hdr_len;
	__le32 cfg_len;
	__le32 fw_len;
	__le32 cfg_crc;
	u8 fw_ver;
	u8 build_ver;
	u8 data[0];
} __packed;

struct mxt_fw_info {
	char release_date[5];
	u8 fw_ver;
	u8 build_ver;
	u32 hdr_len;
	u32 cfg_len;
	u32 fw_len;
	u32 cfg_crc;
	const u8 *cfg_raw_data;	/* start address of configuration data */
	const u8 *fw_raw_data;	/* start address of firmware data */
};

#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
struct atmel_dbg {
	u16 last_read_addr;
	u8 stop_sync;		/* to disable input sync report */
	u8 display_log;		/* to display raw message */
	u8 block_access;	/* to prevent access IC with I2c */
};
#endif

#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)
#define TSP_VENDOR			"ATMEL"
#define TSP_IC				"MXT1188S"

#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN		512
#define TSP_CMD_PARAM_NUM		8

struct factory_data {
	struct list_head		cmd_list_head;
	u8				cmd_state;
	char				cmd[TSP_CMD_STR_LEN];
	int				cmd_param[TSP_CMD_PARAM_NUM];
	char				cmd_result[TSP_CMD_RESULT_STR_LEN];
	char				cmd_buff[TSP_CMD_RESULT_STR_LEN];
	struct mutex			cmd_lock;
	bool				cmd_is_running;
};

struct node_data {
	s16				*reference_data;
	s16				*delta_data;
	s16				delta_max_data;
	s16				delta_max_node;
	s16				ref_max_data;
	s16				ref_min_data;
};
#endif


#define MXT_T61_TIMER_ONESHOT		0
#define MXT_T61_TIMER_CMD_START		1
#define MXT_T61_TIMER_CMD_STOP		2

struct mxt_atch {
	u8 enable;
	u8 coin;
	u8 calgood;
	u8 autocal;
	u8 timer;
	u8 timer_id;
	u8 coin_time;
	u8 calgood_time;
	u8 autocal_size;
	u8 autocal_time;
	u8 autocal_distance;
	u8 coin_atch_min;
	u8 coin_atch_ratio;
	u8 big_atch_ratio;
	u8 big_atch_tch_max;
	u8 big_tch_ratio;
	u8 big_tch_atch_min;
	u8 prev_chargin_status;
	u8 gstylus_diff;
	u8 abnormal_enable;
	u8 bigpalm_enable;
};

struct mxt_data {
	u8 max_reportid;
	u32 finger_mask;
	bool mxt_enabled;
	bool *mxt_key_state;
	struct i2c_client		*client;
	struct i2c_client		*client_boot;
	struct input_dev		*input_dev;
	struct mxt_id_info		id_info;
	struct mxt_object		*objects;
	struct mxt_reportid		*reportids;
	struct mxt_finger		fingers[MXT_MAX_FINGER];
	struct mxt_fw_info		fw_info, bin_fw_info;
	struct completion		init_done;
	struct sec_ts_platform_data	*pdata;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend		early_suspend;
#endif
#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
	struct atmel_dbg		atmeldbg;
#endif
	int finger_cnt;
#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)
	struct factory_data		*factory_data;
	struct node_data		*node_data;
#endif
	struct mxt_atch			atch;
	u16 defend_sum_size;
	u16 defend_atch;
	u16 defend_tch;
	bool touch_is_pressed;
	bool cal_is_ongoing;
	u8 t9_size;
	u8 t9_amp;
	u8 t70_touch_event;
};

#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
#define __mxt_debug_msg(_data, ...)					\
do {									\
	if (unlikely((_data)->atmeldbg.display_log))			\
		dev_info(&(_data)->client->dev, __VA_ARGS__);		\
} while (0)
#else
#define __mxt_debug_msg(_data, ...)
#endif

#define __mid_log(_data, ...)						\
do {									\
	if (data->pdata->log_on)					\
		dev_info(&(_data)->client->dev, __VA_ARGS__);		\
} while (0)								\

static int mxt_wait_for_chg(struct mxt_data *data, u16 time)
{
	int timeout_counter = 0;

	msleep(time);
	while (gpio_get_value(data->pdata->gpio_irq)
						&& timeout_counter++ <= 20) {
		msleep(MXT_RESET_INTEVAL_TIME);
		dev_err(&data->client->dev,
			"Spend %d time waiting for chg_high\n",
			(MXT_RESET_INTEVAL_TIME * timeout_counter) + time);
	}

	return 0;
}

static int mxt_read_mem(struct mxt_data *data, u16 reg, u8 len, void *buf)
{
	int ret = 0, i = 0;
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.len = 2,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	for (i = 0; i < 3 ; i++) {
		ret = i2c_transfer(data->client->adapter, msg, 2);
		if (ret < 0)
			dev_err(&data->client->dev, "%s fail[%d] address[0x%x]\n",
				__func__, ret, le_reg);
		else
			break;
	}
	return ret == 2 ? 0 : -EIO;
}

static int mxt_write_mem(struct mxt_data *data, u16 reg, u8 len, const u8 *buf)
{
	int ret = 0, i = 0;
	u8 tmp[len + 2];

	put_unaligned_le16(cpu_to_le16(reg), tmp);
	memcpy(tmp + 2, buf, len);

	for (i = 0; i < 3 ; i++) {
		ret = i2c_master_send(data->client, tmp, sizeof(tmp));
		if (ret < 0)
			dev_err(&data->client->dev,	"%s %d times write error on address[0x%x,0x%x]\n",
				__func__, i, tmp[1], tmp[0]);
		else
			break;
	}

	return ret == sizeof(tmp) ? 0 : -EIO;
}

static struct mxt_object *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	if (!data->objects)
		return NULL;

	for (i = 0; i < data->id_info.object_num; i++) {
		object = data->objects + i;
		if (object->type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type T%d\n", type);

	return NULL;
}

static int mxt_read_message(struct mxt_data *data, struct mxt_message *message)
{
	struct mxt_object *object;

	object = mxt_get_object(data, MXT_GEN_MESSAGEPROCESSOR_T5);
	if (!object)
		return -EINVAL;

	return mxt_read_mem(data, object->start_address,
					sizeof(struct mxt_message), message);
}

static int mxt_read_message_reportid(struct mxt_data *data,
				struct mxt_message *message, u8 reportid)
{
	int try = 0;
	int error;
	int fail_count;

	fail_count = data->max_reportid * 2;

	while (++try < fail_count) {
		error = mxt_read_message(data, message);
		if (error)
			return error;

		if (message->reportid == 0xff)
			continue;

		if (message->reportid == reportid)
			return 0;
	}

	return -EINVAL;
}

static int mxt_read_object(struct mxt_data *data, u8 type, u8 offset, u8 *val)
{
	struct mxt_object *object;
	int error = 0;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	error = mxt_read_mem(data, object->start_address + offset, 1, val);
	if (error)
		dev_err(&data->client->dev, "Error to read T[%d] offset[%d] val[%d]\n",
			type, offset, *val);
	return error;
}

static int mxt_write_object(struct mxt_data *data, u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	int error = 0;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	if (offset >= object->size * object->instances) {
		dev_err(&data->client->dev, "Tried to write outside object T%d offset:%d, size:%d\n",
			type, offset, object->size);
		return -EINVAL;
	}
	reg = object->start_address;
	error = mxt_write_mem(data, reg + offset, 1, &val);
	if (error)
		dev_err(&data->client->dev, "Error to write T[%d] offset[%d] val[%d]\n",
			type, offset, val);

	return error;
}

static u32 mxt_make_crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16)byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32)data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}

static int mxt_calculate_infoblock_crc(struct mxt_data *data, u32 *crc_pointer)
{
	u32 crc = 0;
	u8 mem[7 + data->id_info.object_num * 6];
	int ret;
	int i;

	ret = mxt_read_mem(data, 0, sizeof(mem), mem);

	if (ret)
		return ret;

	for (i = 0; i < sizeof(mem) - 1; i += 2)
		crc = mxt_make_crc24(crc, mem[i], mem[i + 1]);

	*crc_pointer = mxt_make_crc24(crc, mem[i], 0) & 0x00FFFFFF;

	return 0;
}

static int mxt_read_info_crc(struct mxt_data *data, u32 *crc_pointer)
{
	u16 crc_address;
	u8 msg[3];
	int ret;

	/* Read Info block CRC address */
	crc_address = MXT_OBJECT_TABLE_START_ADDRESS +
		data->id_info.object_num * MXT_OBJECT_TABLE_ELEMENT_SIZE;

	ret = mxt_read_mem(data, crc_address, 3, msg);
	if (ret)
		return ret;

	*crc_pointer = msg[0] | (msg[1] << 8) | (msg[2] << 16);

	return 0;
}

static int mxt_read_config_crc(struct mxt_data *data, u32 *crc)
{
	struct device *dev = &data->client->dev;
	struct mxt_message message;
	struct mxt_object *object;
	int error;

	object = mxt_get_object(data, MXT_GEN_COMMANDPROCESSOR_T6);
	if (!object)
		return -EIO;

	/* Try to read the config checksum of the existing cfg */
	mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
						MXT_COMMAND_REPORTALL, 1);

	/* Read message from command processor, which only has one report ID */
	error = mxt_read_message_reportid(data, &message, object->max_reportid);
	if (error) {
		dev_err(dev, "Failed to retrieve CRC\n");
		return error;
	}

	/* Bytes 1-3 are the checksum. */
	*crc = message.message[1] | (message.message[2] << 8) |
		(message.message[3] << 16);

	return 0;
}

static int mxt_write_config(struct mxt_data *data)
{
	struct mxt_fw_info *fw_info = &data->fw_info;
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	struct mxt_cfg_data *cfg_data;
	u32 current_crc;
	u8 i, val = 0;
	u16 reg, index;
	int ret = 0;

	if (!fw_info->cfg_raw_data) {
		dev_info(dev, "No cfg data in file\n");
		return ret;
	}

	/* Get config CRC from device */
	ret = mxt_read_config_crc(data, &current_crc);
	if (ret)
		return ret;

	/* Check Version information */
	if (fw_info->fw_ver != data->id_info.version) {
		dev_err(dev, "%s: version mismatch! fw_ver: 0x%X, id_version: 0x%X\n",
			__func__, fw_info->fw_ver, data->id_info.version);
		return 0;
	}

	if (fw_info->build_ver != data->id_info.build) {
		dev_err(dev, "Warning: build num mismatch! %s\n", __func__);
		return 0;
	}

	/* Check config CRC */
	if (current_crc == fw_info->cfg_crc) {
		dev_info(dev, "Skip writing Config: CRC 0x%.6X\n", current_crc);
		return 0;
	}

	dev_info(dev, "Writing Config: CRC 0x%06X != 0x%06X\n",
						current_crc, fw_info->cfg_crc);

	/* Write config info */
	for (index = 0; index < fw_info->cfg_len;) {

		if (index + sizeof(struct mxt_cfg_data) >= fw_info->cfg_len) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d)!!\n",
				index + sizeof(struct mxt_cfg_data),
				fw_info->cfg_len);
			return -EINVAL;
		}

		/* Get the info about each object */
		cfg_data = (struct mxt_cfg_data *)&fw_info->cfg_raw_data[index];

		index += sizeof(struct mxt_cfg_data) + cfg_data->size;
		if (index > fw_info->cfg_len) {
			dev_err(dev, "index(%d) of cfg_data exceeded total size(%d) in T%d object!!\n",
				index, fw_info->cfg_len, cfg_data->type);
			return -EINVAL;
		}

		object = mxt_get_object(data, cfg_data->type);
		if (!object) {
			dev_err(dev, "T%d is Invalid object type\n",
				cfg_data->type);
			return -EINVAL;
		}

		/* Check and compare the size, instance of each object */
		if (cfg_data->size > object->size) {
			dev_err(dev, "T%d Object length exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}
		if (cfg_data->instance >= object->instances) {
			dev_err(dev, "T%d Object instances exceeded!\n",
				cfg_data->type);
			return -EINVAL;
		}

		dev_dbg(dev, "Writing config for obj %d len %d instance %d (%d/%d)\n",
							cfg_data->type,
							object->size,
							cfg_data->instance,
							index,
							fw_info->cfg_len);

		reg = object->start_address + object->size * cfg_data->instance;

		/* Write register values of each object */
		ret = mxt_write_mem(data, reg, cfg_data->size,
							cfg_data->register_val);
		if (ret) {
			dev_err(dev, "Write T%d Object failed\n", object->type);
			return ret;
		}

		/*
		 * If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated.
		 */
		if (cfg_data->size < object->size) {
			dev_err(dev, "Warning: zeroing %d byte(s) in T%d\n",
						object->size - cfg_data->size,
						cfg_data->type);

			for (i = cfg_data->size + 1; i < object->size; i++) {
				ret = mxt_write_mem(data, reg + i, 1, &val);
				if (ret)
					return ret;
			}
		}
	}

	dev_info(dev, "Updated configuration\n");

	return ret;
}

static int mxt_command_reset(struct mxt_data *data, u8 value)
{
	int error;

	mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
						MXT_COMMAND_RESET, value);
	error = mxt_wait_for_chg(data, MXT_SW_RESET_TIME);

	if (error)
		dev_err(&data->client->dev,
				"Not respond after reset command[%d]\n", value);
	return error;
}

static int mxt_command_backup(struct mxt_data *data, u8 value)
{
	mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
						MXT_COMMAND_BACKUPNV, value);
	msleep(MXT_BACKUP_TIME);

	return 0;
}

static void mxt_report_input_data(struct mxt_data *data)
{
	int i;
	int count = 0;

	for (i = 0; i < MXT_MAX_FINGER; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;

		input_mt_slot(data->input_dev, i);
		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			input_mt_report_slot_state(data->input_dev,
							MT_TOOL_FINGER, false);
		} else {
			input_mt_report_slot_state(data->input_dev,
							MT_TOOL_FINGER, true);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
							data->fingers[i].x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
							data->fingers[i].y);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
							data->fingers[i].w);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE,
							data->fingers[i].z);
		}

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			data->finger_cnt--;
			__mid_log(data, "finger %d up. remain : %d\n",
						i, data->finger_cnt);
		} else if (data->fingers[i].state == MXT_STATE_PRESS) {
			data->finger_cnt++;
			__mid_log(data, "finger %d dn. remain : %d\n",
						i, data->finger_cnt);
			__mid_log(data, "sum_size: %d, atch_area: %d, tch_area: %d\n",
						data->defend_sum_size,
						data->defend_atch,
						data->defend_tch);
			__mxt_debug_msg(data,
					"[x:%d, y:%d], press state: sum_size: %d, atch_area: %d, tch_area: %d\n",
						data->fingers[i].x,
						data->fingers[i].y,
						data->defend_sum_size,
						data->defend_atch,
						data->defend_tch);
		}

		if (data->fingers[i].state == MXT_STATE_RELEASE) {
			data->fingers[i].state = MXT_STATE_INACTIVE;
			data->fingers[i].mcount = 0;
		} else {
			data->fingers[i].state = MXT_STATE_MOVE;
			count++;
		}

		data->touch_is_pressed = count ? true : false;
	}

	data->finger_mask = 0;
	input_sync(data->input_dev);
}

static void mxt_release_all_finger(struct mxt_data *data)
{
	int i;
	int count = 0;

	for (i = 0; i < MXT_MAX_FINGER; i++) {
		if (data->fingers[i].state == MXT_STATE_INACTIVE)
			continue;
		data->fingers[i].z = 0;
		data->fingers[i].state = MXT_STATE_RELEASE;
		count++;
	}
	if (count) {
		dev_err(&data->client->dev, "%s\n", __func__);
		mxt_report_input_data(data);
	}

	data->finger_cnt = 0;
}

static void mxt_release_all_keys(struct mxt_data *data)
{
	int i = 0;

	for (i = 0; i < data->pdata->key_size; i++) {
		data->mxt_key_state[i] = false;
		input_report_key(data->input_dev, data->pdata->key[i].code,
							data->mxt_key_state[i]);
	}

	input_sync(data->input_dev);
}

void mxt_set_T8_object_autocal(struct mxt_data *data, u8 mstime)
{
	__mxt_debug_msg(data, "T8 Autocal Set %d x200ms\n", mstime);

	mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 4, mstime);
}


void mxt_set_T61_object(struct mxt_data *data, u8 id, u8 mode,
							u8 cmd, u16 ms_period)
{
	struct mxt_object *object;
	int ret = 0;
	u8 buf[5] = {3, 0, 0, 0, 0};

	object = mxt_get_object(data, MXT_SPT_TIMER_T61);

	buf[1] = cmd;
	buf[2] = mode;
	buf[3] = ms_period & 0xFF;
	buf[4] = (ms_period >> 8) & 0xFF;

	ret = mxt_write_mem(data, object->start_address+5*id, 5, buf);

	if (ret)
		dev_err(&data->client->dev,
			"%s write error T%d(%d) address[0x%x]\n",
			__func__, MXT_SPT_TIMER_T61, id, object->start_address);

	__mxt_debug_msg(data, "T61[%d] Timer Set %d ms\n", id, ms_period);
}

static void mxt_atch_check_coordinate(struct mxt_data *data,
	bool detect, u8 id, u16 *px, u16 *py)
{
	static int tcount[] = {0, };
	static u16 pre_x[][4] = {{0}, };
	static u16 pre_y[][4] = {{0}, };
	int distance = 0;

	if (detect)
		tcount[id] = 0;

	if (tcount[id] > 1) {
		distance = abs(pre_x[id][0] - *px) + abs(pre_y[id][0] - *py);
		__mxt_debug_msg(data, "ATCH: Check Distance ID:%d, %d\n",
				id, distance);

		if (distance > data->atch.autocal_distance) {
			mxt_set_T8_object_autocal(data, 0);
			data->atch.autocal = 0;
		}
	}

	pre_x[id][0] = *px;
	pre_y[id][0] = *py;

	tcount[id]++;
}

void mxt_atch_check_calibration(struct mxt_data *data)
{
	if (data->atch.enable) {
		/* Fast Drift */
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 2, 1);
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 3, 1);
		/* Disable Stylus */
		mxt_write_object(data, MXT_PROCI_STYLUS_T47, 1, 250);
		/* Lensbending On */
		mxt_write_object(data, MXT_PROCI_LENSBENDING_T65, 0, 3);
		/* Touch Event Off */
		mxt_write_object(data,
			MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70, 0, 0);
		mxt_set_T61_object(data, data->atch.timer_id,
						MXT_T61_TIMER_ONESHOT,
						MXT_T61_TIMER_CMD_STOP, 0);
		data->atch.timer = 0;
		data->atch.coin = 1;
		data->atch.calgood = 0;
		data->atch.abnormal_enable = 0;
		data->atch.bigpalm_enable = 0;
	}
}

#define GHOST_STYLUS	15

void mxt_atch_init(struct mxt_data *data)
{
	struct mxt_object *obj;
	u8 buf[32];
	u8 offset;
	int error;
	/*  Init flags */
	data->atch.enable = 0;
	data->atch.coin = 0;
	data->atch.calgood = 0;
	data->atch.autocal = 0;
	data->atch.timer = 0;

	obj = mxt_get_object(data, MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71);
	error = mxt_read_mem(data, obj->start_address,
						sizeof(struct mxt_atch), buf);

	if (error || buf[0] != 255)
		dev_err(&data->client->dev, "Error to read T71 Container\n");
	else if (!error && buf[0] == 255) {
		dev_info(&data->client->dev,
				"ATCH: Read Check configuration size:%d\n",
				sizeof(struct mxt_atch));

		offset = buf[1];

		if (unlikely(offset >= 19)) {
			dev_err(&data->client->dev, "Error T71 offset\n");
			WARN_ON(true);
			return;
		}

		data->atch.timer_id = buf[2+offset]; /* 0 */
		data->atch.coin_time = 1;
		data->atch.calgood_time = 1;

		data->atch.autocal_size = buf[5+offset]; /*2 */
		data->atch.autocal_time = buf[6+offset]; /* 5 */
		data->atch.autocal_distance = buf[7+offset]; /*3 */

		data->atch.coin_atch_min = buf[8+offset]; /* 5 */
		data->atch.coin_atch_ratio = buf[9+offset]; /* 0 */

		data->atch.big_atch_ratio = buf[10+offset]; /* 8 */
		data->atch.big_atch_tch_max = buf[11+offset]; /* 35 */

		data->atch.big_tch_ratio = buf[12+offset]; /* 40 */
		data->atch.big_tch_atch_min = buf[13+offset]; /*8 */

		data->atch.prev_chargin_status = 0xff;
		data->atch.enable = 1;

		data->atch.gstylus_diff = GHOST_STYLUS;
	}
}

/* The Command Processor T6 object allows commands to be sent to the device.
 * This is done by writing an appropriate value to one of its fiels.
 */
static void mxt_treat_T6_object(struct mxt_data *data,
						struct mxt_message *message)
{
	/* Normal mode */
	if (message->message[0] == 0x00)
		dev_info(&data->client->dev, "Normal mode\n");
	/* I2C checksum error */
	if (message->message[0] & 0x04)
		dev_err(&data->client->dev, "I2C checksum error\n");
	/* Config error */
	if (message->message[0] & 0x08)
		dev_err(&data->client->dev, "Config error\n");
	/* Calibration */
	if (message->message[0] & 0x10) {
		dev_info(&data->client->dev, "Calibration is on going !!\n");
		mxt_atch_check_calibration(data);
		data->cal_is_ongoing = 1;
	} else
		data->cal_is_ongoing = 0;
	/* Signal error */
	if (message->message[0] & 0x20)
		dev_err(&data->client->dev, "Signal error\n");
	/* Overflow */
	if (message->message[0] & 0x40)
		dev_err(&data->client->dev, "Overflow detected\n");
	/* Reset */
	if (message->message[0] & 0x80) {
		dev_info(&data->client->dev, "Reset is ongoing\n");
		data->atch.prev_chargin_status = 0xff;
		mxt_atch_check_calibration(data);
	}
}

/* Touch objects operate on measured signals from the touch sensor and report
 * touch data. For example, a Multiple Touch Touchscreen T9 object reports XY
 * touch positions.
 */
#define MXT_SUPPRESS_MSG_MASK		(1 << 1)
#define MXT_AMPLITUDE_MSG_MASK		(1 << 2)
#define MXT_VECTOR_MSG_MASK		(1 << 3)
#define MXT_MOVE_MSG_MASK		(1 << 4)
#define MXT_RELEASE_MSG_MASK		(1 << 5)
#define MXT_PRESS_MSG_MASK		(1 << 6)
#define MXT_DETECT_MSG_MASK		(1 << 7)

static void mxt_treat_T9_object(struct mxt_data *data,
						struct mxt_message *message)
{
	int id;
	u8 *msg = message->message;

	id = data->reportids[message->reportid].index;

	if (id >= MXT_MAX_FINGER) {
		/* If not a touch event, return */
		dev_err(&data->client->dev, "MAX_FINGER exceeded!\n");
		return;
	}

	if (data->finger_mask & (1U << id))
		mxt_report_input_data(data);

	if (msg[0] & MXT_RELEASE_MSG_MASK) {
		data->fingers[id].z = 0;
		data->fingers[id].w = msg[4];
		data->fingers[id].state = MXT_STATE_RELEASE;

	} else if ((msg[0] & MXT_DETECT_MSG_MASK) &&
			(msg[0] & (MXT_PRESS_MSG_MASK | MXT_MOVE_MSG_MASK))) {
		data->fingers[id].x = (msg[1] << 4) | (msg[3] >> 4);
		data->fingers[id].y = (msg[2] << 4) | (msg[3] & 0xF);
		data->fingers[id].w = msg[4];
		data->fingers[id].z = msg[5];
		data->t9_size = msg[4];
		data->t9_amp = msg[5];

		if (data->pdata->x_pixel_size < 1024)
			data->fingers[id].x = data->fingers[id].x >> 2;
		if (data->pdata->y_pixel_size < 1024)
			data->fingers[id].y = data->fingers[id].y >> 2;

		if (msg[0] & MXT_PRESS_MSG_MASK) {
			data->fingers[id].state = MXT_STATE_PRESS;
			data->fingers[id].mcount = 0;
			data->t70_touch_event = 0;
			if (data->atch.autocal)
				mxt_atch_check_coordinate(data, 1, id,
							&data->fingers[id].x,
							&data->fingers[id].y);
		} else if (msg[0] & MXT_MOVE_MSG_MASK) {
			data->fingers[id].mcount += 1;
			if (data->atch.autocal)
				mxt_atch_check_coordinate(data, 0, id,
							&data->fingers[id].x,
							&data->fingers[id].y);
		}
	} else if ((msg[0] & MXT_SUPPRESS_MSG_MASK)
			&& (data->fingers[id].state != MXT_STATE_INACTIVE)) {
		data->fingers[id].z = 0;
		data->fingers[id].w = msg[4];
		data->fingers[id].state = MXT_STATE_RELEASE;

	} else {
		/* ignore changed amplitude and vector messsage */
		if (unlikely(!((msg[0] & MXT_DETECT_MSG_MASK)
					&& (msg[0] & MXT_AMPLITUDE_MSG_MASK
					 || msg[0] & MXT_VECTOR_MSG_MASK))))
			dev_err(&data->client->dev, "Unknown state %#02x %#02x\n",
								msg[0], msg[1]);
	}

	data->finger_mask |= 1U << id;
}

/* A Key Array T15 object is used to configure a rectagular array of XY channels
 * on the mutual-capacitance sensor for use as keys.
 */
#define MXT_MSG_T15_STATUS	0x00
#define MXT_MSG_T15_KEYSTATE	0x01

static void mxt_treat_T15_object(struct mxt_data *data,
						struct mxt_message *message)
{
	int i;
	u8 state;
	u8 input_message = message->message[MXT_MSG_T15_KEYSTATE];
	const struct touch_key *touch_key = data->pdata->key;

	for (i = 0 ; i < data->pdata->key_size ; i++) {
		state = input_message & touch_key[i].mask;
		if (state != data->mxt_key_state[i]) {
			data->mxt_key_state[i] = state;
			input_report_key(data->input_dev, touch_key[i].code,
									state);
			input_sync(data->input_dev);
			dev_dbg(&data->client->dev, "key: %s: %d\n",
							touch_key[i].name,
							state ? 1 : 0);
		}
	}

	return;
}

/* A Touch Suppression T42 object processes the measurement data received from
 * a particlar instance of a Multiple Touch Touchscreeen T9 object.
 */
static void mxt_treat_T42_object(struct mxt_data *data,
						struct mxt_message *message)
{
	if (message->message[0] & 0x01)
		/* Palm Press */
		dev_info(&data->client->dev, "palm touch detected\n");
	else
		/* Palm release */
		dev_info(&data->client->dev, "palm touch released\n");
}

static int mxt_command_calibration(struct mxt_data *data)
{
	return  mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
						MXT_COMMAND_CALIBRATE, 1);
}

static void mxt_check_command_calibration(struct mxt_data *data)
{
	if (data->cal_is_ongoing == 0) {
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 8, 10);
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 9, 1);
		mxt_write_object(data, MXT_PROCI_LENSBENDING_T65, 0, 3);
		mxt_write_object(data,
			MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70, 0, 0);
		mxt_command_calibration(data);
	}
}

static void mxt_treat_T57_object(struct mxt_data *data,
						struct mxt_message *message)
{
	u16 tch_area;
	u16 atch_area;
	u16 sum_size;
	u8 *msg = message->message;
	static u8 case3_cnt;
	static u8 case4_cnt;
	static u8 calgood2_cnt;

	sum_size = (u16)(msg[0] | (msg[1] << 8));
	tch_area = (u16)(msg[2] | (msg[3] << 8));
	atch_area = (u16)(msg[4] | (msg[5] << 8));

	data->defend_sum_size = sum_size;
	data->defend_atch = atch_area;
	data->defend_tch = tch_area;

	if (data->finger_cnt < 0) /* In case of 0xE0 (Detect|Press|Release) */
		data->finger_cnt = 0;

	if (data->atch.coin) { /* 1st Stage Check */
		__mxt_debug_msg(data,
				"ATCH: COIN TCHAREA=%d ATCHAREA=%d SUM=%d\n",
				tch_area, atch_area, sum_size);
		if (tch_area) {
			if (data->atch.timer == 0) {
				mxt_set_T61_object(data,
						data->atch.timer_id,
						MXT_T61_TIMER_ONESHOT,
						MXT_T61_TIMER_CMD_START,
						data->atch.coin_time*1000);
				data->atch.timer = 1;
				if (!data->atch.prev_chargin_status) {
					mxt_write_object(data,
						MXT_GEN_POWERCONFIG_T7, 1, 12);
					mxt_write_object(data,
						MXT_SPT_CTECONFIG_T46, 2, 8);
					mxt_write_object(data,
						MXT_SPT_CTECONFIG_T46, 3, 10);
				}
			}

			if (tch_area < data->atch.autocal_size) {
				mxt_set_T8_object_autocal(data,
						data->atch.autocal_time);
				data->atch.autocal = 1;
			}

			if ((atch_area > data->atch.coin_atch_min) &&
							(sum_size < 30)) {
				if ((atch_area - tch_area) >
						data->atch.coin_atch_ratio) {
					__mxt_debug_msg(data,
							"ATCH: CASE 1-1\n");
					mxt_check_command_calibration(data);
				}
			}
		} else if (atch_area) { /* Only Anti-touch */
			__mxt_debug_msg(data, "ATCH: CASE 1-2\n");
			mxt_check_command_calibration(data);
		}
		case3_cnt = 0;
		case4_cnt = 0;
		calgood2_cnt = 0;
	}

	if (data->atch.calgood) { /* 2nd Stage Check */
		__mxt_debug_msg(data,
				"ATCH: CALGOOD TCHAREA=%d ATCHAREA=%d SUM=%d\n",
				tch_area, atch_area, sum_size);
		if (tch_area) {
			if (((atch_area - tch_area) > data->atch.big_atch_ratio)
				&& (tch_area < data->atch.big_atch_tch_max)) {
				__mxt_debug_msg(data, "ATCH: CASE 2-1\n");
				mxt_check_command_calibration(data);
			}

			if (((tch_area - atch_area) > data->atch.big_tch_ratio)
				&& (atch_area > data->atch.big_tch_atch_min)) {
				__mxt_debug_msg(data, "ATCH: CASE 2-2\n");
				mxt_check_command_calibration(data);
			}
			if ((sum_size > 200) && (calgood2_cnt++ > 10)) {
				__mxt_debug_msg(data, "ATCH: CASE 2-4\n");
				mxt_check_command_calibration(data);
			}
			if (data->atch.prev_chargin_status == 0) {
				if (data->t9_size < 25 && data->t9_amp > 30 &&
						sum_size > 30 &&
						data->finger_cnt < 7) {
					__mxt_debug_msg(data,
							"ATCH: CASE 2-5\n");
					mxt_check_command_calibration(data);
				}
				if (data->t9_size < 16 && data->t9_amp > 25 &&
					sum_size > 20 && data->finger_cnt < 5) {
					__mxt_debug_msg(data,
							"ATCH: CASE 2-6\n");
					mxt_check_command_calibration(data);
				}
				if (data->t9_size < 10 && data->t9_amp > 55 &&
					sum_size < 40 && (case4_cnt++ > 3)) {
					__mxt_debug_msg(data,
							"ATCH: CASE 2-7\n");
					mxt_check_command_calibration(data);
				}
			}
		} else if (atch_area) { /* Only Anti-touch */
			__mxt_debug_msg(data, "ATCH: CASE 2-3\n");
			mxt_check_command_calibration(data);
		}
	}

	if (data->atch.abnormal_enable && data->finger_cnt) {
		if (data->t9_size == 0 && data->t9_amp > 10 && atch_area > 30 &&
				data->finger_cnt < 6 && (case3_cnt++ > 4)) {
			__mxt_debug_msg(data, "Abnormal Case 3\n");
			mxt_check_command_calibration(data);
		}
		if (sum_size-tch_area > 90 && data->finger_cnt < 3) {
			__mxt_debug_msg(data, "Abnormal touch Case 5\n");
			mxt_check_command_calibration(data);
		}
		if (data->finger_cnt > 6 && sum_size < (data->finger_cnt * 2) &&
				sum_size < 20) {
			__mxt_debug_msg(data, "Abnormal touch Case 6\n");
			mxt_check_command_calibration(data);
		}
		if (data->atch.bigpalm_enable) {
			if ((sum_size - tch_area) > data->atch.gstylus_diff) {
				__mxt_debug_msg(data,
					"Abnormal Stylus Case 1\n");
				mxt_check_command_calibration(data);
			}
			if (data->t9_size >= 20 && data->t9_amp > 35 &&
					atch_area > 60) {
				__mxt_debug_msg(data, "Abnormal Case 1\n");
				mxt_check_command_calibration(data);
			}
			if (data->atch.prev_chargin_status == 0 &&
					data->t9_size < 16 &&
					data->t9_amp > 30 &&
					sum_size > 30 && data->finger_cnt < 7) {
				__mxt_debug_msg(data,
					"Abnormal Case 2 - finger_cnt = %d\n",
					data->finger_cnt);
				mxt_check_command_calibration(data);
			}
			if ((sum_size == tch_area) && (atch_area == 0) &&
					(data->finger_cnt > 5)) {
				__mxt_debug_msg(data,
					"Abnormal touch Case 7\n");
				mxt_check_command_calibration(data);
			}
		}
	}
}

#define ATCH_CALIBRATION_THRESHOLD	128
#define ATCH_CALIBRATION_RATIO		100

static void mxt_treat_T61_object(struct mxt_data *data,
						struct mxt_message *message)
{
	int id;
	u8 *msg = message->message;

	id = data->reportids[message->reportid].index;

	if ((id != data->atch.timer_id) || ((msg[0] & 0xa0) != 0xa0))
		return;

	data->atch.timer = 0;
	if (data->atch.bigpalm_enable) {
		dev_err(&data->client->dev, "BigPalm Case Disabled\n");
		data->atch.bigpalm_enable = 0;
		mxt_write_object(data, MXT_PROCI_STYLUS_T47, 1, 32);
		mxt_set_T8_object_autocal(data, 100);
	}
	if (data->atch.calgood) {
		dev_err(&data->client->dev, "ATCH: Calgood Timer elapsed\n");
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8,
						8, ATCH_CALIBRATION_THRESHOLD);
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8,
						9, ATCH_CALIBRATION_RATIO);
		data->atch.calgood = 0;

		dev_err(&data->client->dev, "Abnormal Case enabled\n");
		data->atch.abnormal_enable = 1;
		mxt_set_T61_object(data, data->atch.timer_id,
							MXT_T61_TIMER_ONESHOT,
							MXT_T61_TIMER_CMD_START,
							3 * 1000);
		data->atch.bigpalm_enable = 1;
		if (!data->atch.prev_chargin_status) {
			mxt_write_object(data, MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70, 0, 3);
			mxt_write_object(data, MXT_GEN_POWERCONFIG_T7, 1, 12);
			mxt_write_object(data, MXT_SPT_CTECONFIG_T46, 2, 8);
			mxt_write_object(data, MXT_SPT_CTECONFIG_T46, 3, 10);
			mxt_write_object(data, MXT_TOUCH_MULTITOUCHSCREEN_T9, 7, 45);
		}

		mxt_write_object(data, MXT_PROCI_LENSBENDING_T65, 0, 0);
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 2, 5);
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 3, 10);
		mxt_write_object(data, MXT_PROCI_STYLUS_T47, 1, 55);
	}
	if (data->atch.coin) {
		dev_err(&data->client->dev, "ATCH: Coin Timer elapsed\n");
		data->atch.coin = 0;
		mxt_set_T61_object(data, data->atch.timer_id,
						MXT_T61_TIMER_ONESHOT,
						MXT_T61_TIMER_CMD_START,
						data->atch.calgood_time * 1000);
		if (!data->atch.prev_chargin_status)
			mxt_write_object(data, MXT_GEN_POWERCONFIG_T7, 1, 12);
		data->atch.calgood = 1;
	}
}

static void mxt_treat_T62_object(struct mxt_data *data,
						struct mxt_message *message)
{
	int chargin_status;
	u8 *msg = message->message;

	if (msg[1] & 0x80) {
		chargin_status = 1;
		dev_dbg(&data->client->dev, "Charger Input pin ON\n");
	} else {
		chargin_status = 0;
		dev_dbg(&data->client->dev, "Charger Input pin OFF\n");
	}

	if (data->atch.prev_chargin_status != chargin_status) {
		if (chargin_status) {
			mxt_write_object(data, MXT_SPT_CTECONFIG_T46, 2, 16);
			mxt_write_object(data, MXT_SPT_CTECONFIG_T46, 3, 28);
			mxt_write_object(data,
				MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70 + 7,
				0, 0);
			mxt_write_object(data,
				MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70 + 7,
				1, 0);
			mxt_write_object(data, MXT_GEN_POWERCONFIG_T7, 1, 255);
			mxt_write_object(data,
			MXT_PROCG_NOISESUPPRESSION_T62, 3, 8);
			dev_info(&data->client->dev, "T46 TA config write\n");
		} else {
			mxt_write_object(data, MXT_SPT_CTECONFIG_T46, 2, 8);
			mxt_write_object(data, MXT_SPT_CTECONFIG_T46, 3, 10);
			mxt_write_object(data, 77, 0, 5);
			mxt_write_object(data,
				MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70 + 7,
				1, 7);
			mxt_write_object(data, MXT_GEN_POWERCONFIG_T7, 1, 12);
			mxt_write_object(data, MXT_PROCG_NOISESUPPRESSION_T62,
				3, 24);
			dev_dbg(&data->client->dev, "T46 BATT config write\n");
		}
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 8, 10);
		mxt_write_object(data, MXT_GEN_ACQUISITIONCONFIG_T8, 9, 1);
		mxt_write_object(data, MXT_TOUCH_MULTITOUCHSCREEN_T9, 8, 2);
		mxt_write_object(data, MXT_PROCI_STYLUS_T47, 8, 3);
		mxt_write_object(data, MXT_TOUCH_KEYARRAY_T15, 0, 135);
		data->atch.prev_chargin_status = chargin_status;
	}
}

/* Indicates that an event has occurred and has been handled. */
#define HANDLED				0x01

/* Indicates a single-edge triggered event or the rising edge of a double-edge
 * triggered event when set to 1, or indicates a falling edge of a double-edge
 * triggered event when set to 0
 */
#define EVENTEDGE			0x02

static void mxt_treat_T70_object(struct mxt_data *data,
						struct mxt_message *message)
{
	int id;
	u8 *msg = message->message;

	id = data->reportids[message->reportid].index;

	if (id == 0 && msg[0] == 0x3 && data->t70_touch_event++ > 100) {
		dev_info(&data->client->dev, "Abnormal T70 Touch Events\n");
		mxt_check_command_calibration(data);
		data->t70_touch_event = 0;
	}
}

/* Bootloader ID */
#define MXT_BOOT_EXTENDED_ID		0x20
#define MXT_BOOT_ID_MASK		0x1f

static int mxt_get_bootloader_version(struct i2c_client *client, u8 val)
{
	u8 buf[3];

	if (val & MXT_BOOT_EXTENDED_ID) {
		if (i2c_master_recv(client, buf, sizeof(buf)) != sizeof(buf)) {
			dev_err(&client->dev, "%s: i2c recv failed\n",
								__func__);
			return -EIO;
		}
		dev_info(&client->dev, "Bootloader ID:%d Version:%d",
								buf[1], buf[2]);
	} else
		dev_info(&client->dev, "Bootloader ID:%d",
							val & MXT_BOOT_ID_MASK);
	return 0;
}

static int mxt_check_bootloader(struct i2c_client *client, unsigned int state)
{
	u8 val;

recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
		if (mxt_get_bootloader_version(client, val))
			return -EIO;
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(&client->dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(&client->dev, "Invalid bootloader mode state 0x%X\n",
									val);
		return -EINVAL;
	}

	return 0;
}

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB		0xaa
#define MXT_UNLOCK_CMD_LSB		0xdc

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2] = {MXT_UNLOCK_CMD_LSB, MXT_UNLOCK_CMD_MSB};

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);

		return -EIO;
	}

	return 0;
}

static int mxt_probe_bootloader(struct i2c_client *client)
{
	u8 val;

	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	if (val & (~MXT_BOOT_STATUS_MASK)) {
		if (val & MXT_APP_CRC_FAIL)
			dev_err(&client->dev, "Application CRC failure\n");
		else
			dev_err(&client->dev, "Device in bootloader mode\n");
	} else {
		dev_err(&client->dev, "%s: Unknow status\n", __func__);
		return -EIO;
	}
	return 0;
}

static int mxt_fw_write(struct i2c_client *client, const u8 *frame_data,
								u16 frame_size)
{
	if (i2c_master_send(client, frame_data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

#define MXT_FW_MAGIC			0x4D3C2B1A

int mxt_verify_fw(struct mxt_data *data, struct mxt_fw_info *fw_info,
						const struct firmware *fw)
{
	struct device *dev = &data->client->dev;
	struct mxt_fw_image *fw_img;

	if (!fw) {
		dev_err(dev, "could not find firmware file\n");
		return -ENOENT;
	}

	fw_img = (struct mxt_fw_image *)fw->data;

	if (le32_to_cpu(fw_img->magic_code) != MXT_FW_MAGIC) {
		/* In case, firmware file only consist of firmware */
		dev_dbg(dev, "Firmware file only consist of raw firmware\n");
		fw_info->fw_len = fw->size;
		fw_info->fw_raw_data = fw->data;
	} else {
		/*
		 * In case, firmware file consist of header,
		 * configuration, firmware.
		 */
		dev_info(dev, "Firmware file consist of header, configuration, firmware\n");
		fw_info->fw_ver = fw_img->fw_ver;
		fw_info->build_ver = fw_img->build_ver;
		fw_info->hdr_len = le32_to_cpu(fw_img->hdr_len);
		fw_info->cfg_len = le32_to_cpu(fw_img->cfg_len);
		fw_info->fw_len = le32_to_cpu(fw_img->fw_len);
		fw_info->cfg_crc = le32_to_cpu(fw_img->cfg_crc);

		/* Check the firmware file with header */
		if (fw_info->hdr_len != sizeof(struct mxt_fw_image) ||
						fw_info->hdr_len +
						fw_info->cfg_len +
						fw_info->fw_len != fw->size) {
			dev_err(dev, "Firmware file is invaild!! hdr size[%d] cfg size[%d], fw size[%d] file size[%d]\n",
						fw_info->hdr_len,
						fw_info->cfg_len,
						fw_info->fw_len,
						fw->size);
			return -EINVAL;
		}

		if (!fw_info->cfg_len) {
			dev_err(dev, "Firmware file dose not include configuration data\n");
			return -EINVAL;
		}

		if (!fw_info->fw_len) {
			dev_err(dev, "Firmware file dose not include raw firmware data\n");
			return -EINVAL;
		}

		/* Get the address of configuration data */
		fw_info->cfg_raw_data = fw_img->data;

		/* Get the address of firmware data */
		fw_info->fw_raw_data = fw_img->data + fw_info->cfg_len;
	}

	return 0;
}

static int mxt_flash_fw(struct mxt_data *data, struct mxt_fw_info fw_info)
{
	struct i2c_client *client = data->client_boot;
	struct device *dev = &client->dev;
	u16 frame_size, prev_frame_size = 0;
	const u8 *fw_data = fw_info.fw_raw_data;
	const size_t fw_size = fw_info.fw_len;
	u32 pos = 0;
	int ret;

	if (!fw_data) {
		dev_err(dev, "firmware data is Null\n");
		return -ENOMEM;
	}

	ret = mxt_check_bootloader(client, MXT_WAITING_BOOTLOAD_CMD);
	if (ret) {
		/*may still be unlocked from previous update attempt */
		ret = mxt_check_bootloader(client, MXT_WAITING_FRAME_DATA);
		if (ret)
			goto out;
	} else {
		dev_info(&client->dev, "Unlocking bootloader\n");
		/* Unlock bootloader */
		mxt_unlock_bootloader(client);
	}

	while (pos < fw_size) {
		ret = mxt_check_bootloader(client, MXT_WAITING_FRAME_DATA);

		if (ret) {
			dev_err(dev, "Fail updating firmware. wating_frame_data err\n");
			goto out;
		}

		frame_size = ((*(fw_data + pos) << 8) | *(fw_data + pos + 1));

		/*
		 * We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */

		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(client, fw_data + pos, frame_size);

		ret = mxt_check_bootloader(client, MXT_FRAME_CRC_PASS);
		if (ret) {
			dev_err(dev, "Fail updating firmware. frame_crc err\n");
			goto out;
		}

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %d bytes\n", pos, fw_size);

		/* If frame size was over continuous 200 bytes, device need more
		 * 200 msec time to data writing.
		 * It is workaround codes, avoid i2c time-out at Intel based
		 * model.
		 */
		if (prev_frame_size > 200 && frame_size > 200)
			msleep(200);
		prev_frame_size = frame_size;
	}

	ret = mxt_wait_for_chg(data, MXT_SW_RESET_TIME);
	if (ret) {
		dev_err(dev, "Not respond after F/W  finish reset\n");
		goto out;
	}

	dev_info(dev, "success updating firmware\n");
out:
	return ret;
}

#define MXT_INFOMATION_BLOCK_SIZE	7

static int mxt_get_id_info(struct mxt_data *data)
{
	int ret = 0;
	u8 id[MXT_INFOMATION_BLOCK_SIZE];

	/* Read IC information */
	ret = mxt_read_mem(data, 0, MXT_INFOMATION_BLOCK_SIZE, id);
	if (ret) {
		dev_err(&data->client->dev, "Read fail. IC information\n");
		goto out;
	} else {
		dev_info(&data->client->dev,
			"family: 0x%x variant: 0x%x version: 0x%x"
			" build: 0x%x matrix X, Y size: %d, %d"
			" number of object: %d\n"
			, id[0], id[1], id[2], id[3], id[4], id[5], id[6]);

		data->id_info.family_id = id[0];
		data->id_info.variant_id = id[1];
		data->id_info.version = id[2];
		data->id_info.build = id[3];
		data->id_info.matrix_xsize = id[4];
		data->id_info.matrix_ysize = id[5];
		data->id_info.object_num = id[6];
	}

out:
	return ret;
}

static int mxt_get_object_table(struct mxt_data *data)
{
	int error;
	int i;
	u16 reg;
	u8 reportid = 0;
	u8 buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];

	for (i = 0; i < data->id_info.object_num; i++) {
		struct mxt_object *object = data->objects + i;

		reg = MXT_OBJECT_TABLE_START_ADDRESS +
				MXT_OBJECT_TABLE_ELEMENT_SIZE * i;
		error = mxt_read_mem(data, reg,
				MXT_OBJECT_TABLE_ELEMENT_SIZE, buf);
		if (error)
			return error;

		object->type = buf[0];
		object->start_address = (buf[2] << 8) | buf[1];
		/* the real size of object is buf[3]+1 */
		object->size = buf[3] + 1;
		/* the real instances of object is buf[4]+1 */
		object->instances = buf[4] + 1;
		object->num_report_ids = buf[5];

		dev_dbg(&data->client->dev,
			"Object:T%d\t\t\t Address:0x%x\tSize:%d\tInstance:%d\tReport Id's:%d\n",
			object->type, object->start_address, object->size,
			object->instances, object->num_report_ids);

		if (object->num_report_ids) {
			reportid += object->num_report_ids * object->instances;
			object->max_reportid = reportid;
		}
	}

	/* Store maximum reportid */
	data->max_reportid = reportid;
	dev_dbg(&data->client->dev, "maXTouch: %d report ID\n",
			data->max_reportid);

	return 0;
}

static void mxt_make_reportid_table(struct mxt_data *data)
{
	struct mxt_object *objects = data->objects;
	struct mxt_reportid *reportids = data->reportids;
	int i, j;
	int id = 0;

	for (i = 0; i < data->id_info.object_num; i++) {
		for (j = 0; j < objects[i].num_report_ids *
				objects[i].instances; j++) {
			id++;

			reportids[id].type = objects[i].type;
			reportids[id].index = j;

			dev_dbg(&data->client->dev, "Report_id[%d]:\tT%d\tIndex[%d]\n",
				id, reportids[id].type, reportids[id].index);
		}
	}
}

static int mxt_make_highchg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_message message;
	int count = data->max_reportid * 2;
	int error;

	/* Read dummy message to make high CHG pin */
	do {
		error = mxt_read_message(data, &message);
		if (error)
			return error;
	} while (message.reportid != 0xff && --count);

	if (!count) {
		dev_err(dev, "CHG pin isn't cleared\n");
		return -EBUSY;
	}

	return 0;
}

#define MXT_RESET_VALUE			0x01

static int mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	u32 read_info_crc, calc_info_crc;
	int ret;

	ret = mxt_get_id_info(data);
	if (ret)
		return ret;

	data->objects = kcalloc(data->id_info.object_num,
				sizeof(struct mxt_object),
				GFP_KERNEL);
	if (!data->objects) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Get object table infomation */
	ret = mxt_get_object_table(data);
	if (ret)
		goto out;

	data->reportids = kcalloc(data->max_reportid + 1,
						sizeof(struct mxt_reportid),
						GFP_KERNEL);
	if (!data->reportids) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Make report id table */
	mxt_make_reportid_table(data);

	/* Verify the info CRC */
	ret = mxt_read_info_crc(data, &read_info_crc);
	if (ret)
		goto out;

	ret = mxt_calculate_infoblock_crc(data, &calc_info_crc);
	if (ret)
		goto out;

	if (read_info_crc != calc_info_crc) {
		dev_err(&client->dev, "Infomation CRC error :[CRC 0x%06X!=0x%06X]\n",
				read_info_crc, calc_info_crc);
		ret = -EFAULT;
		goto out;
	}

	/* Restore memory and stop event handing */
	ret = mxt_command_backup(data, MXT_DISALEEVT_VALUE);
	if (ret) {
		dev_err(&client->dev, "Failed Restore NV and stop event\n");
		goto out;
	}

	/* Write config */
	ret = mxt_write_config(data);
	if (ret) {
		dev_err(&client->dev, "Failed to write config from file\n");
		goto out;
	}

	/* Backup to memory */
	ret = mxt_command_backup(data, MXT_BACKUP_VALUE);
	if (ret) {
		dev_err(&client->dev, "Failed backup NV data\n");
		goto out;
	}

	/* Soft reset */
	ret = mxt_command_reset(data, MXT_RESET_VALUE);
	if (ret) {
		dev_err(&client->dev, "Failed Reset IC\n");
		goto out;
	}

	/* Wait for CHG pin to high */
	ret = mxt_make_highchg(data);
	if (ret) {
		dev_err(&client->dev, "Failed to clear CHG pin\n");
		goto out;
	}

	mxt_atch_init(data);

	return 0;

out:
	return ret;
}

static int mxt_power_on(struct mxt_data *data)
{
	/*
	 * If do not turn off the power during suspend, you can use deep sleep
	 * or disable scan to use T7, T9 Object. But to turn on/off the power
	 * is better.
	 */
	int error = 0;

	if (data->mxt_enabled)
		return 0;

	if (!data->pdata->set_power) {
		dev_warn(&data->client->dev, "Power on function is not defined\n");
		error = -EINVAL;
		goto out;
	}

	data->pdata->set_power(true);
	error = mxt_wait_for_chg(data, MXT_HW_RESET_TIME);
	if (error)
		dev_err(&data->client->dev, "Not respond after H/W reset\n");

	data->mxt_enabled = true;
	dev_info(&data->client->dev, "Power on\n");
out:
	return error;
}

static int mxt_power_off(struct mxt_data *data)
{
	int error = 0;

	if (!data->mxt_enabled)
		return 0;

	if (!data->pdata->set_power) {
		dev_warn(&data->client->dev, "Power off function is not defined\n");
		error = -EINVAL;
		goto out;
	}

	data->pdata->set_power(false);
	data->mxt_enabled = false;
	dev_info(&data->client->dev, "Power off\n");
out:
	return error;
}

/* Need to be called by function that is blocked with mutex */
static int mxt_start(struct mxt_data *data)
{
	int error = 0;

	if (data->mxt_enabled) {
		dev_err(&data->client->dev,
			"%s. but touch already on\n", __func__);
		return error;
	}

	error = mxt_power_on(data);

	if (error)
		dev_err(&data->client->dev, "Fail to start touch\n");
	else
		enable_irq(data->client->irq);

	return error;
}

/* Need to be called by function that is blocked with mutex */
static int mxt_stop(struct mxt_data *data)
{
	int error = 0;

	if (!data->mxt_enabled) {
		dev_err(&data->client->dev,
			"%s. but touch already off\n", __func__);
		return error;
	}
	disable_irq(data->client->irq);

	error = mxt_power_off(data);
	if (error) {
		dev_err(&data->client->dev, "Fail to stop touch\n");
		goto err_power_off;
	}
	mxt_release_all_finger(data);
	mxt_release_all_keys(data);

	return 0;

err_power_off:
	enable_irq(data->client->irq);
	return error;
}

static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);
	int ret;

	ret = mxt_start(data);
	if (ret)
		goto err_open;

	dev_dbg(&data->client->dev, "%s\n", __func__);

	return 0;

err_open:
	return ret;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_stop(data);

	dev_dbg(&data->client->dev, "%s\n", __func__);
}

#define MXT_BOOT_VALUE			0xA5

static int mxt_enter_bootloader(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int error;

	data->objects = kcalloc(data->id_info.object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
	if (!data->objects) {
		dev_err(dev, "%s Failed to allocate memory\n", __func__);
		error = -ENOMEM;
		goto out;
	}

	/* Get object table information*/
	error = mxt_get_object_table(data);
	if (error)
		goto err_free_mem;

	/* Change to the bootloader mode */
	error = mxt_command_reset(data, MXT_BOOT_VALUE);
	if (error)
		goto err_free_mem;

err_free_mem:
	kfree(data->objects);
	data->objects = NULL;

out:
	return error;
}

enum {
	FORCE = 0,
	EXT_FILE,
	NORMAL,
};

static int mxt_fw_updater(struct mxt_data *data, u8 mode)
{
	struct i2c_client *client = data->client;
	struct device *dev = &data->client->dev;
	int ret = 0;
	const struct firmware *fw;

	ret = request_firmware(&fw, data->pdata->fw_name, &client->dev);
	if (ret) {
		dev_err(&client->dev, "error requesting built-in firmware\n");
		goto out;
	}

	if (mode == EXT_FILE) {
		struct firmware ext_fw;
		long fw_size;
		u8 *fw_data;
		struct file *filp;
		mm_segment_t oldfs;

		dev_info(dev, "fw_updater: force upload from external file.");

		oldfs = get_fs();
		set_fs(KERNEL_DS);

		filp = filp_open(data->pdata->ext_fw_name, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(filp)) {
			dev_err(dev, "file open error:%d\n", (s32)filp);
			ret = -ENOENT;
			goto out;
		}

		fw_size = filp->f_path.dentry->d_inode->i_size;
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		if (!fw_data) {
			dev_err(dev, "failed to allocation memory.");
			filp_close(filp, current->files);
			ret = -ENOMEM;
			goto out;
		}

		ret = vfs_read(filp, (char __user *)fw_data, fw_size,
								&filp->f_pos);
		if (ret != fw_size) {
			dev_err(dev, "failed to read file (ret = %d).", ret);
			filp_close(filp, current->files);
			kfree(fw_data);
			goto out;
		}

		filp_close(filp, current->files);
		set_fs(oldfs);

		ext_fw.size = fw_size;
		ext_fw.data = fw_data;

		ret = mxt_verify_fw(data, &data->fw_info, &ext_fw);
		if (ret)
			goto out;

		ret = mxt_enter_bootloader(data);
		if (ret) {
			dev_err(dev, "Failed updating firmware\n");
			goto out;
		}

		ret = mxt_flash_fw(data, data->fw_info);
		if (ret)
			dev_err(dev, "Failed updating firmware\n");
		else
			dev_info(dev, "succeeded updating firmware\n");

		kfree(fw_data);

		ret = mxt_initialize(data);
		if (ret) {
			dev_err(&client->dev, "Failed to initialize\n");
			goto out;
		}

		return ret;
	} else {
		ret = mxt_verify_fw(data, &data->fw_info, fw);
		if (ret)
			return ret;

		/* Skip update on boot up if firmware file does not have a header */
		if (!data->fw_info.hdr_len)
			return ret;

		ret = mxt_get_id_info(data);
		if (ret) {
			/* need to check IC is in boot mode */
			ret = mxt_probe_bootloader(data->client_boot);
			if (ret) {
				dev_err(dev, "Failed to verify bootloader's status\n");
				goto out;
			}
			dev_info(dev, "Updating firmware from boot-mode\n");
			goto write_fw;
		}

		memcpy(&data->bin_fw_info, &data->fw_info,
						sizeof(struct mxt_fw_info));

		/* compare the version to verify necessity of firmware updating */
		if ((data->id_info.version == data->fw_info.fw_ver
			&& data->id_info.build == data->fw_info.build_ver)
				&& mode != FORCE) {
			dev_dbg(dev, "Firmware version is same with in IC\n");
			goto out;
		}
	}
	dev_info(dev, "Updating firmware from app-mode : IC:0x%x,0x%x =! FW:0x%x,0x%x\n",
						data->id_info.version,
						data->id_info.build,
						data->fw_info.fw_ver,
						data->fw_info.build_ver);
	ret = mxt_enter_bootloader(data);
	if (ret) {
		dev_err(dev, "Failed updating firmware\n");
		goto out;
	}

write_fw:
	ret = mxt_flash_fw(data, data->fw_info);
	if (ret)
		dev_err(dev, "Failed updating firmware\n");
	else
		dev_info(dev, "succeeded updating firmware\n");

	ret = mxt_initialize(data);
	if (ret) {
		dev_err(&client->dev, "Failed to initialize\n");
		goto out;
	}
out:
	release_firmware(fw);
	return ret;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
#define mxt_suspend	NULL
#define mxt_resume	NULL

static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data, early_suspend);
	mutex_lock(&data->input_dev->mutex);

	mxt_stop(data);

	mutex_unlock(&data->input_dev->mutex);
	dev_info(&data->client->dev, "early_suspend done\n");
}

static void mxt_late_resume(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data, early_suspend);
	mutex_lock(&data->input_dev->mutex);

	mxt_start(data);

	mutex_unlock(&data->input_dev->mutex);
	dev_info(&data->client->dev, "late_resume done\n");
}
#else
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->input_dev->mutex);

	mxt_stop(data);

	mutex_unlock(&data->input_dev->mutex);
	return 0;
}

static int mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->input_dev->mutex);

	mxt_start(data);

	mutex_unlock(&data->input_dev->mutex);
	return 0;
}
#endif

static irqreturn_t mxt_irq_thread(int irq, void *ptr)
{
	struct mxt_data *data = ptr;
	struct mxt_message message;
	struct device *dev = &data->client->dev;
	u8 type;

	do {
		if (unlikely(mxt_read_message(data, &message))) {
			dev_err(dev, "Failed to read message\n");
			goto end;
		}
#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
		if (data->atmeldbg.display_log)
			print_hex_dump(KERN_INFO, "MXT MSG:", DUMP_PREFIX_NONE,
				16, 1, &message, sizeof(struct mxt_message),
				false);
#endif
		if (message.reportid > data->max_reportid)
			goto end;

		type = data->reportids[message.reportid].type;

		switch (type) {
		case MXT_RESERVED_T0:
			goto end;
			break;
		case MXT_GEN_COMMANDPROCESSOR_T6:
			mxt_treat_T6_object(data, &message);
			break;
		case MXT_TOUCH_MULTITOUCHSCREEN_T9:
			mxt_treat_T9_object(data, &message);
			break;
		case MXT_TOUCH_KEYARRAY_T15:
			mxt_treat_T15_object(data, &message);
			break;
		case MXT_SPT_SELFTEST_T25:
			dev_err(dev, "Self test fail [0x%x 0x%x 0x%x 0x%x]\n",
				message.message[0], message.message[1],
				message.message[2], message.message[3]);
			break;
		case MXT_PROCI_TOUCHSUPPRESSION_T42:
			mxt_treat_T42_object(data, &message);
			break;
		case MXT_PROCI_EXTRATOUCHSCREENDATA_T57:
			mxt_treat_T57_object(data, &message);
			break;
		case MXT_SPT_TIMER_T61: /* TSP_CHECK_ANTITOUCH */
			mxt_treat_T61_object(data, &message);
			break;
		case MXT_PROCG_NOISESUPPRESSION_T62: /* detect TA state */
			mxt_treat_T62_object(data, &message);
			break;
		case MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70:
			mxt_treat_T70_object(data, &message);
			break;
		default:
			dev_dbg(dev, "Untreated Object type[%d]\tmessage[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
				type, message.message[0],
				message.message[1], message.message[2],
				message.message[3], message.message[4],
				message.message[5], message.message[6]);
			break;
		}
	} while (!gpio_get_value(data->pdata->gpio_irq));

	if (data->finger_mask)
		mxt_report_input_data(data);

end:
	return IRQ_HANDLED;
}

#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
static int mxt_atmeldbg_read_block(struct i2c_client *client, u16 addr,
							u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16 le_addr;
	struct mxt_data *data = i2c_get_clientdata(client);
	struct mxt_object *object;

	if (data == NULL) {
		dev_err(&client->dev, "%s Data is Null\n", __func__);
		return -EINVAL;
	}

	object = mxt_get_object(data, MXT_GEN_MESSAGEPROCESSOR_T5);

	if ((data->atmeldbg.last_read_addr == addr) &&
					(addr == object->start_address)) {
		if  (i2c_master_recv(client, value, length) == length) {
			if (data->atmeldbg.display_log)
				print_hex_dump(KERN_INFO, "MXT RX:",
					DUMP_PREFIX_NONE, 16, 1,
					value, length, false);
			return 0;
		} else
			return -EIO;
	} else {
		data->atmeldbg.last_read_addr = addr;
	}

	le_addr = cpu_to_le16(addr);
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;

	if (i2c_transfer(adapter, msg, 2) == 2) {
		if (data->atmeldbg.display_log) {
			print_hex_dump(KERN_INFO, "MXT TX:", DUMP_PREFIX_NONE,
				16, 1, msg[0].buf, msg[0].len, false);
			print_hex_dump(KERN_INFO, "MXT RX:", DUMP_PREFIX_NONE,
				16, 1, msg[1].buf, msg[1].len, false);
		}
		return 0;
	} else
		return -EIO;
}

/* Writes a block of bytes (max 256) to given address in mXT chip. */
static int mxt_atmeldbg_write_block(struct i2c_client *client, u16 addr,
							u16 length, u8 *value)
{
	int i;
	struct {
		__le16	le_addr;
		u8	data[256];
	} i2c_block_transfer;

	struct mxt_data *data = i2c_get_clientdata(client);

	if (length > 256)
		return -EINVAL;

	if (data == NULL) {
		dev_err(&client->dev, "%s Data is Null\n", __func__);
		return -EINVAL;
	}

	data->atmeldbg.last_read_addr = -1;

	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;

	i2c_block_transfer.le_addr = cpu_to_le16(addr);

	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);

	if (i == (length + 2)) {
		if (data->atmeldbg.display_log)
			print_hex_dump(KERN_INFO, "MXT TX:", DUMP_PREFIX_NONE,
					16, 1, &i2c_block_transfer, length+2,
					false);
		return length;
	} else
		return -EIO;
}

static ssize_t mem_atmeldbg_access_read(struct file *filp,
			struct kobject *kobj, struct bin_attribute *bin_attr,
			char *buf, loff_t off, size_t count)
{
	int ret = 0;
	struct i2c_client *client =
			to_i2c_client(container_of(kobj, struct device, kobj));

	dev_info(&client->dev, "%s p=%p off=%lli c=%zi\n",
						__func__, buf, off, count);

	if (off >= 32768)
		return -EIO;

	if (off + count > 32768)
		count = 32768 - off;

	if (count > 256)
		count = 256;

	if (count > 0)
		ret = mxt_atmeldbg_read_block(client, off, count, buf);

	return ret >= 0 ? count : ret;
}

static ssize_t mem_atmeldbg_access_write(struct file *filp,
			struct kobject *kobj, struct bin_attribute *bin_attr,
			char *buf, loff_t off, size_t count)
{
	int ret = 0;
	struct i2c_client *client =
			to_i2c_client(container_of(kobj, struct device, kobj));

	dev_info(&client->dev, "%s p=%p off=%lli c=%zi\n",
						__func__, buf, off, count);

	if (off >= 32768)
		return -EIO;

	if (off + count > 32768)
		count = 32768 - off;

	if (count > 256)
		count = 256;

	if (count > 0)
		ret = mxt_atmeldbg_write_block(client, off, count, buf);

	return ret >= 0 ? count : 0;
}

static ssize_t mxt_atmeldbg_pause_driver_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int count = 0;

	count += sprintf(buf + count, "%d", data->atmeldbg.stop_sync);
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t mxt_atmeldbg_pause_driver_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->atmeldbg.stop_sync = i;

		dev_info(&client->dev, "%s input sync for Atmel dbug App\n",
			i ? "Stop" : "Start");
	} else {
		dev_info(&client->dev, "Error %s\n", __func__);
	}

	return count;
}

static ssize_t mxt_atmeldbg_debug_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int count = 0;

	count += sprintf(buf + count, "%d", data->atmeldbg.display_log);
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t mxt_atmeldbg_debug_enable_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->atmeldbg.display_log = i;
		if (i)
			dmesg_restrict = 0;
		else
			dmesg_restrict = 1;

		dev_info(&client->dev, "%s, dmsesg_restricted :%d\n",
			i ? "debug enabled" : "debug disabled", dmesg_restrict);
		dev_info(&client->dev, "%s raw data message for Atmel debug App\n",
			i ? "Display" : "Undisplay");
	} else
		dev_info(&client->dev, "Error %s\n", __func__);

	return count;
}

static ssize_t mxt_atmeldbg_command_off_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->atmeldbg.block_access = i;

		if (data->atmeldbg.block_access)
			disable_irq(client->irq);
		else
			enable_irq(client->irq);

		dev_info(&client->dev, "%s host driver access IC for Atmel dbug App\n",
				i ? "Stop" : "Start");
	} else
		dev_info(&client->dev, "Error %s\n", __func__);

	return count;
}

static ssize_t mxt_atmeldbg_cmd_calibrate_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret;

	/* send calibration command to the chip */
	ret = mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
					MXT_COMMAND_CALIBRATE, 1);

	return (ret < 0) ? ret : count;
}

static ssize_t mxt_atmeldbg_cmd_reset_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret;

	/* send reset command to the chip */
	ret = mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
					MXT_COMMAND_RESET, MXT_RESET_VALUE);

	return (ret < 0) ? ret : count;
}

static ssize_t mxt_atmeldbg_cmd_backup_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret;

	/* send backup command to the chip */
	ret = mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
					MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);

	return (ret < 0) ? ret : count;
}

static ssize_t mxt_object_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	unsigned int type;
	unsigned int offset;
	unsigned int value;
	u8 val;
	int ret;
	sscanf(buf, "%u%u%u", &type, &offset, &value);
	dev_info(&client->dev, "object type T%d", type);
	dev_info(&client->dev, "data offset %d\n", offset);
	dev_info(&client->dev, "data value %d\n", value);

	ret = mxt_write_object(data, (u8)type, (u8)offset, (u8)value);

	if (ret) {
		dev_err(&client->dev, "fail to write T%d index:%d, value:%d\n",
			type, offset, value);
		goto out;
	} else {
		ret = mxt_read_object(data, (u8)type, (u8)offset, &val);

		if (ret) {
			dev_err(&client->dev, "fail to read T%d\n", type);
			goto out;
		} else
			dev_info(&client->dev, "T%d Byte%d is %d\n", type,
								offset, val);
	}
out:
	return count;
}

static ssize_t mxt_object_show(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	struct mxt_object *object;
	unsigned int type;
	u8 val;
	u16 i;

	sscanf(buf, "%u", &type);
	dev_info(&client->dev, "object type T%d\n", type);

	object = mxt_get_object(data, type);
	if (!object) {
		dev_err(&client->dev, "fail to get object_info\n");
		return -EINVAL;
	} else {
		for (i = 0; i < object->size; i++) {
			mxt_read_mem(data, object->start_address + i, 1, &val);
			dev_info(&client->dev, "Byte %u --> %u\n", i, val);
		}
	}

	return count;
}

/* Sysfs files for libmaxtouch interface */
static DEVICE_ATTR(pause_driver, S_IRUGO | S_IWUSR | S_IWGRP,
	mxt_atmeldbg_pause_driver_show, mxt_atmeldbg_pause_driver_store);
static DEVICE_ATTR(debug_enable, S_IRUGO | S_IWUSR | S_IWGRP,
	mxt_atmeldbg_debug_enable_show, mxt_atmeldbg_debug_enable_store);
static DEVICE_ATTR(command_calibrate, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, mxt_atmeldbg_cmd_calibrate_store);
static DEVICE_ATTR(command_reset, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, mxt_atmeldbg_cmd_reset_store);
static DEVICE_ATTR(command_backup, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, mxt_atmeldbg_cmd_backup_store);
static DEVICE_ATTR(command_off, S_IRUGO | S_IWUSR | S_IWGRP,
	NULL, mxt_atmeldbg_command_off_store);
static DEVICE_ATTR(object_show, S_IWUSR | S_IWGRP, NULL, mxt_object_show);
static DEVICE_ATTR(object_write, S_IWUSR | S_IWGRP, NULL, mxt_object_setting);

static struct attribute *libmaxtouch_attributes[] = {
	&dev_attr_pause_driver.attr,
	&dev_attr_debug_enable.attr,
	&dev_attr_command_calibrate.attr,
	&dev_attr_command_reset.attr,
	&dev_attr_command_backup.attr,
	&dev_attr_command_off.attr,
	&dev_attr_object_show.attr,
	&dev_attr_object_write.attr,
	NULL,
};

static struct attribute_group libmaxtouch_attr_group = {
	.attrs = libmaxtouch_attributes,
};

/* Functions for mem_access interface */
static struct bin_attribute mem_access_attr;

static int  __devinit mxt_debug_init(struct mxt_data *data)
{
	int error;

	error = sysfs_create_group(&data->client->dev.kobj,
						&libmaxtouch_attr_group);
	if (error) {
		dev_err(&data->client->dev, "Failed to create libmaxtouch sysfs group\n");
		return -EINVAL;
	}

	sysfs_bin_attr_init(&mem_access_attr);
	mem_access_attr.attr.name = "mem_access";
	mem_access_attr.attr.mode = S_IRUGO | S_IWUGO;
	mem_access_attr.read = mem_atmeldbg_access_read;
	mem_access_attr.write = mem_atmeldbg_access_write;
	mem_access_attr.size = 65535;
	data->atmeldbg.display_log = 0;

	if (sysfs_create_bin_file(&data->client->dev.kobj, &mem_access_attr)
									< 0) {
		dev_err(&data->client->dev, "Failed to create device file(%s)!\n",
				mem_access_attr.attr.name);
		error = -EINVAL;
		goto err_sysfs_atmeldbg;
	}

	return 0;

err_sysfs_atmeldbg:
	sysfs_remove_bin_file(&data->client->dev.kobj, &mem_access_attr);
	sysfs_remove_group(&data->client->dev.kobj, &libmaxtouch_attr_group);

	return error;
}

static void  mxt_debug_remove(struct mxt_data *data)
{
	sysfs_remove_group(&data->client->dev.kobj, &libmaxtouch_attr_group);
	sysfs_remove_bin_file(&data->client->dev.kobj, &mem_access_attr);
}
#endif /* end of defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG) */

#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)

/* Diagnostic command defines  */
#define MXT_DIAG_PAGE_UP		0x01
#define MXT_DIAG_DELTA_MODE		0x10
#define MXT_DIAG_REFERENCE_MODE		0x11
#define MXT_DIAG_CTE_MODE		0x31
#define MXT_DIAG_MODE_MASK		0xFC
#define MXT_DIAGNOSTIC_MODE		0
#define MXT_DIAGNOSTIC_PAGE		1

#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func
#define TOSTRING(x) #x

enum {	/* this is using by cmd_state valiable. */
	WAITING = 0,
	RUNNING,
	OK,
	FAIL,
	NOT_APPLICABLE,
};

struct tsp_cmd {
	struct list_head		list;
	const char			*cmd_name;
	void				(*cmd_func)(void *device_data);
};

static void set_default_result(struct factory_data *data)
{
	char delim = ':';

	memset(data->cmd_result, 0x00, ARRAY_SIZE(data->cmd_result));
	memset(data->cmd_buff, 0x00, ARRAY_SIZE(data->cmd_buff));
	memcpy(data->cmd_result, data->cmd, strlen(data->cmd));
	strncat(data->cmd_result, &delim, 1);
}

static void set_cmd_result(struct factory_data *data, char *buff, int len)
{
	strncat(data->cmd_result, buff, len);
}

static void not_support_cmd(void *device_data)
{
	struct factory_data *data =
				((struct mxt_data *)device_data)->factory_data;
	set_default_result(data);
	sprintf(data->cmd_buff, "%s", "NA");
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = NOT_APPLICABLE;
	pr_info("tsp factory: %s: \"%s(%d)\"\n", __func__,
			data->cmd_buff, strlen(data->cmd_buff));
	return;
}

static bool mxt_check_xy_range(struct mxt_data *data, u16 node)
{
	u8 x_line = node / data->id_info.matrix_ysize;
	u8 y_line = node % data->id_info.matrix_ysize;
	return (y_line < data->pdata->y_channel_no) ?
		(x_line < data->pdata->x_channel_no) : false;
}

/* +  Vendor specific helper functions */
static int mxt_xy_to_node(struct mxt_data *ts_data)
{
	struct factory_data *data = ts_data->factory_data;
	int node;

	/* cmd_param[0][1] : [x][y] */
	if (data->cmd_param[0] < 0
		|| data->cmd_param[0] >= ts_data->pdata->x_channel_no
		|| data->cmd_param[1] < 0
		|| data->cmd_param[1] >= ts_data->pdata->y_channel_no) {
		sprintf(data->cmd_buff, "%s", "NG");
		set_cmd_result(data, data->cmd_buff,
					strlen(data->cmd_buff));
		data->cmd_state = FAIL;
		return -EINVAL;
	}
	/*
	 * maybe need to consider orient.
	 *   --> y number
	 *  |(0,0) (0,1)
	 *  |(1,0) (1,1)
	 *  v
	 *  x number
	 */
	node = data->cmd_param[0] * ts_data->pdata->y_channel_no
			+ data->cmd_param[1];

	return node;
}

static void mxt_node_to_xy(struct mxt_data *ts_data, u16 *x, u16 *y)
{
	*x = ts_data->node_data->delta_max_node / ts_data->pdata->y_channel_no;
	*y = ts_data->node_data->delta_max_node % ts_data->pdata->y_channel_no;
}

static int mxt_set_diagnostic_mode(struct mxt_data *data, u8 dbg_mode)
{
	struct i2c_client *client = data->client;
	u8 cur_mode;
	int ret;

	ret = mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,
			MXT_COMMAND_DIAGNOSTIC, dbg_mode);

	if (ret) {
		dev_err(&client->dev, "Failed change diagnositc mode to %d\n",
				dbg_mode);
		return ret;
	}

	if (dbg_mode & MXT_DIAG_MODE_MASK) {
		do {
			ret = mxt_read_object(data, MXT_DEBUG_DIAGNOSTIC_T37,
					MXT_DIAGNOSTIC_MODE, &cur_mode);
			if (ret) {
				dev_err(&client->dev,
						"Failed getting diagnositc mode\n");
				return ret;
			}
			usleep_range(3000, 4000);
		} while (cur_mode != dbg_mode);
		dev_dbg(&client->dev, "current dianostic chip mode is %d\n",
				cur_mode);
	}

	return ret;
}

#define DATA_PER_NODE		2

const u16 REF_MIN_VAL_MXT1188S	= 3366;
const u16 REF_MAX_VAL_MXT1188S	= 14616;
const u16 REF_OFFSET_VALUE	= 16384;

static void mxt_treat_dbg_data(struct mxt_data *ts_data,
	struct mxt_object *dbg_object, u8 dbg_mode, u8 read_point, u16 num)
{
	struct i2c_client *client = ts_data->client;
	u8 data_buffer[DATA_PER_NODE] = {0, };
	int xnode = num / ts_data->pdata->y_channel_no;
	int ynode = num % ts_data->pdata->y_channel_no;

	if (dbg_mode == MXT_DIAG_DELTA_MODE) {
		/* read delta data */
		mxt_read_mem(ts_data, dbg_object->start_address + read_point,
			DATA_PER_NODE, data_buffer);

		ts_data->node_data->delta_data[num] =
			((u16)data_buffer[1]<<8) + (u16)data_buffer[0];

		dev_dbg(&client->dev, "delta[%d] = %d\n", num,
					ts_data->node_data->delta_data[num]);

		if (abs(ts_data->node_data->delta_data[num]) >
				abs(ts_data->node_data->delta_max_data)) {
			ts_data->node_data->delta_max_node = num;
			ts_data->node_data->delta_max_data =
					ts_data->node_data->delta_data[num];
		}
	} else if (dbg_mode == MXT_DIAG_REFERENCE_MODE) {
		/* read reference data */
		mxt_read_mem(ts_data, dbg_object->start_address + read_point,
			DATA_PER_NODE, data_buffer);

		ts_data->node_data->reference_data[num] =
			((u16)data_buffer[1] << 8) + (u16)data_buffer[0]
			- REF_OFFSET_VALUE;

		/* check that reference is in spec or not */
		if ((ts_data->node_data->reference_data[num] < REF_MIN_VAL_MXT1188S)
			|| (ts_data->node_data->reference_data[num]
			> REF_MAX_VAL_MXT1188S)) {
			dev_err(&client->dev,
				"reference[%d] is out of range = %d(%d,%d)\n",
				num, ts_data->node_data->reference_data[num],
				xnode, ynode);
		}

		if (ts_data->node_data->reference_data[num]
				> ts_data->node_data->ref_max_data)
			ts_data->node_data->ref_max_data =
				ts_data->node_data->reference_data[num];
		if (ts_data->node_data->reference_data[num]
				< ts_data->node_data->ref_min_data)
			ts_data->node_data->ref_min_data =
				ts_data->node_data->reference_data[num];
		dev_dbg(&client->dev, "reference[%d] = %d\n",
				num, ts_data->node_data->reference_data[num]);
	}
}

#define NODE_PER_PAGE		64

static int mxt_read_all_diagnostic_data(struct mxt_data *ts_data, u8 dbg_mode)
{
	struct i2c_client *client = ts_data->client;
	struct mxt_object *dbg_object;
	u8 read_page, cur_page, end_page, read_point;
	u16 node, num = 0, cnt = 0;
	int ret = 0;

	/* to make the Page Num to 0 */
	ret = mxt_set_diagnostic_mode(ts_data, MXT_DIAG_CTE_MODE);
	if (ret)
		return ret;

	/* change the debug mode */
	ret = mxt_set_diagnostic_mode(ts_data, dbg_mode);
	if (ret)
		return ret;

	/* get object info for diagnostic */
	dbg_object = mxt_get_object(ts_data, MXT_DEBUG_DIAGNOSTIC_T37);
	if (!dbg_object) {
		dev_err(&client->dev, "fail to get object_info\n");
		return -EINVAL;
	}

	ts_data->node_data->ref_min_data = REF_MAX_VAL_MXT1188S;
	ts_data->node_data->ref_max_data = REF_MIN_VAL_MXT1188S;
	ts_data->node_data->delta_max_data = 0;
	ts_data->node_data->delta_max_node = 0;
	end_page = (ts_data->id_info.matrix_xsize * \
			ts_data->id_info.matrix_ysize) / NODE_PER_PAGE;

	/* read the dbg data */
	for (read_page = 0 ; read_page < end_page; read_page++) {
		for (node = 0; node < NODE_PER_PAGE; node++, cnt++) {
			read_point = (node * DATA_PER_NODE) + 2;

			if (mxt_check_xy_range(ts_data, cnt)) {
				mxt_treat_dbg_data(ts_data, dbg_object,
						dbg_mode, read_point, num);
				num++;
			}
		}
		ret = mxt_set_diagnostic_mode(ts_data, MXT_DIAG_PAGE_UP);
		if (ret)
			return ret;
		do {
			msleep(20);
			ret = mxt_read_mem(ts_data, dbg_object->start_address
							+ MXT_DIAGNOSTIC_PAGE,
							1, &cur_page);
			if (ret) {
				dev_err(&client->dev, "%s Read fail page\n",
								__func__);
				return ret;
			}
		} while (cur_page != read_page + 1);
	}
	return ret;
}

static int mxt_read_diagnostic_data(struct mxt_data *data, u8 dbg_mode,
							u16 node, u16 *dbg_data)
{
	struct i2c_client *client = data->client;
	struct mxt_object *dbg_object;
	u8 read_page, read_point;
	u8 cur_page, cnt_page;
	u8 data_buf[DATA_PER_NODE] = { 0 };
	int ret = 0;

	/* calculate the read page and point */
	read_page = node / NODE_PER_PAGE;
	node %= NODE_PER_PAGE;
	read_point = (u8)((node * DATA_PER_NODE) + 2);

	/* to make the Page Num to 0 */
	ret = mxt_set_diagnostic_mode(data, MXT_DIAG_CTE_MODE);
	if (ret)
		goto out;

	/* change the debug mode */
	ret = mxt_set_diagnostic_mode(data, dbg_mode);
	if (ret)
		goto out;

	/* get object info for diagnostic */
	dbg_object = mxt_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
	if (!dbg_object) {
		dev_err(&client->dev, "fail to get object_info\n");
		ret = -EINVAL;
		goto out;
	}

	/* move to the proper page */
	for (cnt_page = 1; cnt_page <= read_page; cnt_page++) {
		ret = mxt_set_diagnostic_mode(data, MXT_DIAG_PAGE_UP);
		if (ret)
			goto out;
		do {
			msleep(20);
			ret = mxt_read_mem(data,
				dbg_object->start_address + MXT_DIAGNOSTIC_PAGE,
				1, &cur_page);
			if (ret) {
				dev_err(&client->dev, "%s Read fail page\n",
					__func__);
				goto out;
			}
		} while (cur_page != cnt_page);
	}

	/* read the dbg data */
	ret = mxt_read_mem(data, dbg_object->start_address + read_point,
		DATA_PER_NODE, data_buf);
	if (ret)
		goto out;

	*dbg_data = ((u16)data_buf[1] << 8) + (u16)data_buf[0];

	dev_dbg(&client->dev, "dbg_mode[%d]: dbg data[%d] = %d\n",
		dbg_mode, (read_page * NODE_PER_PAGE) + node,
		dbg_mode == MXT_DIAG_DELTA_MODE ? (s16)(*dbg_data) : *dbg_data);
out:
	return ret;
}

static void fw_update(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);
	disable_irq(ts_data->client->irq);

	if (data->cmd_param[0] == 1) {
		if (mxt_fw_updater(ts_data, EXT_FILE))
			data->cmd_state = FAIL;
		else
			data->cmd_state = OK;
	} else if (data->cmd_param[0] == 0) {
		if (mxt_fw_updater(ts_data, FORCE))
			data->cmd_state = FAIL;
		else
			data->cmd_state = OK;
	} else
		data->cmd_state = FAIL;

	enable_irq(ts_data->client->irq);

	if (data->cmd_state == OK)
		sprintf(data->cmd_buff, "%s", "OK");
	else
		sprintf(data->cmd_buff, "%s", "FAIL");

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);

	sprintf(data->cmd_buff, "AT00%.2X%.2X",
				ts_data->bin_fw_info.fw_ver,
				ts_data->bin_fw_info.build_ver);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = OK;

	return;
}

static void get_fw_ver_ic(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);
	sprintf(data->cmd_buff, "AT00%.2X%.2X", ts_data->id_info.version,
							ts_data->id_info.build);

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = OK;
	return;
}

static void get_config_ver(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;
	struct mxt_object *user_object;

	data->cmd_state = RUNNING;

	set_default_result(data);

	user_object = mxt_get_object(ts_data, MXT_SPT_USERDATA_T38);
	if (!user_object) {
		sprintf(data->cmd_buff, "%s", "NG");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = FAIL;
		return;
	}

	mxt_read_mem(ts_data, user_object->start_address,
					sizeof(ts_data->fw_info.release_date),
					ts_data->fw_info.release_date);

	sprintf(data->cmd_buff, "%s_AT_%s_0x%.6X",
						ts_data->pdata->model_name,
						ts_data->fw_info.release_date,
						ts_data->fw_info.cfg_crc);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = OK;

	return;
}

static void get_threshold(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;
	u8 threshold = 0;

	data->cmd_state = RUNNING;

	set_default_result(data);

	mxt_read_object(ts_data, MXT_TOUCH_MULTITOUCHSCREEN_T9, 7, &threshold);

	sprintf(data->cmd_buff, "%03d", threshold);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = OK;
	return;
}

static void get_chip_vendor(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);
	sprintf(data->cmd_buff, "%s", TSP_VENDOR);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = OK;
	return;
}

static void get_chip_name(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);
	sprintf(data->cmd_buff, "%s", TSP_IC);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = OK;
	return;
}

static void get_x_num(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);
	sprintf(data->cmd_buff, "%d", ts_data->pdata->x_channel_no);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = OK;
	return;
}

static void get_y_num(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;

	data->cmd_state = RUNNING;

	set_default_result(data);
	sprintf(data->cmd_buff, "%d", ts_data->pdata->y_channel_no);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = OK;
	return;
}

static void run_reference_read(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;
	int ret;

	data->cmd_state = RUNNING;

	set_default_result(data);
	mxt_write_object(ts_data, MXT_GEN_ACQUISITIONCONFIG_T8, 2, 5);
	mxt_write_object(ts_data, MXT_GEN_ACQUISITIONCONFIG_T8, 3, 10);
	mxt_write_object(ts_data, MXT_PROCI_STYLUS_T47, 1, 30);
	mxt_write_object(ts_data, MXT_PROCI_STYLUS_T47, 3, 255);
	mxt_write_object(ts_data, MXT_PROCI_STYLUS_T47, 4, 10);
	mxt_write_object(ts_data, MXT_PROCI_STYLUS_T47, 5, 50);
	mxt_write_object(ts_data, MXT_PROCI_STYLUS_T47, 6, 255);
	mxt_write_object(ts_data, MXT_PROCI_STYLUS_T47, 7, 255);
	mxt_write_object(ts_data, MXT_PROCI_EXTRATOUCHSCREENDATA_T57, 0, 0);
	mxt_write_object(ts_data, MXT_TOUCH_MULTITOUCHSCREEN_T9, 7, 35);

	ret = mxt_read_all_diagnostic_data(ts_data, MXT_DIAG_REFERENCE_MODE);
	if (ret)
		data->cmd_state = FAIL;
	else {
		sprintf(data->cmd_buff, "%d,%d",
				ts_data->node_data->ref_min_data,
				ts_data->node_data->ref_max_data);
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = OK;
	}
}

static void get_reference(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;
	int node;

	data->cmd_state = RUNNING;
	set_default_result(data);

	node = mxt_xy_to_node(ts_data);
	if (node < 0) {
		data->cmd_state = FAIL;
		return;
	}
	sprintf(data->cmd_buff, "%u", ts_data->node_data->reference_data[node]);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = OK;
}

static void run_delta_read(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;
	int ret;

	data->cmd_state = RUNNING;

	set_default_result(data);

	ret = mxt_read_all_diagnostic_data(ts_data, MXT_DIAG_DELTA_MODE);
	if (ret)
		data->cmd_state = FAIL;
	else
		data->cmd_state = OK;

}

static void get_delta(void *device_data)
{
	struct mxt_data *ts_data = (struct mxt_data *)device_data;
	struct factory_data *data = ts_data->factory_data;
	int node;

	data->cmd_state = RUNNING;

	set_default_result(data);

	node = mxt_xy_to_node(ts_data);
	if (node < 0) {
		data->cmd_state = FAIL;
		return;
	}
	sprintf(data->cmd_buff, "%d", ts_data->node_data->delta_data[node]);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = OK;
}

static struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", not_support_cmd),},
	{TSP_CMD("module_on_master", not_support_cmd),},
	{TSP_CMD("module_off_slave", not_support_cmd),},
	{TSP_CMD("module_on_slave", not_support_cmd),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("run_delta_read", run_delta_read),},
	{TSP_CMD("get_delta", get_delta),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};

static ssize_t cmd_store(struct device *dev, struct device_attribute *devattr,
						const char *buf, size_t count)
{
	struct mxt_data *ts_data = dev_get_drvdata(dev);
	struct factory_data *data = ts_data->factory_data;
	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0, };
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;

	if (!ts_data->mxt_enabled) {
		dev_err(dev, "device is not ready.\n");
		goto err_out;
	}

	if (!data) {
		dev_err(dev, "factory_data is NULL.\n");
		goto err_out;
	}

	if (data->cmd_is_running == true) {
		dev_err(dev, "other cmd is running.\n");
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = true;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = RUNNING;

	for (i = 0; i < ARRAY_SIZE(data->cmd_param); i++)
		data->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(data->cmd, 0x00, ARRAY_SIZE(data->cmd));
	memcpy(data->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &data->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &data->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strlen(buff)) = '\0';
				ret = kstrtoint(buff, 10, \
					data->cmd_param + param_cnt);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while (cur - buf <= len);
	}

	dev_info(dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(dev, "cmd param %d= %d\n", i, data->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(ts_data);

err_out:
	return count;
}

static ssize_t cmd_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mxt_data *ts_data = dev_get_drvdata(dev);
	struct factory_data *data = ts_data->factory_data;
	char buff[16];

	dev_err(dev, "cmd: status:%d\n", data->cmd_state);

	switch (data->cmd_state) {
	case WAITING:
		sprintf(buff, "%s", TOSTRING(WAITING));
		break;
	case RUNNING:
		sprintf(buff, "%s", TOSTRING(RUNNING));
		break;
	case OK:
		sprintf(buff, "%s", TOSTRING(OK));
		break;
	case FAIL:
		sprintf(buff, "%s", TOSTRING(FAIL));
		break;
	case NOT_APPLICABLE:
		sprintf(buff, "%s", TOSTRING(NOT_APPLICABLE));
		break;
	default:
		sprintf(buff, "%s", TOSTRING(NOT_APPLICABLE));
		break;
	}

	return sprintf(buf, "%s\n", buff);
}

static ssize_t cmd_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mxt_data *ts_data = dev_get_drvdata(dev);
	struct factory_data *data = ts_data->factory_data;

	dev_err(dev, "factory: tsp cmd: result: \"%s(%d)\"\n",
				data->cmd_result, strlen(data->cmd_result));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = WAITING;

	return sprintf(buf, "%s\n", data->cmd_result);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, cmd_store);
static DEVICE_ATTR(cmd_status, S_IRUGO, cmd_status_show, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, cmd_result_show, NULL);

static struct attribute *touchscreen_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group touchscreen_attr_group = {
	.attrs = touchscreen_attributes,
};

#define MXT_DIAG_KEYDELTA_MODE		253
#define MXT_TOUCHKEY_DIVIDER		8

int get_key_raw_data(struct device *dev, u32 key_offset)
{
	struct mxt_data *ts_data = dev_get_drvdata(dev);
	s16 data = 0;

	mxt_read_diagnostic_data(ts_data, MXT_DIAG_KEYDELTA_MODE, key_offset,
									&data);
	data /= MXT_TOUCHKEY_DIVIDER;

	return (int)data;
}

static ssize_t touchkey_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mxt_data *ts_data = dev_get_drvdata(dev);
	u8 threshold = 0;
	u8 i;
	char key_threshold[5];

	mxt_read_object(ts_data, MXT_TOUCH_KEYARRAY_T15, 7, &threshold);
	sprintf(key_threshold, "%d ", threshold);

	for (i = 0; i < ts_data->pdata->key_size; i++)
		strcat(buf, key_threshold);

	strcat(buf, "\n");

	return strlen(buf);

}

static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);

static int __devinit init_sec_factory_test_touchkey(struct mxt_data *ts_data)
{
	struct device *touchkey_dev = ts_data->pdata->key_dev;

	if (!touchkey_dev) {
		dev_err(&ts_data->client->dev, "Failed to find fac touchkey dev\n");
		return -1;
	}

	if (dev_set_drvdata(touchkey_dev, ts_data) < 0)
		return -1;

	device_create_file(touchkey_dev, &dev_attr_touchkey_threshold);

	return 0;
}

static int __devinit init_sec_factory_test(struct mxt_data *ts_data)
{
	struct device *fac_dev_ts;
	struct factory_data *factory_data;
	struct node_data *node_data;
	u32 x, y;
	int i, ret;

	x = ts_data->pdata->x_channel_no;
	y = ts_data->pdata->y_channel_no;

	node_data = kzalloc(sizeof(struct node_data), GFP_KERNEL);
	if (unlikely(!node_data)) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	node_data->delta_data = kzalloc(sizeof(s16) * x * y, GFP_KERNEL);
	node_data->reference_data = kzalloc(sizeof(s16) * x * y, GFP_KERNEL);
	if (unlikely(!node_data->delta_data || !node_data->reference_data)) {
		ret = -ENOMEM;
		dev_err(&ts_data->client->dev, "err_alloc_node_data failed.\n");
		goto err_alloc_data_failed;
	}

	factory_data = kzalloc(sizeof(struct factory_data), GFP_KERNEL);
	if (unlikely(!factory_data)) {
		ret = -ENOMEM;
		dev_err(&ts_data->client->dev, "err_alloc_factory_data failed.\n");
		goto err_alloc_data_failed;
	}

	INIT_LIST_HEAD(&factory_data->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &factory_data->cmd_list_head);

	mutex_init(&factory_data->cmd_lock);
	factory_data->cmd_is_running = false;

	fac_dev_ts = device_create(sec_class, NULL, 0, ts_data, "tsp");
	if (!fac_dev_ts)
		dev_err(&ts_data->client->dev, "Failed to create fac tsp dev.\n");

	if (sysfs_create_group(&fac_dev_ts->kobj, &touchscreen_attr_group))
		dev_err(&ts_data->client->dev, "Failed to create sysfs (touchscreen_attr_group).\n");

	ts_data->factory_data = factory_data;
	ts_data->node_data = node_data;

	if (ts_data->pdata->key_size > 0)
		init_sec_factory_test_touchkey(ts_data);

	return 0;

err_alloc_data_failed:
	kfree(node_data->reference_data);
	kfree(node_data->delta_data);
	kfree(node_data);

	return ret;
}

static void deinit_sec_factory_test(struct mxt_data *ts_data)
{
	kfree(ts_data->factory_data);
	kfree(ts_data->node_data->reference_data);
	kfree(ts_data->node_data->delta_data);
	kfree(ts_data->node_data);
}
#endif /* end of defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST) */

/* I2C Slave addresses */
#define MXT_APP_LOW			0x4a
#define MXT_APP_HIGH			0x4b
#define MXT_BOOT_LOW			0x26
#define MXT_BOOT_HIGH			0x25

static int __devinit mxt_probe(struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct sec_ts_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev = NULL;
	u16 boot_address;
	int error = 0, i = 0;

	if (!pdata) {
		dev_err(&client->dev, "Platform data is not proper\n");
		return -EINVAL;
	}

	data = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	if (unlikely(!data)) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	data->mxt_key_state = kzalloc(sizeof(pdata->key_size), GFP_KERNEL);
	if (unlikely(!data->mxt_key_state)) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		kfree(data);
		return -ENOMEM;
	}

	boot_address = (client->addr == MXT_APP_LOW) ?
						MXT_BOOT_LOW : MXT_BOOT_HIGH;

	data->client_boot = i2c_new_dummy(client->adapter, boot_address);

	if (unlikely(!data->client_boot)) {
		dev_err(&client->dev, "Fail to register sub client[0x%x]\n",
								boot_address);
		error = -ENODEV;
		goto err_create_sub_client;
	}

	input_dev = input_allocate_device();
	if (unlikely(!input_dev)) {
		error = -ENOMEM;
		dev_err(&client->dev, "Input device allocation failed\n");
		goto err_allocate_input_device;
	}

	input_dev->name = SEC_TS_NAME;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	data->client = client;
	data->input_dev = input_dev;
	data->pdata = pdata;

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;

	if (data->pdata->key_size > 0) {
		set_bit(EV_KEY, input_dev->evbit);
		set_bit(EV_LED, input_dev->evbit);
		set_bit(LED_MISC, input_dev->ledbit);
		for (i = 0; i < data->pdata->key_size; i++)
			set_bit(data->pdata->key[i].code, input_dev->keybit);
	}

	init_completion(&data->init_done);

	input_mt_init_slots(input_dev, MXT_MAX_FINGER);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
						0, pdata->x_pixel_size, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
						0, pdata->y_pixel_size, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
						0, MXT_AREA_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
						0, MXT_AMPLITUDE_MAX, 0, 0);
	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	error = pdata->platform_init ? pdata->platform_init() : 0;
	if (unlikely(error))
		goto err_platform_init;

	error = input_register_device(input_dev);
	if (unlikely(error))
		goto err_register_input_device;

	error = mxt_fw_updater(data, NORMAL);
	if (unlikely(error)) {
		dev_err(&client->dev, "Failed to firmware update\n");
		goto err_fw_updater;
	}

	error = mxt_initialize(data);
	if (unlikely(error)) {
		dev_err(&client->dev, "Failed to initialize\n");
		goto err_touch_init;
	}

	client->irq = gpio_to_irq(data->pdata->gpio_irq);

	if (client->irq) {
		error = request_threaded_irq(client->irq,
						NULL,
						mxt_irq_thread,
						IRQF_TRIGGER_LOW | IRQF_ONESHOT,
						client->dev.driver->name,
						data);
		if (error) {
			dev_err(&client->dev, "Failed to register interrupt\n");
			goto err_req_irq;
		}
	}
#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)
	error = init_sec_factory_test(data);
	if (unlikely(error)) {
		dev_err(&client->dev, "Failed to make sec factory sysfs\n");
		goto err_sec_factory_init;
	}
#endif
#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
	error = mxt_debug_init(data);
	if (unlikely(error)) {
		dev_err(&client->dev, "Failed to make debug sysfs\n");
		goto err_debug_init;
	}
#endif

	complete_all(&data->init_done);
	dev_info(&client->dev, "Ateml MXT1188S probe done.\n");

	return 0;

#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
err_debug_init:
#endif
#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)
err_sec_factory_init:
	free_irq(client->irq, data);
#endif
err_req_irq:
err_touch_init:
	kfree(data->objects);
	data->objects = NULL;
	kfree(data->reportids);
	data->reportids = NULL;
err_fw_updater:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_register_input_device:
err_platform_init:
err_allocate_input_device:
	i2c_unregister_device(data->client_boot);
err_create_sub_client:
	kfree(data);

	return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);
#if defined(CONFIG_TOUCHSCREEN_ATMEL_DEBUG)
	mxt_debug_remove(data);
#endif
#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)
	deinit_sec_factory_test(data);
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	kfree(data->objects);
	kfree(data->reportids);
	input_unregister_device(data->input_dev);
	i2c_unregister_device(data->client_boot);
	mxt_power_off(data);
	if (data->pdata->platform_deinit)
		data->pdata->platform_deinit();
	kfree(data);

	return 0;
}

static void mxt_shutdown(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->input_dev->mutex);
	mxt_stop(data);
	mutex_unlock(&data->input_dev->mutex);

	return;
}

static struct i2c_device_id mxt_idtable[] = {
	{MXT_DEV_NAME, 0},
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static const struct dev_pm_ops mxt_pm_ops = {
	.suspend = mxt_suspend,
	.resume = mxt_resume,
};

static struct i2c_driver mxt_i2c_driver = {
	.id_table = mxt_idtable,
	.probe = mxt_probe,
	.remove = __devexit_p(mxt_remove),
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MXT_DEV_NAME,
#if defined(CONFIG_PM)
		.pm	= &mxt_pm_ops,
#endif
	},
	.shutdown = mxt_shutdown,
};

static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_i2c_driver);
}

static void __exit mxt_exit(void)
{
	i2c_del_driver(&mxt_i2c_driver);
}

module_init(mxt_init);
module_exit(mxt_exit);

MODULE_DESCRIPTION("Atmel MaXTouch driver");
MODULE_LICENSE("GPL");
