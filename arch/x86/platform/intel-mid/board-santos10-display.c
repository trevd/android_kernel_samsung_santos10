/* arch/x86/platform/intel-mid/board-santos10-display.c
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

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <linux/i2c/tc35876x-sec.h>
#include <linux/regulator/consumer.h>

#include <video/cmc624.h>
#include <video/psb_sec_bl.h>

#include <asm/intel-mid.h>
#include <asm/intel_muxtbl.h>

#include "sec_libs/sec_common.h"
#include "sec_libs/platform_cmc624_tune.h"

#define MAX_CMC624_PWM_COUNTER		0x07FF	/* PWM_COUNTER field has 11 bit */
#define SANTOS10_SDC_MAX_PWM_CNT	0x06E5
#define SANTOS10_SDC_DEF_PWM_CNT	0x03B6
#define SANTOS10_SDC_MIN_PWM_CNT	0x0019

#define SANTOS10_BOE_MAX_PWM_CNT	0x06E5
#define SANTOS10_BOE_DEF_PWM_CNT	0x03B6
#define SANTOS10_BOE_MIN_PWM_CNT	0x0019

#define SANTOS10_AUO_MAX_PWM_CNT	0x0528
#define SANTOS10_AUO_DEF_PWM_CNT	0x02D8
#define SANTOS10_AUO_MIN_PWM_CNT	0x0014

#define GET_PWR_LUT(_org, _ratio) \
	((_org) * (_ratio) / 10000 + (((_org) * (_ratio) % 10000) ? 1 : 0))

/* LCD panel */
enum {
	PANEL_SDC = 0,
	PANEL_BOE,
	PANEL_AUO,
	MAX_PANEL_ID,
};

/*
 * GPIO
 */
enum {
	GPIO_BACKLIGHT_RESET = 0
};

struct gpio backlight_gpios[] = {
	[GPIO_BACKLIGHT_RESET] = {
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "BACKLIGHT_RESET"
	}
};

enum {
	GPIO_MLCD_ON = 0,
	GPIO_IMA_PWR_EN,
	GPIO_IMA_SLEEP,
	GPIO_IMA_NRST,
	GPIO_IMA_CMC_EN,
	GPIO_IMA_INT,
};

static struct gpio cmc624_gpios[] = {
	[GPIO_MLCD_ON]	= {	/* V_IMA_1.8V */
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "MLCD_ON",
	},
	[GPIO_IMA_PWR_EN] = {	/* V_IMA_1.1V */
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "IMA_PWR_EN",
	},
	[GPIO_IMA_SLEEP]	= {
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "IMA_SLEEP",
	},
	[GPIO_IMA_NRST]	= {
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "IMA_nRST",
	},
	[GPIO_IMA_CMC_EN]	= {
		.flags	= GPIOF_OUT_INIT_HIGH,
		.label	= "IMA_CMC_EN",
	},
	[GPIO_IMA_INT]	= {
		.flags	= GPIOF_IN,
		.label	= "IMA_INT",
	},
};

enum {
	GPIO_LVDS_EN2 = 0,	/* VREG_LVS5_LVDS_1P8, VREG_L16_LVDS_3P3 */
	GPIO_LCD_EN,		/* DISPLAY_3.3V */
	GPIO_LVDS_RESET,
};

struct gpio tc35876x_gpios[] = {
	[GPIO_LVDS_EN2] = {
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "LVDS_LDO_EN2",
	},
	[GPIO_LCD_EN] = {
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "LCD_EN",
	},
	[GPIO_LVDS_RESET] = {
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "LVDS_RST",
	}
};

static struct regulator *vprog1_reg;

enum {
	GPIO_LVDS_I2C_CLK = 0,
	GPIO_LVDS_I2C_SDA,
	MAX_GPIO_LVDS_I2C,
};

struct gpio tc35876x_gpio_i2c[] = {
	[GPIO_LVDS_I2C_CLK] = {
		.label = "LVDS_I2C_CLK",
	},
	[GPIO_LVDS_I2C_SDA] = {
		.label = "LVDS_I2C_SDA",
	}
};

