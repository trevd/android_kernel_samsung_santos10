/*
 * max77693-muic.c - MUIC driver for the Maxim 77693
 *
 *  Copyright (C) 2012 Samsung Electronics
 *  <sukdong.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/delay.h>
#include <linux/extcon.h>
#include <linux/irqdomain.h>

/* USB ID ADC Value */
#define ADC_GND			0x00
#define ADC_MHL			0x01
#define ADC_DOCK_PREV_KEY	0x04
#define ADC_DOCK_NEXT_KEY	0x07
#define ADC_DOCK_VOL_DN		0x0a /* 0x01010 14.46K ohm */
#define ADC_DOCK_VOL_UP		0x0b /* 0x01011 17.26K ohm */
#define ADC_DOCK_PLAY_PAUSE_KEY 0x0d
#define ADC_SMARTDOCK		0x10 /* 0x10000 40.2K ohm */
#define ADC_USB_AUDIODOCK	0x12 /* 0x10010 64.9K ohm */
#define ADC_CEA936ATYPE1_CHG	0x17 /* 0x10111 200K ohm */
#define ADC_JIG_USB_OFF		0x18 /* 0x11000 255K ohm */
#define ADC_JIG_USB_ON		0x19 /* 0x11001 301K ohm */
#define ADC_DESKDOCK		0x1a /* 0x11010 365K ohm */
#define ADC_CEA936ATYPE2_CHG	0x1b /* 0x11011 442K ohm */
#define ADC_JIG_UART_OFF	0x1c /* 0x11100 523K ohm */
#define ADC_JIG_UART_ON		0x1d /* 0x11101 619K ohm */
#define ADC_CARDOCK		0x1d /* 0x11101 619K ohm */
#define ADC_OPEN		0x1f

#define MAX77693_OTG_ILIM	0x80

#define MAX77693_REV_PASS1	0 /* pmic revision */

/**
 * struct max77693_muic_irq
 * @irq: the index of irq list of MUIC device.
 * @name: the name of irq.
 * @virq: the virtual irq to use irq domain
 */
struct max77693_muic_irq {
	unsigned int irq_id;
	const char *name;
	unsigned int virq;
};

static struct max77693_muic_irq muic_irqs[] = {
	{ MAX77693_MUIC_IRQ_INT1_ADC,		"muic-ADC" },
	{ MAX77693_MUIC_IRQ_INT2_CHGTYP,	"muic-CHGTYP" },
	{ MAX77693_MUIC_IRQ_INT2_VBVOLT,	"muic-VBVOLT" },
	{ MAX77693_MUIC_IRQ_INT1_ADC1K,		"muic-ADC1K" },
};

static const char const *max77693_path_name[] = {
	[PATH_OPEN]	= "OPEN",
	[PATH_USB_AP]	= "USB-AP",
	[PATH_AUDIO]	= "AUDIO",
	[PATH_UART_AP]	= "UART-AP",
	[PATH_USB_CP]	= "USB-CP",
	[PATH_UART_CP]	= "UART-CP",
	NULL,
};

/* MAX77693 MUIC CHG_TYP setting values */
enum {
	/* No Valid voltage at VB (Vvb < Vvbdet) */
	CHGTYP_NO_VOLTAGE	= 0x00,
	/* Unknown (D+/D- does not present a valid USB charger signature) */
	CHGTYP_USB		= 0x01,
	/* Charging Downstream Port */
	CHGTYP_DOWNSTREAM_PORT	= 0x02,
	/* Dedicated Charger (D+/D- shorted) */
	CHGTYP_DEDICATED_CHGR	= 0x03,
	/* Special 500mA charger, max current 500mA */
	CHGTYP_500MA		= 0x04,
	/* Special 1A charger, max current 1A */
	CHGTYP_1A		= 0x05,
	/* Reserved for Future Use */
	CHGTYP_RFU		= 0x06,
	/* Dead Battery Charging, max current 100mA */
	CHGTYP_DB_100MA		= 0x07,
	CHGTYP_MAX,

	CHGTYP_INIT,
	CHGTYP_MIN = CHGTYP_NO_VOLTAGE
};

enum {
	DOCK_KEY_NONE			= 0,
	DOCK_KEY_VOL_UP_PRESSED,
	DOCK_KEY_VOL_UP_RELEASED,
	DOCK_KEY_VOL_DOWN_PRESSED,
	DOCK_KEY_VOL_DOWN_RELEASED,
	DOCK_KEY_PREV_PRESSED,
	DOCK_KEY_PREV_RELEASED,
	DOCK_KEY_PLAY_PAUSE_PRESSED,
	DOCK_KEY_PLAY_PAUSE_RELEASED,
	DOCK_KEY_NEXT_PRESSED,
	DOCK_KEY_NEXT_RELEASED,
};

struct max77693_muic_info {
	struct device		*dev;
	struct extcon_dev	*edev;
	struct max77693_dev	*max77693;
	struct max77693_muic_platform_data *muic_data;
	int			irq;
	struct work_struct	irq_work;
	int			mansw;
	struct mutex		mutex;

	struct input_dev	*input;
	int			previous_key;
	int			path;

