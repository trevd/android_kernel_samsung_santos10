
/* include/linux/i2c/tc35876x_sec.h
 * *
 * * Copyright (C) 2013 Samsung Electronics Co, Ltd.
 * *
 * * This software is licensed under the terms of the GNU General Public
 * * License version 2, as published by the Free Software Foundation, and
 * * may be copied, distributed, and modified under those terms.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * * GNU General Public License for more details.
 * */

#ifndef _TC35876X_SEC_H
#define _TC35876X_SEC_H

#include <linux/i2c.h>
struct tc35876x_platform_data {
	int gpio_lvds_reset;
	int (*platform_init)(void);
	void (*platform_deinit)(void);
	void (*power)(int on);
	void (*power_rest)(int on);
	void (*panel_power)(bool on);
	void (*backlight_power)(int on);
};

#endif /* _TC35876X_SEC_H */