static struct intel_muxtbl *muxtbl_lvds_i2c[MAX_GPIO_LVDS_I2C];

/*
 * LP8553 BACKLIGHT CHIP
 */

static struct platform_device santos103g_backlight_device = {
	.name          = "psb_backlight",
	.id            = -1,
	.dev            = {
		.platform_data = NULL,
	},
};

static int backlight_gpio_init(void)
{
	int ret;

	ret = gpio_request_array(backlight_gpios, ARRAY_SIZE(backlight_gpios));
	if (unlikely(ret < 0))
		pr_err("%s: failed to request backlight gpio(%d)",
			__func__, ret);

	return ret;
}

static void backlight_power(int on)
{
	gpio_direction_output(backlight_gpios[GPIO_BACKLIGHT_RESET].gpio, !!on);
}

static int backlight_set_pwm(int level)
{
	return cmc624_set_pwm(level);
}

static void backglight_set_auto_brt(int auto_brightness)
{
	cmc624_set_auto_brightness(auto_brightness);
}

/* rootfs_initcall */
int __init sec_santos10_backlight_init(void)
{
	static struct psb_backlight_platform_data pdata;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(backlight_gpios); i++)
		backlight_gpios[i].gpio =
				get_gpio_by_name(backlight_gpios[i].label);

	if (backlight_gpio_init() < 0)
		pr_err("Unable to init backlight gpio device\n");

	pdata.backlight_power = backlight_power;
	pdata.backlight_set_pwm = backlight_set_pwm;
	pdata.set_auto_brt = backglight_set_auto_brt;

	platform_device_add_data(&santos103g_backlight_device,
			&pdata, sizeof(pdata));
	if (platform_device_register(&santos103g_backlight_device) < 0)
		pr_err("Unable to register backlight device\n");

	return 0;
}

/*
 * TC358764 MIPI-LVDS BRIDGE CHIP
 */
static int lvds_platform_init(void)
{
	unsigned int i;
	int ret;

	ret = gpio_request_array(tc35876x_gpios, ARRAY_SIZE(tc35876x_gpios));
	if (unlikely(ret)) {
		pr_err("%s(%d):can't request gpios\n", __func__, __LINE__);
		return -ENODEV;
	}

	vprog1_reg = regulator_get(NULL, "vprog1");
	if (IS_ERR(vprog1_reg)) {
		pr_err("%s(%d):regulator_get failed\n",
				__func__, __LINE__);
		return PTR_ERR(vprog1_reg);
	}
	ret = regulator_enable(vprog1_reg);
	if (unlikely(ret))
		pr_err("Failed to enable regulator vprog1\n");

	return 0;
}

static void lvds_platform_deinit(void)
{
	unsigned int i;

	regulator_put(vprog1_reg);

	for (i = 0; i < ARRAY_SIZE(tc35876x_gpios); i++)
		gpio_free(tc35876x_gpios[i].gpio);

}

static void ltn101a03_power(bool on)
{
	if (on)
		gpio_set_value(tc35876x_gpios[GPIO_LCD_EN].gpio, 1);
	else {
		gpio_set_value(tc35876x_gpios[GPIO_LCD_EN].gpio, 0);
		msleep(500);
	}
}

static void tc35876x_power(int on)
{
	int ret;
	unsigned int i;

	if (on) {
		for (i = 0; i < MAX_GPIO_LVDS_I2C; i++) {
			ret = config_pin_flis(muxtbl_lvds_i2c[i]->mux.pinname,
				PULL, muxtbl_lvds_i2c[i]->mux.pull);
			if (unlikely(ret))
				pr_err("(%s) failed to config pin flis\n",
					muxtbl_lvds_i2c[i]->gpio.label);
			gpio_direction_output(tc35876x_gpio_i2c[i].gpio, 1);
		}
		/* VREG_L18_LVDS_1P2 */
		ret = regulator_enable(vprog1_reg);
		if (unlikely(ret < 0))
			pr_err("failed to enable regualtor\n");
		usleep_range(100, 110);

		/* VREG_LVS5_LVDS_1P8, VREG_L16_LVDS_3P3 */
		gpio_set_value(tc35876x_gpios[GPIO_LVDS_EN2].gpio, 1);

	} else {
		gpio_set_value(tc35876x_gpios[GPIO_LVDS_EN2].gpio, 0);
		regulator_disable(vprog1_reg);

		for (i = 0; i < MAX_GPIO_LVDS_I2C; i++) {
			gpio_direction_output(tc35876x_gpio_i2c[i].gpio, 0);
			ret = config_pin_flis(muxtbl_lvds_i2c[i]->mux.pinname,
						PULL, NONE);
			if (unlikely(ret))
				pr_err("(%s) failed to config pin flis\n",
					muxtbl_lvds_i2c[i]->gpio.label);
		}
		msleep(20);
	}
}

