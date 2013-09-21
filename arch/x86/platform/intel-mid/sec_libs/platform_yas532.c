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
#include <linux/i2c.h>
#include <linux/sensor/yas.h>

#include "platform_yas532.h"

#define YAS_TA_OFFSET			{0, 0, 0}
#define YAS_USB_OFFSET			{0, 0, 0}
#define YAS_FULL_OFFSET			{0, 0, 0}

struct mag_platform_data yas532_pdata = {
	.offset_enable	= 0,
	.chg_status	= CABLE_TYPE_NONE,
	.ta_offset.v	= YAS_TA_OFFSET,
	.usb_offset.v	= YAS_USB_OFFSET,
	.full_offset.v	= YAS_FULL_OFFSET,
};

void __init *mag_platform_data(void *info)
{
	struct i2c_board_info *i2c_info = info;

	pr_debug("%s addr : %x\n", __func__, i2c_info->addr);

	return &yas532_pdata;
}
