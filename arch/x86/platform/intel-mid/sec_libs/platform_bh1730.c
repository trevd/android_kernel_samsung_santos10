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
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/sensor/bh1730.h>
#include <asm/intel-mid.h>
#include "platform_bh1730.h"

static unsigned int gpio_als_nrst;

static int bh1730fvc_light_sensor_reset(void)
{
	int err;

	pr_info("bh1730_light_sensor_reset !!\n");

	err = gpio_request(gpio_als_nrst, "LIGHT_SENSOR_RESET");
	if (err) {
		pr_err("failed to request reset, %d\n", err);
		return err;
	}

	gpio_direction_output(gpio_als_nrst, 1);
	gpio_set_value(gpio_als_nrst, 0);
	udelay(2);
	gpio_set_value(gpio_als_nrst, 1);

	gpio_free(gpio_als_nrst);

	return 0;
}

static struct bh1730fvc_platform_data bh1730_pdata = {
	.reset = bh1730fvc_light_sensor_reset,
};

void __init *bh1730fvc_platform_data(void *info)
{
	struct i2c_board_info *i2c_info = info;

	gpio_als_nrst = get_gpio_by_name("ALS_NRST");
	pr_info("%s addr : %x  gpio : %d\n", __func__,
			i2c_info->addr, gpio_als_nrst);
	if (gpio_als_nrst == -1)
		return NULL;

	return &bh1730_pdata;
}