void *tc35876x_platform_data(void *data)
{
	static struct tc35876x_platform_data pdata;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(tc35876x_gpios); i++)
		tc35876x_gpios[i].gpio =
				get_gpio_by_name(tc35876x_gpios[i].label);

	for (i = 0; i < MAX_GPIO_LVDS_I2C; i++) {
		muxtbl_lvds_i2c[i] =
			intel_muxtbl_find_by_net_name(
				tc35876x_gpio_i2c[i].label);

		tc35876x_gpio_i2c[i].gpio = muxtbl_lvds_i2c[i]->gpio.gpio;
	}

	pdata.gpio_lvds_reset = tc35876x_gpios[GPIO_LVDS_RESET].gpio;
	pdata.platform_init = lvds_platform_init;
	pdata.platform_deinit = lvds_platform_deinit;
	pdata.power = tc35876x_power;
	pdata.power_rest = cmc624_power;
	pdata.panel_power = ltn101a03_power;
	pdata.backlight_power = backlight_power;

	return &pdata;
}


/*
 * CMC624 mDNIE CHIP
 */
static int santos10_cmc624_gpio_init(void)
{
	int ret;

	ret = gpio_request_array(cmc624_gpios, ARRAY_SIZE(cmc624_gpios));
	if (ret < 0)
		pr_err("%s: failed to request display gpio(%d)", __func__, ret);

	return 0;
}

