/* arch/x86/platform/intel-mid/board-santos10-input.c
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
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/platform_data/sec_ts.h>

#include <linux/input/atmel_mxt1188s.h>

#include "sec_libs/sec_common.h"
#include "sec_libs/sec_debug.h"
#include "board-santos10.h"

enum {
	GPIO_EXT_WAKE = 0,
};

static struct gpio keys_map_high_gpios[] __initdata = {
	[GPIO_EXT_WAKE] = {
		.label	= "EXT_WAKEUP",
	},
};

static struct gpio_event_direct_entry santos10_keypad_keys_map_high[] = {
	[GPIO_EXT_WAKE] = {
		.code	= KEY_POWER,
	},
};

static struct gpio_event_input_info santos10_keypad_keys_info_high = {
	.info.func		= gpio_event_input_func,
	.info.no_suspend	= true,
	.type			= EV_KEY,
	.keymap			= santos10_keypad_keys_map_high,
	.keymap_size		= ARRAY_SIZE(santos10_keypad_keys_map_high),
	.flags			= GPIOEDF_ACTIVE_HIGH,
	.debounce_time.tv64	= 3 * NSEC_PER_MSEC,
};

enum {
	GPIO_HOME_KEY = 0,
	GPIO_VOL_UP,
	GPIO_VOL_DN,
	GPIO_HALL_INT_N,
};

static struct gpio keys_map_low_gpios[] = {
	[GPIO_HOME_KEY] = {
		.label	= "HOME_KEY",
	},
	[GPIO_VOL_UP] = {
		.label	= "VOL_UP",
	},
	[GPIO_VOL_DN] = {
		.label	= "VOL_DN",
	},
};

static struct gpio_event_direct_entry santos10_keypad_keys_map_low[] = {
	[GPIO_HOME_KEY] = {
		.code	= KEY_HOMEPAGE,
	},
	[GPIO_VOL_UP] = {
		.code	= KEY_VOLUMEUP,
	},
	[GPIO_VOL_DN] = {
		.code	= KEY_VOLUMEDOWN,
	},
};

static struct gpio_event_input_info santos10_keypad_keys_info_low = {
	.info.func		= gpio_event_input_func,
	.info.no_suspend	= true,
	.type			= EV_KEY,
	.keymap			= santos10_keypad_keys_map_low,
	.keymap_size		= ARRAY_SIZE(santos10_keypad_keys_map_low),
	.debounce_time.tv64	= 3 * NSEC_PER_MSEC,
};

enum {
	GPIO_SW_FLIP = 0,
};

static struct gpio switches_map_high_gpios[] __initdata = {
	[GPIO_SW_FLIP] = {
		.label	= "HALL_INT_N",
	},
};

static struct gpio_event_direct_entry santos10_keypad_switches_map_low[] = {
	[GPIO_SW_FLIP] = {
		.code	= SW_FLIP,
	},
};

static struct gpio_event_input_info santos10_keypad_siwtches_info_high = {
	.info.func		= gpio_event_input_func,
	.info.no_suspend	= true,
	.type			= EV_SW,
	.keymap			= santos10_keypad_switches_map_low,
	.keymap_size		= ARRAY_SIZE(santos10_keypad_switches_map_low),
	.flags			= GPIOEDF_ACTIVE_HIGH,
	.debounce_time.tv64	= 10 * NSEC_PER_MSEC,
};

static struct gpio_event_info *santos10_keypad_info[] = {
/* FIXME: power key driver of PMIC will be used.
 * we don't need to register this */
#if 0
	&sangos10_keypad_keys_info_high.info,
#endif
	&santos10_keypad_keys_info_low.info,
	&santos10_keypad_siwtches_info_high.info
};

static struct gpio_event_platform_data santos10_keypad_data = {
	.name		= "sec_key",
	.info		= santos10_keypad_info,
	.info_count	= ARRAY_SIZE(santos10_keypad_info),
};

static struct platform_device santos10_keypad_device = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= 0,
	.dev = {
		.platform_data = &santos10_keypad_data,
	},
};

void __init sec_santos10_input_init_early(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(keys_map_high_gpios); i++)
		santos10_keypad_keys_map_high[i].gpio =
			get_gpio_by_name(keys_map_high_gpios[i].label);

	for (i = 0; i < ARRAY_SIZE(keys_map_low_gpios); i++)
		santos10_keypad_keys_map_low[i].gpio =
			get_gpio_by_name(keys_map_low_gpios[i].label);

	for (i = 0; i < ARRAY_SIZE(switches_map_high_gpios); i++)
		santos10_keypad_switches_map_low[i].gpio =
			get_gpio_by_name(switches_map_high_gpios[i].label);

	if (sec_debug_get_level()) {
		santos10_keypad_keys_info_high.flags |= GPIOEDF_PRINT_KEYS;
		santos10_keypad_keys_info_low.flags |= GPIOEDF_PRINT_KEYS;
		santos10_keypad_siwtches_info_high.flags |= GPIOEDF_PRINT_KEYS;
	}

	platform_device_register(&santos10_keypad_device);
}

