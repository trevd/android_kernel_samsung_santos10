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

#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <asm/intel-mid.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#include <sound/asound.h>
#include "platform_wm8994.h"

static struct regulator_consumer_supply wm8994_avdd1_supply =
	REGULATOR_SUPPLY("AVDD1", "1-001a");

static struct regulator_consumer_supply wm8994_dcvdd_supply =
	REGULATOR_SUPPLY("DCVDD", "1-001a");

static struct regulator_init_data wm8994_ldo1_data = {
	.constraints	= {
		.name		= "AVDD1_3.0V",
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_avdd1_supply,
};

static struct regulator_init_data wm8994_ldo2_data = {
	.constraints	= {
		.name		= "DCVDD_1.0V",
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_dcvdd_supply,
};

static struct wm8994_drc_cfg wm8994_drc_cfgs[] = {
	{
		.name = "Default",
		.regs = {0x0098, 0x0845, 0x0, 0x0, 0x0}
	},
	{
		.name = "Speakers Media",
		.regs = {0x0050, 0x000C, 0x0018, 0x00E5, 0x0}
	},
	{
		.name = "Speakers Voip",
		.regs = {0x0050, 0x000C, 0x0018, 0x00E5, 0x0}
	}
};

static struct wm8994_override_params override_params = {
	.format[0] = SNDRV_PCM_FORMAT_S32_LE,
	.format[1] = SNDRV_PCM_FORMAT_S16_LE,
	.format[2] = 0,
	.slots = 2,
};

static struct wm8994_pdata pdata = {
	/* configure gpio1 function: 0x0001(Logic level input/output) */
	.gpio_defaults[0] = 0x0001,
	/* configure gpio3/4/5/7 function for AIF2 voice */
	.gpio_defaults[2] = 0x8100,
	.gpio_defaults[3] = 0x8100,
	.gpio_defaults[4] = 0x8100,
	.gpio_defaults[6] = 0x0100,
	/* configure gpio8/9/10/11 function for AIF3 BT */
	.gpio_defaults[7] = 0x8100,
	.gpio_defaults[8] = 0x0100,
	.gpio_defaults[9] = 0x0100,
	.gpio_defaults[10] = 0x0100,
	.ldo[0]	= { 0, &wm8994_ldo1_data }, // set actual value at wm8994_platform_data()
	.ldo[1]	= { 0, &wm8994_ldo2_data },
	.ldo_ena_always_driven = 1,
	.num_drc_cfgs = ARRAY_SIZE(wm8994_drc_cfgs),
	.drc_cfgs = wm8994_drc_cfgs,
	.override_params = &override_params,
};

void *wm8994_platform_data(void *info)
{
	pdata.ldo[0].enable	= get_gpio_by_name("CODEC_LDO_EN");

	return &pdata;
}