static int santos10_init_tune_list(void)
{
	u32 id;
	int bg;
	int cabc;

	/* init command */
	TUNE_DATA_ID(id, MENU_CMD_INIT, MENU_SKIP, MENU_SKIP, MENU_SKIP,
				MENU_SKIP, MENU_SKIP, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, cmc624_init, ARRAY_SIZE(cmc624_init));

	/* Auto Browser CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_browser_cabcoff,
				ARRAY_SIZE(tune_auto_browser_cabcoff));

	/* Auto Browser CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_browser_cabcon,
				ARRAY_SIZE(tune_auto_browser_cabcon));

	/* Auto Gallery CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_gallery_cabcoff,
				ARRAY_SIZE(tune_auto_gallery_cabcoff));

	/* Auto Gallery CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_gallery_cabcon,
				ARRAY_SIZE(tune_auto_gallery_cabcon));

	/* Auto UI CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_ui_cabcoff,
				ARRAY_SIZE(tune_auto_ui_cabcoff));

	/* Auto UI CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_ui_cabcon,
				ARRAY_SIZE(tune_auto_ui_cabcon));

	/* Auto Video CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_video_cabcoff,
				ARRAY_SIZE(tune_auto_video_cabcoff));

	/* Auto Video CABC-on  */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_video_cabcon,
				ARRAY_SIZE(tune_auto_video_cabcon));

	/* Auto VT-call CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_vtcall_cabcoff,
				ARRAY_SIZE(tune_auto_vtcall_cabcoff));

	/* Auto VT-call CABC-on  */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_auto_vtcall_cabcon,
				ARRAY_SIZE(tune_auto_vtcall_cabcon));

	/* Bypass ON  */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_BYPASS_ON, MENU_SKIP, MENU_SKIP, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_bypass, ARRAY_SIZE(tune_bypass));

	/* Bypass OFF  */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_BYPASS_OFF, MENU_SKIP, MENU_SKIP, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_bypass, ARRAY_SIZE(tune_bypass));

	/* Camera */
	for (bg = MENU_MODE_DYNAMIC; bg < MAX_BACKGROUND_MODE; bg++) {
		for (cabc = MENU_CABC_OFF; cabc < MAX_CABC_MODE; cabc++) {
			TUNE_DATA_ID(id, MENU_CMD_TUNE, bg, MENU_APP_CAMERA,
				MENU_SKIP, MENU_SKIP, cabc, MENU_SKIP,
				MENU_SKIP);
			cmc624_register_tune_data(id, tune_camera,
						ARRAY_SIZE(tune_camera));
		}
	}

	/* Dynamic Browser CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_browser_cabcoff,
				ARRAY_SIZE(tune_dynamic_browser_cabcoff));

	/* Dynamic Browser CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_browser_cabcon,
				ARRAY_SIZE(tune_dynamic_browser_cabcon));

	/* Dynamic Gallery CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_gallery_cabcoff,
				ARRAY_SIZE(tune_dynamic_gallery_cabcoff));

	/* Dynamic Gallery CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_gallery_cabcon,
				ARRAY_SIZE(tune_dynamic_gallery_cabcon));

	/* Dynamic UI CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_ui_cabcoff,
				ARRAY_SIZE(tune_dynamic_ui_cabcoff));

	/* Dynamic UI CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_ui_cabcon,
				ARRAY_SIZE(tune_dynamic_ui_cabcon));

	/* Dynamic Video cabc-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_video_cabcoff,
				ARRAY_SIZE(tune_dynamic_video_cabcoff));

	/* Dynamic Video cabc-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_video_cabcon,
				ARRAY_SIZE(tune_dynamic_video_cabcon));

	/* Dynamic VT-call CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_vtcall_cabcoff,
				ARRAY_SIZE(tune_dynamic_vtcall_cabcoff));

	/* Dynamic VT-call CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_dynamic_vtcall_cabcon,
				ARRAY_SIZE(tune_dynamic_vtcall_cabcon));

	/* eBook CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcoff,
				ARRAY_SIZE(tune_ebook_cabcoff));

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcoff,
				ARRAY_SIZE(tune_ebook_cabcoff));

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcoff,
				ARRAY_SIZE(tune_ebook_cabcoff));

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcoff,
				ARRAY_SIZE(tune_ebook_cabcoff));

	/* eBook CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_DYNAMIC, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcon,
				ARRAY_SIZE(tune_ebook_cabcon));

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcon,
				ARRAY_SIZE(tune_ebook_cabcon));

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcon,
				ARRAY_SIZE(tune_ebook_cabcon));

	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_AUTO, MENU_APP_EBOOK,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_ebook_cabcon,
				ARRAY_SIZE(tune_ebook_cabcon));

	/* Movie Browser CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_ui_cabcoff,
				ARRAY_SIZE(tune_movie_ui_cabcoff));

	/* Movie browser CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_browser_cabcon,
				ARRAY_SIZE(tune_movie_browser_cabcon));

	/* Movie Gallery CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_gallery_cabcoff,
				ARRAY_SIZE(tune_movie_gallery_cabcoff));

	/* Movie UI CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_ui_cabcoff,
				ARRAY_SIZE(tune_movie_ui_cabcoff));

	/* Movie UI CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_ui_cabcon,
				ARRAY_SIZE(tune_movie_ui_cabcon));

	/* Movie Gallery CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_gallery_cabcon,
				ARRAY_SIZE(tune_movie_gallery_cabcon));

	/* Movie Video CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_video_cabcoff,
				ARRAY_SIZE(tune_movie_video_cabcoff));

	/* Movie Video CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_video_cabcon,
				ARRAY_SIZE(tune_movie_video_cabcon));

	/* Movie VT-call CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_vtcall_cabcoff,
				ARRAY_SIZE(tune_movie_vtcall_cabcoff));

	/* Movie VT-call CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_MOVIE, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_movie_vtcall_cabcon,
				ARRAY_SIZE(tune_movie_vtcall_cabcon));

	/* Negative CABC-off*/
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_SKIP, MENU_NEGATIVE_ON, MENU_CABC_OFF, MENU_SKIP,
		MENU_SKIP);
	cmc624_register_tune_data(id, tune_negative_cabcoff,
				ARRAY_SIZE(tune_negative_cabcoff));

	/* Negative CABC-on*/
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_SKIP, MENU_NEGATIVE_ON, MENU_CABC_ON, MENU_SKIP,
		MENU_SKIP);
	cmc624_register_tune_data(id, tune_negative_cabcon,
				ARRAY_SIZE(tune_negative_cabcon));

	/* Color-blind CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP,
		MENU_ACC_COLOR_BLIND);
	cmc624_register_tune_data(id, tune_color_blind_cabcoff,
				ARRAY_SIZE(tune_color_blind_cabcoff));

	/* Color-blind CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_SKIP, MENU_SKIP,
		MENU_SKIP,  MENU_SKIP, MENU_CABC_ON, MENU_SKIP,
		MENU_ACC_COLOR_BLIND);
	cmc624_register_tune_data(id, tune_color_blind_cabcon,
				ARRAY_SIZE(tune_color_blind_cabcon));

	/* Standard Browser CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_ui_cabcoff,
				ARRAY_SIZE(tune_standard_browser_cabcoff));

	/* Stadard browser CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_BROWSER,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_browser_cabcon,
				ARRAY_SIZE(tune_standard_browser_cabcon));


	/* Standard UI CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_ui_cabcoff,
				ARRAY_SIZE(tune_standard_ui_cabcoff));

	/* Standard UI CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_UI,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_ui_cabcon,
				ARRAY_SIZE(tune_standard_ui_cabcon));

	/* Standard Gallery CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_gallery_cabcoff,
				ARRAY_SIZE(tune_standard_gallery_cabcoff));

	/* Standard Gallery CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_GALLERY,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_gallery_cabcon,
				ARRAY_SIZE(tune_standard_gallery_cabcon));

	/* Standard VT-call CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_vtcall_cabcoff,
				ARRAY_SIZE(tune_standard_vtcall_cabcoff));

	/* Standard VT-call CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_VT,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_vtcall_cabcon,
				ARRAY_SIZE(tune_standard_vtcall_cabcon));

	/* Standard Video CABC-off */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_OFF, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_video_cabcoff,
				ARRAY_SIZE(tune_standard_video_cabcoff));

	/* Standard Video CABC-on */
	TUNE_DATA_ID(id, MENU_CMD_TUNE, MENU_MODE_STANDARD, MENU_APP_VIDEO,
		MENU_SKIP, MENU_SKIP, MENU_CABC_ON, MENU_SKIP, MENU_SKIP);
	cmc624_register_tune_data(id, tune_standard_video_cabcon,
				ARRAY_SIZE(tune_standard_video_cabcon));

	return 0;
}