	/* For restore charger interrupt states */
	u8			chg_int_state;

	/* Flag for otg boost converter status */
	int			is_otg_boost;
};
static struct max77693_muic_info *gInfo;	/* for providing API */

static void max77693_muic_detect_dev(struct work_struct *work);

static ssize_t max77693_muic_regs_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const enum max77693_muic_reg muic_regs[] = {
		MAX77693_MUIC_REG_STATUS1,
		MAX77693_MUIC_REG_STATUS2,
		MAX77693_MUIC_REG_STATUS3,
		MAX77693_MUIC_REG_INTMASK1,
		MAX77693_MUIC_REG_INTMASK2,
		MAX77693_MUIC_REG_INTMASK3,
		MAX77693_MUIC_REG_CDETCTRL1,
		MAX77693_MUIC_REG_CDETCTRL2,
		MAX77693_MUIC_REG_CTRL1,
		MAX77693_MUIC_REG_CTRL2,
		MAX77693_MUIC_REG_CTRL3,
		MAX77693_MUIC_REG_INT1,
		MAX77693_MUIC_REG_INT2,
		MAX77693_MUIC_REG_INT3,
	};
	u8 val;
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(muic_regs); i++) {
		ret = max77693_read_reg(gInfo->max77693->regmap_muic,
					muic_regs[i], &val);
		if (likely(!ret)) {
			pr_info("%s: REG=0x%x, VAL=0x%x\n",
					__func__, muic_regs[i], val);
		}
	}

	return sprintf(buf, "%s\n", "done");
}

static ssize_t max77693_muic_regs_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret;
	u32 reg, val;

	ret = sscanf(buf, "0x%x,0x%x\n", &reg, &val);
	if (unlikely(ret < 0)) {
		dev_err(dev, "%s: invalid input value(%s)\n", __func__, buf);
		return -EINVAL;
	}

	pr_info("%s: reg=0x%x, val=0x%x\n", __func__, reg, val);
	if (reg >= MAX77693_MUIC_REG_END) {
		pr_warn("%s: skip to write (reg=0x%x)\n", __func__, reg);
		goto skip;
	}

	max77693_write_reg(gInfo->max77693->regmap_muic, reg, val);
skip:
	return size;
}

static DEVICE_ATTR(regs, S_IRUGO,
		max77693_muic_regs_show, max77693_muic_regs_store);

/* support otg control */
static void max77693_otg_control(struct max77693_muic_info *info, int enable)
{
	u8 int_mask, cdetctrl1, chg_cnfg_00;

	pr_info("%s: enable(%d)\n", __func__, enable);

	if (enable) {
		/* disable charger interrupt */
		max77693_read_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_INT_MASK, &int_mask);
		info->chg_int_state = int_mask;
		int_mask |= (1 << 4);	/* disable chgin intr */
		int_mask |= (1 << 6);	/* disable chg */
		int_mask &= ~(1 << 0);	/* enable byp intr */
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_INT_MASK, int_mask);

		/* disable charger detection */
		max77693_read_reg(info->max77693->regmap_muic,
			MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 &= ~(1 << 0);
		max77693_write_reg(info->max77693->regmap_muic,
			MAX77693_MUIC_REG_CDETCTRL1, cdetctrl1);

		/* OTG on, boost on, DIS_MUIC_CTRL=1 */
		max77693_read_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_CHG_MASK
				| CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BUCK_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= (CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_00, chg_cnfg_00);
	} else {
		/* OTG off, boost off, (buck on),
		   DIS_MUIC_CTRL = 0 unless CHG_ENA = 1 */
		max77693_read_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= CHG_CNFG_00_BUCK_MASK;
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

		msleep(50);

		/* enable charger detection */
		max77693_read_reg(info->max77693->regmap_muic,
			MAX77693_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 |= (1 << 0);
		max77693_write_reg(info->max77693->regmap_muic,
			MAX77693_MUIC_REG_CDETCTRL1, cdetctrl1);

		/* enable charger interrupt */
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_INT_MASK, info->chg_int_state);
		max77693_read_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_INT_MASK, &int_mask);
	}

	info->is_otg_boost = enable;
	pr_debug("%s: INT_MASK(0x%x), CDETCTRL1(0x%x), CHG_CNFG_00(0x%x)\n",
				__func__, int_mask, cdetctrl1, chg_cnfg_00);
}

/* use in mach for otg */
void otg_control(int enable)
{
	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77693_otg_control(gInfo, enable);
}
EXPORT_SYMBOL(otg_control);

static void max77693_powered_otg_control(struct max77693_muic_info *info,
								int enable)
{
	u8 reg_data = 0;
	pr_debug("%s: enable(%d)\n", __func__, enable);

	if (enable) {
		/* OTG on, boost on */
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_00, 0x05);

		reg_data = 0x0E;
		reg_data |= MAX77693_OTG_ILIM;
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_02, reg_data);
	} else {
		/* OTG off, boost off, (buck on) */
		max77693_write_reg(info->max77693->regmap,
			MAX77693_CHG_REG_CHG_CNFG_00, 0x04);
	}
}