static ssize_t sec_key_pressed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int key_press_status = 0;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(santos10_keypad_keys_map_high); i++) {
		if (unlikely
		    (santos10_keypad_keys_map_high[i].gpio < 0))
			continue;
		key_press_status |=
		    ((!!gpio_get_value(santos10_keypad_keys_map_high[i].gpio)
		      << i));
	}

	for (i = 0; i < ARRAY_SIZE(santos10_keypad_keys_map_low); i++) {
		if (unlikely
		    (santos10_keypad_keys_map_low[i].gpio < 0))
			continue;
		key_press_status |=
		    ((!gpio_get_value(santos10_keypad_keys_map_low[i].gpio)
		      << (i + ARRAY_SIZE(santos10_keypad_keys_map_high))));
	}

	return sprintf(buf, "%u\n", key_press_status);
}

static ssize_t hall_detect_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int gpio = santos10_keypad_switches_map_low[GPIO_SW_FLIP].gpio;
	bool state;

	if (gpio < 0) {
		pr_err("Couldn't find hall IC gpio.\n");
		return 0;
	}

	state = !gpio_get_value(gpio);
	pr_info("hall ic: sysfs state: %s\n", state ? "CLOSE" : "OPEN");

	return sprintf(buf, state ? "CLOSE\n" : "OPEN\n");
}

static DEVICE_ATTR(sec_key_pressed, S_IRUGO, sec_key_pressed_show, NULL);
static DEVICE_ATTR(hall_detect, S_IRUGO, hall_detect_show, NULL);

static int __init santos10_create_sec_key_dev(void)
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

	if (device_create_file(sec_key, &dev_attr_hall_detect) < 0) {
		pr_err("(%s): failed to create device file (hall_detect)!\n",
				__func__);
		device_destroy(sec_class, key);
		return -ENOMEM;
	}

	return 0;
}

/* rootfs_initcall */
int __init sec_santos10_button_init(void)
{
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(keys_map_high_gpios); i++) {
		if (santos10_keypad_keys_map_high[i].code != KEY_POWER)
			continue;
		keys_map_high_gpios[i].gpio =
			get_gpio_by_name(keys_map_high_gpios[i].label);
		if (keys_map_high_gpios[i].gpio < 0)
			continue;
		err = gpio_request_one(keys_map_high_gpios[i].gpio,
				GPIOF_IN, keys_map_high_gpios[i].label);
		if (unlikely(err))
			pr_warning("(%s): can't request %s(%d)!\n", __func__,
				keys_map_high_gpios[i].label,
				keys_map_high_gpios[i].gpio);
	}

	santos10_create_sec_key_dev();

	return 0;
}

/* touch device */
enum {
	GPIO_TOUCH_EN = 0,
	GPIO_TOUCH_EN2,
	GPIO_TOUCH_RST,
	GPIO_TOUCH_nINT,
};

static struct gpio tsp_gpios[] = {
	[GPIO_TOUCH_EN] = {
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "TSP_LDO_ON",
	},
	[GPIO_TOUCH_EN2] = {
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "TSP_SW3_EN",
	},
	[GPIO_TOUCH_RST] = {
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "TSP_RST",
	},
	[GPIO_TOUCH_nINT] = {
		.flags = GPIOF_IN,
		.label = "TSP_INT",
	},
};

static void santos10_syna_set_power(bool on)
{
	if (on) {
		pr_info("tsp: power on.\n");
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN].gpio, 1);
		mdelay(300);
	} else {
		pr_info("tsp: power off.\n");
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN].gpio, 0);
		mdelay(50);
	}
}

static void santos10_atmel_set_power(bool on)
{
	if (on) {
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN].gpio, 1);
		msleep(20);
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN2].gpio, 1);
		msleep(20);
		gpio_set_value(tsp_gpios[GPIO_TOUCH_RST].gpio, 1);
		msleep(150);
	} else {
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN2].gpio, 0);
		gpio_set_value(tsp_gpios[GPIO_TOUCH_EN].gpio, 0);
		gpio_set_value(tsp_gpios[GPIO_TOUCH_RST].gpio, 0);
		msleep(50);
	}
}