static int santos10_power_vima_1_1V(int on)
{
	gpio_set_value(cmc624_gpios[GPIO_IMA_PWR_EN].gpio, !!on);
	return 0;
}

static int santos10_power_vima_1_8V(int on)
{
	gpio_set_value(cmc624_gpios[GPIO_MLCD_ON].gpio, !!on);
	return 0;
}

static u8 lcd_panel_id;
static u8 *lcd_name[MAX_PANEL_ID] = {
	"SDC_LTL101AL06",
	"BOE_BP101WX1",
	"AUO_B101EAN01",
};

static struct cabcoff_pwm_cnt_tbl santos10_pwm_tbl[MAX_PANEL_ID] = {
	{ /* SDC panel pwm table */
		.max	= SANTOS10_SDC_MAX_PWM_CNT,
		.def	= SANTOS10_SDC_DEF_PWM_CNT,
		.min	= SANTOS10_SDC_MIN_PWM_CNT,
	},
	{ /* BOE panel pwm table */
		.max	= SANTOS10_BOE_MAX_PWM_CNT,
		.def	= SANTOS10_BOE_DEF_PWM_CNT,
		.min	= SANTOS10_BOE_MIN_PWM_CNT,
	},
	{ /* AUO panel pwm table */
		.max	= SANTOS10_AUO_MAX_PWM_CNT,
		.def	= SANTOS10_AUO_DEF_PWM_CNT,
		.min	= SANTOS10_AUO_MIN_PWM_CNT,
	},
};