/* use in mach for powered-otg */
void powered_otg_control(int enable)
{
	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77693_powered_otg_control(gInfo, enable);
}
EXPORT_SYMBOL(powered_otg_control);

int max77693_muic_get_status1_adc1k_value(void)
{
	u8 adc1k;
	int ret;

	ret = max77693_read_reg(gInfo->max77693->regmap_muic,
				MAX77693_MUIC_REG_STATUS1, &adc1k);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
					__func__, ret);
		return -EINVAL;
	}
	adc1k = adc1k & STATUS1_ADC1K_MASK ? 1 : 0;

	pr_debug("func:%s, adc1k: %d\n", __func__, adc1k);
	/* -1:err, 0:adc1k not detected, 1:adc1k detected */
	return adc1k;
}
EXPORT_SYMBOL(max77693_muic_get_status1_adc1k_value);

int max77693_muic_get_status1_adc_value(void)
{
	u8 adc;
	int ret;

	ret = max77693_read_reg(gInfo->max77693->regmap_muic,
		MAX77693_MUIC_REG_STATUS1, &adc);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
			__func__, ret);
		return -EINVAL;
	}

	return adc & STATUS1_ADC_MASK;
}
EXPORT_SYMBOL(max77693_muic_get_status1_adc_value);

static void max77693_muic_set_ctrl3(struct max77693_muic_info *info)
{
	u8 val;
	int ret;

	max77693_read_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_CTRL3, &val);
	dev_info(info->dev, "current ctrl3 value = 0x%x\n", val);

	val = 0;
	val |= 0x2 << CTRL3_ADCDBSET_SHIFT;	/* ADC debounce time: 25ms */

	if (info->muic_data->support_jig_auto)
		val |= 0x0 << CTRL3_JIGSET_SHIFT; /* JIG: auto detection */
	else
		val |= 0x2 << CTRL3_JIGSET_SHIFT; /* JIG: Hi-Impedance */

	if (info->muic_data->support_jig_auto)
		val |= 0x0 << CTRL3_BOOTSET_SHIFT; /* BTLD: auto detection */
	else
		val |= 0x2 << CTRL3_BOOTSET_SHIFT; /* BTLD: Hi-Impedance */
	dev_info(info->dev, "set ctrl3 value = 0x%x\n", val);

	max77693_write_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_CTRL3, val);

	max77693_read_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_CTRL3, &val);
	dev_info(info->dev, "new ctrl3 value = 0x%x\n", val);
}

static void max77693_muic_set_accdet(struct max77693_muic_info *info,
					int enable)
{
	u8 ctrl2_val;

	if (enable)
		ctrl2_val =  (1 << CTRL2_ACCDET_SHIFT);
	else
		ctrl2_val = 0;

	max77693_update_reg(info->max77693->regmap_muic,
			MAX77693_MUIC_REG_CTRL2, ctrl2_val, CTRL2_ACCDET_MASK);
	dev_info(info->dev, "%s: enable=%d\n", __func__, enable);
}

static int max77693_muic_handle_dock_vol_key(struct max77693_muic_info *info,
					     u8 adc)
{
	struct input_dev *input = info->input;
	int pre_key = info->previous_key;
	unsigned int code;
	int state;

	if (adc == ADC_OPEN) {
		switch (pre_key) {
		case DOCK_KEY_VOL_UP_PRESSED:
			code = KEY_VOLUMEUP;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
			break;
		case DOCK_KEY_VOL_DOWN_PRESSED:
			code = KEY_VOLUMEDOWN;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
			break;
		case DOCK_KEY_PREV_PRESSED:
			code = KEY_PREVIOUSSONG;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
			break;
		case DOCK_KEY_PLAY_PAUSE_PRESSED:
			code = KEY_PLAYPAUSE;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
			break;
		case DOCK_KEY_NEXT_PRESSED:
			code = KEY_NEXTSONG;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
			break;
		default:
			return 0;
		}

		input_event(input, EV_KEY, code, 0);
		input_sync(input);
		return 0;
	}

	if (pre_key == DOCK_KEY_NONE) {
		/*
		if (adc != ADC_DOCK_VOL_UP && adc != ADC_DOCK_VOL_DN && \
		adc != ADC_DOCK_PREV_KEY && adc != ADC_DOCK_PLAY_PAUSE_KEY \
		&& adc != ADC_DOCK_NEXT_KEY)
		*/
		if ((adc < 0x03) || (adc > 0x0d))
			return 0;
	}

	dev_info(info->dev, "%s: dock vol key(%d)\n", __func__, pre_key);

	state = 1;

	switch (adc) {
	case ADC_DOCK_VOL_UP:
		code = KEY_VOLUMEUP;
		info->previous_key = DOCK_KEY_VOL_UP_PRESSED;
		break;
	case ADC_DOCK_VOL_DN:
		code = KEY_VOLUMEDOWN;
		info->previous_key = DOCK_KEY_VOL_DOWN_PRESSED;
		break;
	case ADC_DOCK_PREV_KEY-1 ... ADC_DOCK_PREV_KEY+1:
		code = KEY_PREVIOUSSONG;
		info->previous_key = DOCK_KEY_PREV_PRESSED;
		break;
	case ADC_DOCK_PLAY_PAUSE_KEY-1 ... ADC_DOCK_PLAY_PAUSE_KEY+1:
		code = KEY_PLAYPAUSE;
		info->previous_key = DOCK_KEY_PLAY_PAUSE_PRESSED;
		break;
	case ADC_DOCK_NEXT_KEY-1 ... ADC_DOCK_NEXT_KEY+1:
		code = KEY_NEXTSONG;
		info->previous_key = DOCK_KEY_NEXT_PRESSED;
		break;
	case ADC_DESKDOCK: /* key release routine */
		state = 0;
		if (pre_key == DOCK_KEY_VOL_UP_PRESSED) {
			code = KEY_VOLUMEUP;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
		} else if (pre_key == DOCK_KEY_VOL_DOWN_PRESSED) {
			code = KEY_VOLUMEDOWN;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
		} else if (pre_key == DOCK_KEY_PREV_PRESSED) {
			code = KEY_PREVIOUSSONG;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
		} else if (pre_key == DOCK_KEY_PLAY_PAUSE_PRESSED) {
			code = KEY_PLAYPAUSE;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
		} else if (pre_key == DOCK_KEY_NEXT_PRESSED) {
			code = KEY_NEXTSONG;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
		} else {
			dev_warn(info->dev, "%s:%d should not reach here\n",
				 __func__, __LINE__);
			return 0;
		}
		break;
	default:
		dev_warn(info->dev, "%s: unsupported ADC(0x%02x)\n", __func__,
			 adc);
		return 0;
	}

	input_event(input, EV_KEY, code, state);
	input_sync(input);

	return 1;
}

