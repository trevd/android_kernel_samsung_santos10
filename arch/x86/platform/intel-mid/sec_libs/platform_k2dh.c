/*
 * Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/sensor/yas.h>
#include <linux/sensor/k2dh.h>
#include <linux/sensor/sensors_axes_s16.h>

static u8 stm_get_position(void);

static struct acc_platform_data accel_pdata = {
	.accel_get_position	= stm_get_position,
	.select_func = select_func_s16,
};

static u8 stm_get_position(void)
{
	return TYPE4_BOTTOM_UPPER_RIGHT;
}

void __init *acc_platform_data(void *info)
{
	struct i2c_board_info *i2c_info = info;

	pr_debug("%s addr : %x\n", __func__, i2c_info->addr);

	return &accel_pdata;
}
