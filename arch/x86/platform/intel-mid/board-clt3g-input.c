/* arch/x86/platform/intel-mid/board-clt3g-input.c
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

#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/platform_data/sec_ts.h>
#include <linux/input/synaptics.h>

#include "device_libs/platform_gpio_keys.h"
#include "device_libs/platform_s7301.h"

#include "sec_libs/sec_common.h"
#include "board-clt3g.h"
#include <linux/i2c.h>

#define CTP3G_NR_NOT_REQUIRED		1

enum {
	GPIO_TOUCH_nINT = 0,
	GPIO_TOUCH_EN,
};

static struct gpio tsp_gpios[] = {
	[GPIO_TOUCH_nINT] = {
		.flags = GPIOF_IN,
		.label = "TSP_INT",
	},
	[GPIO_TOUCH_EN] = {
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "TSP_LDO_ON",
	},
};

static struct gpio_keys_button clt3g_button[] = {
	{KEY_VOLUMEUP,		-1, 1, "VOL_UP",	EV_KEY, 0, 20},
	{KEY_VOLUMEDOWN,	-1, 1, "VOL_DN",	EV_KEY, 0, 20},
	/* TODO: remove from counts after this line */
	{KEY_HOMEPAGE,		-1, 0, "EXT_WAKEUP",	EV_KEY, 0, 20},
};

static void tsp_set_power(bool on)
{
	if (on) {
		pr_debug("tsp: power on.\n");
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN].gpio, 1);
		mdelay(300);
	} else {
		pr_debug("tsp: power off.\n");
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN].gpio, 0);
		mdelay(50);
	}
}

static int tsp_platform_init(void)
{
	return gpio_request_array(tsp_gpios, ARRAY_SIZE(tsp_gpios));
}

static int tsp_platform_deinit(void)
{
	gpio_free_array(tsp_gpios, ARRAY_SIZE(tsp_gpios));

	return 0;
}

static struct synaptics_fw_info clt3g_tsp_fw_info = {
	.vendor		= "SY",
};

void *synaptics_s7301_platform_data(void *info)
{
	static struct sec_ts_platform_data platform_data;
	struct i2c_board_info *i2c_info = info;
	int i;


	for (i = 0; i < ARRAY_SIZE(tsp_gpios); i++)
		tsp_gpios[i].gpio = get_gpio_by_name(tsp_gpios[i].label);

	platform_data.i2c_address	= (unsigned short)i2c_info->addr;

	platform_data.platform_init	= tsp_platform_init;
	platform_data.platform_deinit	= tsp_platform_deinit;

	platform_data.gpio_en		= tsp_gpios[GPIO_TOUCH_EN].gpio;
	platform_data.gpio_irq		= tsp_gpios[GPIO_TOUCH_nINT].gpio;

	platform_data.model_name	= "S7301";
	platform_data.fw_info		= &clt3g_tsp_fw_info,
	platform_data.fw_name		= "synaptics/p5100.fw";
	platform_data.ext_fw_name	= "/mnt/sdcard/p5100.img";
	platform_data.rx_channel_no	= 42;	/* Y channel line */
	platform_data.tx_channel_no	= 7;	/* X channel line */
	platform_data.x_pixel_size	= 1279;
	platform_data.y_pixel_size	= 799;
	platform_data.pivot		= false;
	platform_data.set_power		= tsp_set_power;

	return &platform_data;
}

void __init sec_clt3g_input_init_early(void)
{
	intel_mid_customize_gpio_keys(clt3g_button,
			ARRAY_SIZE(clt3g_button) - CTP3G_NR_NOT_REQUIRED);
}

static ssize_t sec_key_pressed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int key_press_status = 0;
	unsigned int gpio;
	unsigned int value;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(clt3g_button); i++) {
		gpio = clt3g_button[i].gpio;
		if (gpio < 0)
			continue;
		value = gpio_get_value(gpio);
		value = clt3g_button[i].active_low ? !value : value;
		key_press_status |= (!!value) << i;
	}

	return sprintf(buf, "%u\n", key_press_status);
}

static DEVICE_ATTR(sec_key_pressed, S_IRUGO, sec_key_pressed_show, NULL);

static int __init clt3g_create_sec_key_dev(void)
{
	struct device *sec_key;
	static dev_t key;

	sec_key = device_create(sec_class, NULL, key, NULL, "sec_key");
	if (unlikely(!sec_key)) {
		pr_err("(%s): failed to create sysfs (sec_key)!\n",
				__func__);
		return -ENOMEM;
	}

	if (device_create_file(sec_key, &dev_attr_sec_key_pressed) < 0) {
		pr_err("(%s): failed to create device file (sec_key)!\n",
				__func__);
		device_destroy(sec_class, key);
		return -ENOMEM;
	}

	return 0;
}

static void __init clt3g_button_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(clt3g_button); i++)
		clt3g_button[i].gpio =
			get_gpio_by_name(clt3g_button[i].desc);

	clt3g_create_sec_key_dev();
}

void __init sec_clt3g_input_init_rest(void)
{
	clt3g_button_init();
}