static int max77693_muic_set_path(struct max77693_muic_info *info, int path)
{
	struct max77693_muic_platform_data *mdata = info->muic_data;
	int ret;
	u8 ctrl1_val, ctrl1_msk;
	u8 ctrl2_val, ctrl2_msk;
	int val;

	dev_info(info->dev, "set path to (%d)\n", path);

	if (mdata->set_safeout) {
		ret = mdata->set_safeout(path);
		if (ret) {
			dev_err(info->dev, "fail to set safout!(%d)\n", ret);
			return ret;
		}
	}

	if ((info->max77693->pmic_rev == MAX77693_REV_PASS1) &&
			((path == PATH_USB_CP) || (path == PATH_UART_CP))) {
		dev_info(info->dev,
				"PASS1 doesn't support manual path to CP.\n");
		if (path == PATH_USB_CP)
			path = PATH_USB_AP;
		else if (path == PATH_UART_CP)
			path = PATH_UART_AP;
	}

	switch (path) {
	case PATH_USB_AP:
		val = MAX77693_MUIC_CTRL1_BIN_1_001;
		break;
	case PATH_AUDIO:
		val = MAX77693_MUIC_CTRL1_BIN_2_010;
		break;
	case PATH_UART_AP:
		val = MAX77693_MUIC_CTRL1_BIN_3_011;
		break;
	case PATH_USB_CP:
		val = MAX77693_MUIC_CTRL1_BIN_4_100;
		break;
	case PATH_UART_CP:
		val = MAX77693_MUIC_CTRL1_BIN_5_101;
		break;
	default:
		dev_warn(info->dev, "invalid path(%d)\n", path);
		return -EINVAL;
	}

	ctrl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	ctrl1_msk = COMN1SW_MASK | COMP2SW_MASK;

	if (path == PATH_AUDIO) {
		ctrl1_val |= 0 << MICEN_SHIFT;
		ctrl1_msk |= MICEN_MASK;
	}

	max77693_update_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_CTRL1, ctrl1_val, ctrl1_msk);

	ctrl2_val = CTRL2_CPEn1_LOWPWD0;
	ctrl2_msk = CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK;

	max77693_update_reg(info->max77693->regmap_muic,
			MAX77693_MUIC_REG_CTRL2, ctrl2_val, ctrl2_msk);

	return 0;
}

#define MAX77693_PATH_FIX_USB_MASK \
	(BIT(EXTCON_USB_HOST) | BIT(EXTCON_SMARTDOCK) | BIT(EXTCON_AUDIODOCK))
#define MAX77693_PATH_USB_MASK \
	(BIT(EXTCON_USB) | BIT(EXTCON_JIG_USBOFF) | BIT(EXTCON_JIG_USBON))
#define MAX77693_PATH_AUDIO_MASK \
	(BIT(EXTCON_DESKDOCK) | BIT(EXTCON_CARDOCK))
#define MAX77693_PATH_UART_MASK	\
	(BIT(EXTCON_JIG_UARTOFF) | BIT(EXTCON_JIG_UARTON))