static int santos10_platform_init(void)
{
	return gpio_request_array(tsp_gpios, ARRAY_SIZE(tsp_gpios));
}

static int santos10_platform_deinit(void)
{
	gpio_free_array(tsp_gpios, ARRAY_SIZE(tsp_gpios));

	return 0;
}

#include <linux/input/synaptics.h>

static struct synaptics_fw_info santos10_tsp_fw_info = {
	.vendor			= "SY",
};

static struct sec_ts_platform_data santos10_synaptics_ts_pdata = {
	.fw_info		= &santos10_tsp_fw_info,
	.x_channel_no		= 1,	/* X channel line */
	.y_channel_no		= 1,	/* Y channel line */
	.x_pixel_size		= 1279,
	.y_pixel_size		= 799,
	.pivot			= false,
	.set_power		= santos10_syna_set_power,
	.platform_init		= santos10_platform_init,
	.platform_deinit	= santos10_platform_deinit,
};

void *santos10_synaptics_ts_platform_data(void *info)
{
	struct i2c_board_info *i2c_info = info;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(tsp_gpios); i++)
		tsp_gpios[i].gpio = get_gpio_by_name(tsp_gpios[i].label);

	if (sec_board_id == sec_id_santos103g)
		santos10_synaptics_ts_pdata.model_name = "P5200";
	else if (sec_board_id == sec_id_santos10wifi)
		santos10_synaptics_ts_pdata.model_name = "P5210";
	else if (sec_board_id == sec_id_santos10lte)
		santos10_synaptics_ts_pdata.model_name = "P5220"; /* NEED A CHECK */
	else
		santos10_synaptics_ts_pdata.model_name = NULL;

	santos10_synaptics_ts_pdata.i2c_address = (u16)i2c_info->addr;
	santos10_synaptics_ts_pdata.gpio_en = tsp_gpios[GPIO_TOUCH_EN].gpio;
	santos10_synaptics_ts_pdata.gpio_irq = tsp_gpios[GPIO_TOUCH_nINT].gpio;
	santos10_synaptics_ts_pdata.fw_name = "synaptics/p5100.fw";

	return &santos10_synaptics_ts_pdata;
}

static struct touch_key santos10_touch_keys[] = {
	{
		.name = "dummy_home1",
		.code = KEY_DUMMY_HOME1,
		.mask = 0x01,
	},
	{
		.name = "dummy_home2",
		.code = KEY_DUMMY_HOME2,
		.mask = 0x02,
	},
	{
		.name = "dummy_menu",
		.code = KEY_DUMMY_MENU,
		.mask = 0x04,
	},
	{
		.name = "dummy_back",
		.code = KEY_DUMMY_BACK,
		.mask = 0x08,
	},
	{
		.name = "menu",
		.code = KEY_MENU,
		.mask = 0x10,
	},
	{
		.name = "back",
		.code = KEY_BACK,
		.mask = 0x20,
	},
};

static struct sec_ts_platform_data santos10_mxt1188s_ts_pdata = {
	.fw_name		= "atmel/p5200.fw",
	.ext_fw_name		= "/mnt/sdcard/p5200.fw",
	.x_channel_no		= 27,	/* X channel line */
	.y_channel_no		= 43,	/* Y channel line */
				/* Y 44 line is attached to touch keys */
	.x_pixel_size		= 4095,
	.y_pixel_size		= 4095,
	.key			= santos10_touch_keys,
	.key_size		= ARRAY_SIZE(santos10_touch_keys),
	.set_power		= santos10_atmel_set_power,
	.platform_init		= santos10_platform_init,
	.platform_deinit	= santos10_platform_deinit,
};

void *santos10_mxt1188s_ts_platform_data(void *info)
{
	struct i2c_board_info *i2c_info = info;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(tsp_gpios); i++)
		tsp_gpios[i].gpio = get_gpio_by_name(tsp_gpios[i].label);

	if (sec_board_id == sec_id_santos103g)
		santos10_mxt1188s_ts_pdata.model_name = "P5200";
	else if (sec_board_id == sec_id_santos10wifi)
		santos10_mxt1188s_ts_pdata.model_name = "P5210";
	else if (sec_board_id == sec_id_santos10lte)
		santos10_mxt1188s_ts_pdata.model_name = "P5220";
	else
		santos10_mxt1188s_ts_pdata.model_name = NULL;

	santos10_mxt1188s_ts_pdata.i2c_address = (u16)i2c_info->addr;
	santos10_mxt1188s_ts_pdata.gpio_en = tsp_gpios[GPIO_TOUCH_EN].gpio;
	santos10_mxt1188s_ts_pdata.gpio_irq = tsp_gpios[GPIO_TOUCH_nINT].gpio;
	santos10_mxt1188s_ts_pdata.log_on =
					sec_debug_get_level() ? true : false;

	return &santos10_mxt1188s_ts_pdata;
}

