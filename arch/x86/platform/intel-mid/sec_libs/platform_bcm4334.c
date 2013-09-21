/*
 * Copyright (C) 2012 Google, Inc.
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

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <net/bluetooth/bluetooth.h>
#include <linux/platform_data/sec_ts.h>
#include <asm/intel-mid.h>

static struct platform_device bluetooth_platform_device = {
	.name		= "bcm_bluetooth",
	.id		= -1,
};

static struct platform_device *bt_devs[] __initdata = {
	&bluetooth_platform_device,
};

static int __init bcm4334_bt_init(void)
{
	int error_reg;
	pr_info("[BT] bcm4334_bt_init\n");

	error_reg = platform_add_devices(bt_devs, ARRAY_SIZE(bt_devs));

	return error_reg;
}

device_initcall(bcm4334_bt_init);