static int set_muic_path(struct max77693_muic_info *info)
{
	u32 state = info->edev->state;
	int path;
	int ret = 0;

	dev_info(info->dev, "%s: current state=0x%x, path=%s\n",
			__func__, state, max77693_path_name[info->path]);

	if (state & MAX77693_PATH_FIX_USB_MASK)
		path = PATH_USB_AP;
	else if (state & MAX77693_PATH_USB_MASK) {
		/* set path to usb following usb_sel */
		if (info->muic_data->usb_sel == USB_SEL_CP)
			path = PATH_USB_CP;
		else
			path = PATH_USB_AP;
	} else if (state & MAX77693_PATH_UART_MASK) {
		/* set path to uart following uart_sel */
		if (info->muic_data->uart_sel == UART_SEL_CP)
			path = PATH_UART_CP;
		else
			path = PATH_UART_AP;
	} else if (state & MAX77693_PATH_AUDIO_MASK)
		path = PATH_AUDIO;
	else {
		dev_info(info->dev, "%s: don't have to set path\n", __func__);
		return 0;
	}

	ret = max77693_muic_set_path(info, path);
	if (ret < 0) {
		dev_err(info->dev, "fail to set path(%s->%s),stat=%x,ret=%d\n",
				max77693_path_name[info->path],
				max77693_path_name[path], state, ret);
		return ret;
	}

	dev_info(info->dev, "%s: path: %s -> %s\n", __func__,
			max77693_path_name[info->path],
			max77693_path_name[path]);
	info->path = path;

	return 0;
}

void max77693_muic_set_usb_sel(int usb_sel)
{
	struct max77693_muic_info *info = gInfo;

	if (info->muic_data->usb_sel == usb_sel)
		return;

	info->muic_data->usb_sel = usb_sel;
	set_muic_path(info);
}
EXPORT_SYMBOL(max77693_muic_set_usb_sel);

void max77693_muic_set_uart_sel(int uart_sel)
{
	struct max77693_muic_info *info = gInfo;

	if (info->muic_data->uart_sel == uart_sel)
		return;

	info->muic_data->uart_sel = uart_sel;
	set_muic_path(info);
}
EXPORT_SYMBOL(max77693_muic_set_uart_sel);

int max77693_muic_get_usb_sel(void)
{
	struct max77693_muic_info *info = gInfo;
	return info->muic_data->usb_sel;
}
EXPORT_SYMBOL(max77693_muic_get_usb_sel);

int max77693_muic_get_uart_sel(void)
{
	struct max77693_muic_info *info = gInfo;
	return info->muic_data->uart_sel;
}
EXPORT_SYMBOL(max77693_muic_get_uart_sel);

void max77693_muic_set_cardock_support(bool support)
{
	struct max77693_muic_info *info = gInfo;

	pr_info("%s: support = %d\n", __func__, support);

	if (support)
		info->muic_data->support_cardock = true;
	else
		info->muic_data->support_cardock = false;

	info->irq = -1;
	schedule_work(&info->irq_work);
}
EXPORT_SYMBOL(max77693_muic_set_cardock_support);

bool max77693_muic_get_cardock_support(void)
{
	struct max77693_muic_info *info = gInfo;

	pr_info("%s: mode = %d\n", __func__, info->muic_data->support_cardock);
	return info->muic_data->support_cardock;

}
EXPORT_SYMBOL(max77693_muic_get_cardock_support);

static void _detected(struct max77693_muic_info *info, u32 new_state)
{
	u32 current_state;
	u32 changed_state;
	int index;
	int attach;
	int ret;

	current_state = info->edev->state;
	changed_state = current_state ^ new_state;

	dev_info(info->dev, "state: cur=0x%x, new=0x%x, changed=0x%x\n",
			current_state, new_state, changed_state);
	for (index = 0; index < SUPPORTED_CABLE_MAX; index++) {
		if (!(changed_state & BIT(index)))
			continue;

		if (new_state & BIT(index))
			attach = 1;
		else
			attach = 0;

		ret = extcon_set_cable_state(info->edev,
				extcon_cable_name[index], attach);
		if (unlikely(ret < 0))
			dev_err(info->dev, "fail to notify %s(%d), %d, ret=%d\n",
				extcon_cable_name[index], index, attach, ret);
	}

	set_muic_path(info);
}