/* touch key led */
enum {
	GPIO_TSK_EN = 0,
};

static struct gpio keyled_gpios[] = {
	[GPIO_TSK_EN] = {
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "TOUCHKEY_LED_EN",
	}
};

static void santos10_keyled_set_power(bool on)
{
	if (on)
		gpio_set_value(keyled_gpios[GPIO_TSK_EN].gpio, 1);
	else
		gpio_set_value(keyled_gpios[GPIO_TSK_EN].gpio, 0);
}

static ssize_t keyled_control(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	int input, ret;

	ret = kstrtoint(buf, 10, &input);
	if (ret < 0)
		return ret;

	if (input == 1)
		santos10_keyled_set_power(true);
	else
		santos10_keyled_set_power(false);

	return size;
}

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
								keyled_control);

#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)

enum {
	KEY_DUMMY_HOME2_OFFSET = 0,
	KEY_DUMMY_HOME1_OFFSET,
	KEY_DUMMY_MENU_OFFSET,
	KEY_DUMMY_BACK_OFFSET,
	KEY_MENU_OFFSET,
	KEY_BACK_OFFSET,
};

static ssize_t touchkey_dummy1_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int data;

	data = get_key_raw_data(dev, KEY_DUMMY_MENU_OFFSET);

	return sprintf(buf, "%d\n", data);
}

static ssize_t touchkey_dummy3_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int data;

	data = get_key_raw_data(dev, KEY_DUMMY_HOME2_OFFSET);

	return sprintf(buf, "%d\n", data);
}

static ssize_t touchkey_dummy4_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int data;

	data = get_key_raw_data(dev, KEY_DUMMY_HOME1_OFFSET);

	return sprintf(buf, "%d\n", data);
}

static ssize_t touchkey_dummy6_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int data;

	data = get_key_raw_data(dev, KEY_DUMMY_BACK_OFFSET);

	return sprintf(buf, "%d\n", data);
}

static ssize_t touchkey_menu_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int data;

	data = get_key_raw_data(dev, KEY_MENU_OFFSET);

	return sprintf(buf, "%d\n", data);
}

static ssize_t touchkey_back_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int data;

	data = get_key_raw_data(dev, KEY_BACK_OFFSET);

	return sprintf(buf, "%d\n", data);
}

static DEVICE_ATTR(touchkey_dummy_btn1, S_IRUGO, touchkey_dummy1_show, NULL);
static DEVICE_ATTR(touchkey_dummy_btn3, S_IRUGO, touchkey_dummy3_show, NULL);
static DEVICE_ATTR(touchkey_dummy_btn4, S_IRUGO, touchkey_dummy4_show, NULL);
static DEVICE_ATTR(touchkey_dummy_btn6, S_IRUGO, touchkey_dummy6_show, NULL);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
#endif

static struct attribute *touchkey_attributes[] = {
#if defined(CONFIG_TOUCHSCREEN_SEC_FACTORY_TEST)
	&dev_attr_touchkey_dummy_btn1.attr,
	&dev_attr_touchkey_dummy_btn3.attr,
	&dev_attr_touchkey_dummy_btn4.attr,
	&dev_attr_touchkey_dummy_btn6.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_back.attr,
#endif
	&dev_attr_brightness.attr,
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

static int __init santos10_create_keyled_dev(void)
{
	static dev_t keyled;

	santos10_mxt1188s_ts_pdata.key_dev =
		device_create(sec_class, NULL, keyled, NULL, "sec_touchkey");

	if (unlikely(!santos10_mxt1188s_ts_pdata.key_dev)) {
		pr_err("%s: failed to create sysfs (sec_touchkey)!\n",
								__func__);
		return -ENOMEM;
	}

	if (sysfs_create_group(&santos10_mxt1188s_ts_pdata.key_dev->kobj,
							&touchkey_attr_group)) {
		pr_err("%s: Failed to create sysfs (touchkey_attr_group)!\n",
								__func__);
		device_destroy(sec_class, keyled);
		return -ENOMEM;
	}

	return 0;
}

/* late initcall */
int __init sec_santos10_keyled_init(void)
{
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(keyled_gpios); i++)
		keyled_gpios[i].gpio = get_gpio_by_name(keyled_gpios[i].label);

	err = gpio_request_array(keyled_gpios, ARRAY_SIZE(keyled_gpios));
	if (unlikely(err))
		return -ENODEV;

	santos10_create_keyled_dev();

	return 0;
}
