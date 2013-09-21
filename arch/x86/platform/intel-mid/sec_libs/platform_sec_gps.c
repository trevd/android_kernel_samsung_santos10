/* arch/x86/platform/intel-mid/sec_libs/platform_sec_gps.c
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>

#include <asm/intel-mid.h>

#include "sec_common.h"

static int __init platform_sec_gps_init(void)
{
	struct device *gps_dev;
	struct gpio gps_gpios[] = {
		{
			.flags	= GPIOF_OUT_INIT_LOW,
			.label	= "GPS_PWR_EN",
		},
		{	/* some devices don't have this */
			.flags	= GPIOF_OUT_INIT_HIGH,
			.label	= "GPS_nRST",
		},
	};
	unsigned int not_used = 0;
	int err;
	int i;

	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (unlikely(!gps_dev)) {
		pr_err("(%s): failed to create sysfs (gps)!\n", __func__);
		goto __err_dev;
	}

	for (i = 0; i < ARRAY_SIZE(gps_gpios); i++) {
		gps_gpios[i].gpio = get_gpio_by_name(gps_gpios[i].label);
		if (gps_gpios[i].gpio == -1) {
			not_used++;
			continue;
		}

		err = gpio_request_one(gps_gpios[i].gpio, gps_gpios[i].flags,
				gps_gpios[i].label);
		if (unlikely(err < 0)) {
			not_used++;
			continue;
		}
		gpio_export(gps_gpios[i].gpio, 1);
		gpio_export_link(gps_dev, gps_gpios[i].label,
				gps_gpios[i].gpio);
	}

	if (unlikely(ARRAY_SIZE(gps_gpios) == not_used)) {
		pr_err("(%s): failed to set-up gps-gpios!\n", __func__);
		goto __err_gpio;
	}

	return 0;

__err_gpio:
	device_destroy(sec_class, 0);
__err_dev:
	return 0;/* -ENODEV; */
}
rootfs_initcall(platform_sec_gps_init);