static int max77693_muic_handle_attach(struct max77693_muic_info *info,
				       u8 status1, u8 status2)
{
	u8 adc;
	u8 vbvolt;
	u8 chgtyp;
	u8 chgdetrun;
	u8 adc1k;
	u32 pre_state;
	u32 new_state;
	int ret;

	new_state = 0;
	pre_state = info->edev->state;

	adc = status1 & STATUS1_ADC_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	chgdetrun = status2 & STATUS2_CHGDETRUN_MASK;

	dev_info(info->dev, "st1:0x%x st2:0x%x\n", status1, status2);
	dev_info(info->dev,
		"adc:0x%x, adc1k:0x%x, chgtyp:0x%x, vbvolt:%d, otg_boost:%d\n",
				adc, adc1k, chgtyp, vbvolt, info->is_otg_boost);
	dev_info(info->dev, "chgdetrun:0x%x, prev state:0x%x\n",
				chgdetrun, pre_state);

	/* Workaround for Factory mode in MUIC PASS2.
	 * Abandon adc interrupt of approximately +-100K range
	 * if previous cable status was JIG UART BOOT OFF.
	 * In uart path cp, adc is unstable state
	 * MUIC PASS2 turn to AP_UART mode automatically
	 * So, in this state set correct path manually.
	 */
	if (info->max77693->pmic_rev > MAX77693_REV_PASS1) {
		if ((pre_state & BIT(EXTCON_JIG_UARTOFF)) &&
			((adc == ADC_JIG_UART_OFF + 1 ||
			(adc == ADC_JIG_UART_OFF - 1))) &&
			(info->muic_data->uart_sel == PATH_UART_CP)) {
			dev_warn(info->dev, "abandon ADC\n");
			new_state = BIT(EXTCON_JIG_UARTOFF);
			goto __found_cable;
		}
	}

	/* 1Kohm ID regiter detection (mHL)
	 * Old MUIC : ADC value:0x00 or 0x01, ADCLow:1
	 * New MUIC : ADC value is not set(Open), ADCLow:1, ADCError:1
	 */
	if (adc1k) {
		new_state = BIT(EXTCON_MHL);
		/* If is_otg_boost is true,
		 * it means that vbolt is from the device to accessory.
		 */
		if (vbvolt && !info->is_otg_boost)
			new_state |= BIT(EXTCON_MHL_VB);
		goto __found_cable;
	}

	switch (adc) {
	case ADC_GND:
		if (chgtyp == CHGTYP_NO_VOLTAGE)
			new_state = BIT(EXTCON_USB_HOST);
		else if (chgtyp == CHGTYP_USB ||
				chgtyp == CHGTYP_DOWNSTREAM_PORT ||
				chgtyp == CHGTYP_DEDICATED_CHGR ||
				chgtyp == CHGTYP_500MA ||
				chgtyp == CHGTYP_1A) {
			dev_info(info->dev, "OTG charging pump\n");
			new_state = BIT(EXTCON_TA);
			goto __found_cable;
		}
		break;
	case ADC_SMARTDOCK:
		new_state = BIT(EXTCON_SMARTDOCK);
		if (chgtyp == CHGTYP_DEDICATED_CHGR)
			new_state |= BIT(EXTCON_SMARTDOCK_TA);
		else if (chgtyp == CHGTYP_USB)
			new_state |= BIT(EXTCON_SMARTDOCK_USB);
		break;
	case ADC_JIG_UART_OFF:
		new_state = BIT(EXTCON_JIG_UARTOFF);
		if (vbvolt && !info->is_otg_boost) {
			new_state |= BIT(EXTCON_JIG_UARTOFF_VB);
			max77693_muic_set_accdet(info, 0);
		} else
			max77693_muic_set_accdet(info, 1);
		break;
	case ADC_CARDOCK:	/* ADC_CARDOCK == ADC_JIG_UART_ON */
		if (!info->muic_data->support_cardock) {
			new_state = BIT(EXTCON_JIG_UARTON);
			break;
		}
		new_state = BIT(EXTCON_CARDOCK);
		if (vbvolt && !info->is_otg_boost)
			new_state |= BIT(EXTCON_CARDOCK_VB);
		break;
	case ADC_JIG_USB_OFF:
		new_state = BIT(EXTCON_JIG_USBOFF);
		break;
	case ADC_JIG_USB_ON:
		new_state = BIT(EXTCON_JIG_USBON);
		break;
	case ADC_DESKDOCK:
		new_state = BIT(EXTCON_DESKDOCK);
		if (vbvolt && !info->is_otg_boost)
			new_state |= BIT(EXTCON_DESKDOCK_VB);
		break;
	case ADC_USB_AUDIODOCK:
		if (vbvolt && !info->is_otg_boost)
			new_state = BIT(EXTCON_AUDIODOCK);
		break;
	case ADC_CEA936ATYPE1_CHG:
	case ADC_CEA936ATYPE2_CHG:
	case ADC_OPEN:
		switch (chgtyp) {
		case CHGTYP_USB:
		case CHGTYP_DOWNSTREAM_PORT:
			if (adc == ADC_CEA936ATYPE1_CHG ||
					adc == ADC_CEA936ATYPE2_CHG)
				new_state = BIT(EXTCON_CEA936_CHG);
			else
				new_state = BIT(EXTCON_USB);
			break;
		case CHGTYP_DEDICATED_CHGR:
		case CHGTYP_500MA:
		case CHGTYP_1A:
			new_state = BIT(EXTCON_TA);
			break;
		default:
			break;
		}
		break;
	default:
		dev_warn(info->dev, "unsupported adc=0x%x\n", adc);
		break;
	}

	if (!new_state) {
		dev_warn(info->dev, "Fail to get cable type (adc=0x%x)\n", adc);
		return -1;
	}

__found_cable:
	_detected(info, new_state);

	return 0;
}

