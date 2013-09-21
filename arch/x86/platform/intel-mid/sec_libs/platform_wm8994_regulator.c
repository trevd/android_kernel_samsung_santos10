/* arch/x86/platform/intel-mid/sec_libs/platform_wm8994_regulator.c
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>

/***********WM89941 REGUATOR platform data*************/
static struct regulator_consumer_supply vwm89941_consumer[] = {
	REGULATOR_SUPPLY("DBVDD", "1-001a"),
	REGULATOR_SUPPLY("AVDD2", "1-001a"),
	REGULATOR_SUPPLY("CPVDD", "1-001a"),
};

static struct regulator_init_data vwm89941_data = {
		.constraints = {
			.always_on = 1,
		},
		.num_consumer_supplies	=	ARRAY_SIZE(vwm89941_consumer),
		.consumer_supplies	=	vwm89941_consumer,
};

static struct fixed_voltage_config vwm89941_config = {
	.supply_name	= "VCC_1.8V_PDA",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &vwm89941_data,
};

static struct platform_device vwm89941_device = {
	.name = "reg-fixed-voltage",
	.id = 0,
	.dev = {
		.platform_data = &vwm89941_config,
	},
};

/***********WM89942 REGUATOR platform data*************/
static struct regulator_consumer_supply vwm89942_consumer[] = {
	REGULATOR_SUPPLY("SPKVDD1", "1-001a"),
	REGULATOR_SUPPLY("SPKVDD2", "1-001a"),
};

static struct regulator_init_data vwm89942_data = {
		.constraints = {
			.always_on = 1,
		},
		.num_consumer_supplies	=	ARRAY_SIZE(vwm89942_consumer),
		.consumer_supplies	=	vwm89942_consumer,
};

static struct fixed_voltage_config vwm89942_config = {
	.supply_name	= "V_BAT",
	.microvolts	= 3700000,
	.gpio		= -EINVAL,
	.init_data  = &vwm89942_data,
};

static struct platform_device vwm89942_device = {
	.name = "reg-fixed-voltage",
	.id = 1,
	.dev = {
		.platform_data = &vwm89942_config,
	},
};

static struct platform_device *regulator_devices[] __initdata = {
	&vwm89941_device,
	&vwm89942_device,
};

static int __init wm8994_regulator_init(void)
{
	platform_add_devices(regulator_devices,
		ARRAY_SIZE(regulator_devices));
	return 0;
}
device_initcall(wm8994_regulator_init);