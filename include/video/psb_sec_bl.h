/* include/linux/psb_sec_bl.h
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

#ifndef _PSB_SEC_BL_H
#define _PSB_SEC_BL_H

struct psb_backlight_platform_data {
	void (*backlight_power)(int on);
	int (*backlight_set_pwm)(int level);
	void (*set_auto_brt)(int auto_brightness);
};

#endif /* _PSB_SEC_BL_H */