static int max77693_muic_handle_detach(struct max77693_muic_info *info)
{
	u8 ctrl2_val;

	/* Workaround: irq doesn't occur after detaching mHL cable */
	max77693_write_reg(info->max77693->regmap_muic, MAX77693_MUIC_REG_CTRL1,
					MAX77693_MUIC_CTRL1_BIN_0_000);

	/* Enable Factory Accessory Detection State Machine */
	max77693_muic_set_accdet(info, 1);

	max77693_update_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_CTRL2, CTRL2_CPEn0_LOWPWD1,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	max77693_read_reg(info->max77693->regmap_muic, MAX77693_MUIC_REG_CTRL2,
				&ctrl2_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, ctrl2_val);

	info->previous_key = DOCK_KEY_NONE;

	_detected(info, 0);

	return 0;
}

static void max77693_muic_detect_dev(struct work_struct *work)
{
	struct max77693_muic_info *info = container_of(work,
			struct max77693_muic_info, irq_work);
	u32 pre_state;
	int irq_id = -1;
	u8 status[2];
	u8 adc;
	u8 chgtyp;
	u8 adcerr;
	int intr = INT_ATTACH;
	int i;
	int ret;

	if (!info->edev)
		return;

	pre_state = info->edev->state;

	mutex_lock(&info->mutex);

	for (i = 0 ; i < ARRAY_SIZE(muic_irqs) ; i++)
		if (info->irq == muic_irqs[i].virq)
			irq_id = muic_irqs[i].irq_id;

	dev_info(info->dev, "detected accesories, irq id:%d, pre_stat=0x%x\n",
			irq_id, pre_state);

	ret = max77693_bulk_read(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_STATUS1, 2, status);
	if (ret) {
		dev_err(info->dev, "fail to read muic status1(%d)\n", ret);
		goto err;
	}
	dev_info(info->dev, "STATUS1:0x%x, 2:0x%x\n", status[0], status[1]);

	adc = status[0] & STATUS1_ADC_MASK;
	adcerr = status[0] & STATUS1_ADCERR_MASK;
	chgtyp = status[1] & STATUS2_CHGTYP_MASK;

	if (adcerr) {
		dev_err(info->dev, "adc error has occured.\n");
		goto err;
	}

	if ((pre_state & BIT(EXTCON_DESKDOCK)) &&
			(irq_id == MAX77693_MUIC_IRQ_INT1_ADC) &&
			(adc != ADC_OPEN)) {
		/* To Do: adc condition should be modifed */
		max77693_muic_handle_dock_vol_key(info, adc);
		goto err;
	}

	if (adc == ADC_OPEN) {
		if (chgtyp == CHGTYP_NO_VOLTAGE)
			intr = INT_DETACH;
		else if (chgtyp == CHGTYP_USB ||
			 chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			 chgtyp == CHGTYP_DEDICATED_CHGR ||
			 chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A) {
			if (pre_state & (BIT(EXTCON_USB_HOST) |
					BIT(EXTCON_DESKDOCK) |
					BIT(EXTCON_CARDOCK)))
				intr = INT_DETACH;
		}
	}

	if (intr == INT_ATTACH) {
		dev_info(info->dev, "%s: ATTACHED\n", __func__);
		max77693_muic_handle_attach(info, status[0], status[1]);
	} else {
		dev_info(info->dev, "%s: DETACHED\n", __func__);
		max77693_muic_handle_detach(info);
	}

err:
	mutex_unlock(&info->mutex);

	return;
}
static irqreturn_t max77693_muic_irq_handler(int irq, void *data)
{
	struct max77693_muic_info *info = data;
	dev_info(info->dev, "%s: irq:%d\n", __func__, irq);

	info->irq = irq;
	schedule_work(&info->irq_work);

	return IRQ_HANDLED;
}

/* Initilize muic irqs.
 * This should be called after the other cable drivers register extcon notifier.
 * So, calls this funciton in late_initcall_sync.
 */
static int __init max77693_muic_irq_init(void)
{
	struct max77693_muic_info *info = gInfo;
	struct max77693_dev *max77693;
	u8 val;
	int i;
	int ret;

	if (!info) {
		pr_err("%s: info is NULL\n", __func__);
		return -1;
	}

	max77693 = info->max77693;

	INIT_WORK(&info->irq_work, max77693_muic_detect_dev);

	/* Support irq domain for MAX77693 MUIC device */
	for (i = 0; i < ARRAY_SIZE(muic_irqs); i++) {
		struct max77693_muic_irq *muic_irq = &muic_irqs[i];
		unsigned int virq = 0;
		irq_hw_number_t hwirq = muic_irq->irq_id + max77693->irq_base;

		virq = irq_create_mapping(max77693->irq_domain, hwirq);
		if (!virq) {
			dev_err(info->dev, "%s: failed to create irq mapping\n",
								__func__);
			return -1;
		}
		muic_irq->virq = virq;
		ret = request_threaded_irq(virq, NULL,
				max77693_muic_irq_handler,
				IRQF_ONESHOT | IRQF_NO_SUSPEND,
				muic_irq->name, info);
		if (ret) {
			dev_err(info->dev,
				"failed: irq request (IRQ: %ld,"
				" error :%d)\n",
				hwirq, ret);

			return -1;
		}
		dev_info(info->dev, "%s: virq=%d, hwirq=%ld, irq_id=%d\n",
				__func__, virq, hwirq, muic_irq->irq_id);
	}

	/* Clear interrupts and detects accessories */
	max77693_read_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_INT1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT1, val);
	max77693_read_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_INT2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT2, val);
	max77693_read_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_INT3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77693_MUIC_REG_INT3, val);

	info->irq = -1;
	schedule_work(&info->irq_work);

	return 0;
}
late_initcall_sync(max77693_muic_irq_init);