/* POWERLUT_xx values for CABC mode PWM control */
static u16 cmc624_pwr_luts[MAX_PWRLUT_LEV][MAX_PWRLUT_MODE][NUM_PWRLUT_REG] = {
	{ /* Indoor power look up table, 72% */
		/* UI scenario */
		{0x547, 0x4E1, 0x4F5, 0x5C2, 0x547, 0x50A, 0x4A3, 0x466},
		/* Video scenario */
		{0x47A, 0x4E1, 0x428, 0x5C2, 0x47A, 0x43D, 0x3D7, 0x399},
	},
	{ /* Outdoor power look up table for outdoor 1 (1k~15k), 82% */
		{0x547, 0x5AD, 0x4F5, 0x68F, 0x547, 0x50A, 0x4A3, 0x466},
		{0x47A, 0x4E1, 0x428, 0x5C2, 0x47A, 0x43D, 0x3D7, 0x399},
	},
	{ /* Outdoor power look up table (15k ~), 100% */
		{0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF},
		{0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF},
	},
};

static struct cabcon_pwr_lut_tbl santos10_pwr_luts;

static void santos10_init_cmc624_pwr_luts(struct cabcoff_pwm_cnt_tbl *pwm_tbl)
{
	int ratio_max;
	int ratio_def;
	int ratio_min;
	unsigned int i, j, k;

	ratio_max = pwm_tbl->max * 10000 / MAX_CMC624_PWM_COUNTER;
	ratio_def = pwm_tbl->def * 10000 / MAX_CMC624_PWM_COUNTER;
	ratio_min = pwm_tbl->min * 10000 / MAX_CMC624_PWM_COUNTER;

	pr_info("%s: ratio: max=%d, def=%d, min=%d\n", __func__,
					ratio_max, ratio_def, ratio_min);

	for (i = 0; i < MAX_PWRLUT_LEV; i++) {
		for (j = 0; j < MAX_PWRLUT_MODE; j++) {
			for (k = 0; k < NUM_PWRLUT_REG; k++) {
				santos10_pwr_luts.max[i][j][k] = GET_PWR_LUT(
					cmc624_pwr_luts[i][j][k], ratio_max);
				santos10_pwr_luts.def[i][j][k] = GET_PWR_LUT(
					cmc624_pwr_luts[i][j][k], ratio_def);
				santos10_pwr_luts.min[i][j][k] = GET_PWR_LUT(
					cmc624_pwr_luts[i][j][k], ratio_min);
			}
		}
	}
}

void *cmc624_platform_data(void *data)
{
	unsigned int i;
	static struct cmc624_panel_data pdata;

	for (i = 0; i < ARRAY_SIZE(cmc624_gpios); i++)
		cmc624_gpios[i].gpio =
			get_gpio_by_name(cmc624_gpios[i].label);

	pdata.gpio_ima_sleep = cmc624_gpios[GPIO_IMA_SLEEP].gpio;
	pdata.gpio_ima_nrst = cmc624_gpios[GPIO_IMA_NRST].gpio;
	pdata.gpio_ima_cmc_en = cmc624_gpios[GPIO_IMA_CMC_EN].gpio;

	pdata.skip_init = false;
	pdata.skip_ldi_cmd = true;
	pdata.lcd_panel_id = lcd_panel_id;
	pdata.lcd_name = lcd_name[lcd_panel_id];

	pdata.init_tune_list = santos10_init_tune_list;
	pdata.power_vima_1_1V = santos10_power_vima_1_1V;
	pdata.power_vima_1_8V = santos10_power_vima_1_8V;
	pdata.init_power = santos10_cmc624_gpio_init;

	pdata.pwm_tbl = &santos10_pwm_tbl[lcd_panel_id];
	santos10_init_cmc624_pwr_luts(pdata.pwm_tbl);
	pdata.pwr_luts = &santos10_pwr_luts;
	return &pdata;
}

static int __init display_set_panel_id(char *str)
{
	unsigned int panel_id;

	if (!kstrtoint(str, 10, &panel_id))
		lcd_panel_id = panel_id;
	return 0;
}

early_param("lcd_panel_id", display_set_panel_id);
