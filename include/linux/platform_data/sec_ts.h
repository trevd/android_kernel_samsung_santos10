/* include/linux/sec_ts.h
 * platform data structure for Samsung common touch driver
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SEC_TS_H
#define _LINUX_SEC_TS_H

#define SEC_TS_NAME "sec_touchscreen"

#include <linux/gpio.h>

extern struct class *sec_class;

enum {
	CABLE_TA = 0,
	CABLE_USB,
	CABLE_NONE,
};

enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
};

/**
 * struct touch_key - represent a intergrated touch key in touch IC
 * name : key name
 * code : key code reprented in input.h
 * mask : raw data(from IC) masking value
 * rx_channel : physical rx channel number connencted to the touch panel
 * tx_channel : physical tx channel number connencted to the touch panel
 */
struct touch_key {
	char *name;
	u32 code;
	u8 mask;
	u32 rx_channel;
	u32 tx_channel;
};

/**
 * struct sec_ts_platform_data - represent specific touch device
 * @model_name : name of device name
 * @panel_name : name of sensor panel name
 * @fw_name : intenal firmware file name
 * @fw_info : represent device firmware informations
 * @ext_fw_name : external(ex. sd card or interal ROM) firmware file name
 * with path
 * @fw_version : latest version of firmware file. To get this from firmware data
 * is recommended than hard coding.
 * @key : informaition of intergrated touch key. it usually using LCD panel.
 * @key_size : number of touch keys.
 * @key_dev : device pointer for sec factory test sysfs and led control.
 * @i2c_address : receive i2c address from platform data
 * @x_channel_no : vertical channel number of touch sensor
 * @y_channel_no : horizontal channel number of touch sensor
 * @x_pixel_size : maximum of x axis pixel size
 * @y_pixel_size : maximum of y axis pixel size
 * @pivot : change x, y coordination
 * @log_on : print touch UP, DOWN log
 * @ta_state : represent of charger connect state
 * @driver_data : pointer back to the struct class that this structure is
 * associated with.
 * @private_data : pointer that needed by vendor specific information
 * @gpio_irq : physical gpio using to interrupt
 * @gpio_en : physical gpio using to IC enable
 * @gpio_scl : physical gpio using to i2c communication
 * @gpio_sda : physical gpio using to i2c communication
 * @set_ta_mode : callback function when TA, USB connected or disconnected
 * @set_power : control touch screen IC power
 */
struct sec_ts_platform_data {
	const char *model_name;
	const char *panel_name;
	const char *fw_name;
	const void *fw_info;
	const char *ext_fw_name;
	const u32 fw_version;
	const struct touch_key *key;
	size_t key_size;
	struct device *key_dev;
	unsigned short i2c_address;
	int x_channel_no;
	int y_channel_no;
	int x_pixel_size;
	int y_pixel_size;
	bool pivot;
	bool log_on;
	int ta_state;
	void *driver_data;
	void *private_data;
	u32 gpio_irq;
	u32 gpio_en;
	u32 gpio_scl;
	u32 gpio_sda;
	void (*set_ta_mode)(int *);
	void (*set_power)(bool);
	int (*platform_init)(void);
	int (*platform_deinit)(void);
};

/* dummy touch key define */
#define KEY_DUMMY_HOME1		249
#define KEY_DUMMY_HOME2		250
#define KEY_DUMMY_MENU		251
#define KEY_DUMMY_HOME		252
#define KEY_DUMMY_BACK		253

#endif