static int __devinit max77693_muic_probe(struct platform_device *pdev)
{
	struct max77693_dev *max77693 = dev_get_drvdata(pdev->dev.parent);
	struct max77693_platform_data *pdata = dev_get_platdata(max77693->dev);
	struct max77693_muic_info *info;
	struct input_dev *input;
	int ret;

	pr_info("%s: max77693 muic probe\n", __func__);

	info = kzalloc(sizeof(struct max77693_muic_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "%s: failed to allocate info\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}
	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "%s: failed to allocate input\n", __func__);
		ret = -ENOMEM;
		goto err_kfree;
	}
	info->dev = &pdev->dev;
	info->max77693 = max77693;
	info->input = input;
	info->muic_data = pdata->muic_data;
	info->path = PATH_OPEN;
	info->is_otg_boost = 0;
	gInfo = info;

	platform_set_drvdata(pdev, info);

	input->name = pdev->name;
	input->phys = "deskdock-key/input0";
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input, EV_KEY, KEY_PLAYPAUSE);
	input_set_capability(input, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input, EV_KEY, KEY_NEXTSONG);

	ret = input_register_device(input);
	if (ret) {
		dev_err(info->dev, "%s: fail to register input device(%d)\n",
					__func__, ret);
		goto err_input;
	}

	if (info->muic_data->init_cb)
		info->muic_data->init_cb();

	mutex_init(&info->mutex);

	max77693_muic_set_ctrl3(info);

	/* Initialize extcon device */
	info->edev = devm_kzalloc(&pdev->dev, sizeof(struct extcon_dev),
				  GFP_KERNEL);
	if (!info->edev) {
		dev_err(&pdev->dev, "failed to allocate memory for extcon\n");
		ret = -ENOMEM;
		goto err_edev_alloc;
	}
	info->edev->name = "max77693-muic";
	info->edev->supported_cable = extcon_cable_name;
	ret = extcon_dev_register(info->edev, NULL);
	if (ret) {
		dev_err(&pdev->dev, "failed to register extcon device\n");
		goto err_extcon;
	}

	ret = device_create_file(info->edev->dev, &dev_attr_regs);
	if (ret) {
		dev_err(&pdev->dev, "failed to create regs file(%d)\n", ret);
		goto err_file;
	}

	return 0;

err_file:
	extcon_dev_unregister(info->edev);
err_extcon:
	devm_kfree(&pdev->dev, info->edev);
err_edev_alloc:
	mutex_destroy(&info->mutex);
err_input:
	platform_set_drvdata(pdev, NULL);
	input_free_device(input);
err_kfree:
	kfree(info);
err_return:
	return ret;
}

static int __devexit max77693_muic_remove(struct platform_device *pdev)
{
	struct max77693_muic_info *info = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < ARRAY_SIZE(muic_irqs); i++)
		free_irq(muic_irqs[i].virq, info);

	input_unregister_device(info->input);
	cancel_work_sync(&info->irq_work);
	extcon_dev_unregister(info->edev);
	devm_kfree(&pdev->dev, info->edev);
	mutex_destroy(&info->mutex);
	kfree(info);

	return 0;
}

void max77693_muic_shutdown(struct device *dev)
{
	struct max77693_muic_info *info = dev_get_drvdata(dev);
	u8 val;
	int i;
	int ret;

	dev_info(info->dev, "%s: free muic irqs\n", __func__);
	for (i = 0; i < ARRAY_SIZE(muic_irqs); i++)
		free_irq(muic_irqs[i].virq, info);

	dev_info(info->dev, "%s: JIGSet: auto detection\n", __func__);
	val = (0 << CTRL3_JIGSET_SHIFT) | (0 << CTRL3_BOOTSET_SHIFT);

	ret = max77693_update_reg(info->max77693->regmap_muic,
				MAX77693_MUIC_REG_CTRL3, val,
				CTRL3_JIGSET_MASK | CTRL3_BOOTSET_MASK);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
		return;
	}
}

static struct platform_driver max77693_muic_driver = {
	.driver		= {
		.name	= "max77693-muic",
		.owner	= THIS_MODULE,
		.shutdown = max77693_muic_shutdown,
	},
	.probe		= max77693_muic_probe,
	.remove		= __devexit_p(max77693_muic_remove),
};

static int __init max77693_muic_init(void)
{
	return platform_driver_register(&max77693_muic_driver);
}
module_init(max77693_muic_init);

static void __exit max77693_muic_exit(void)
{
	pr_debug("func:%s\n", __func__);
	platform_driver_unregister(&max77693_muic_driver);
}
module_exit(max77693_muic_exit);

MODULE_DESCRIPTION("Maxim MAX77693 MUIC driver");
MODULE_AUTHOR("<sukdong.kim@samsung.com>");
MODULE_LICENSE("GPL");
