/* arch/x86/platform/intel-mid/board-santos10-irled.c
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
#include <linux/delay.h>
#include <linux/sec_irled.h>

#include "sec_libs/sec_common.h"
#include "board-santos10.h"

enum {
	GPIO_IRDA_EN = 0,
	GPIO_IRDA_WAKE,
	GPIO_IRDA_IRQ,
};

static struct gpio irled_gpios[] = {
	[GPIO_IRDA_EN] = {
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "IRDA_EN",
	},
	[GPIO_IRDA_WAKE] = {
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "IRDA_WAKE",
	},
	[GPIO_IRDA_IRQ] = {
		.flags = GPIOF_IN,
		.label = "IRDA_IRQ",
	},
};

static void irda_wake_en(bool onoff)
{
	gpio_direction_output(irled_gpios[GPIO_IRDA_WAKE].gpio, onoff);
}

static void irda_vdd_onoff(bool onoff)
{
	if (onoff) {
		gpio_direction_output(irled_gpios[GPIO_IRDA_EN].gpio, onoff);
	} else {
		gpio_direction_output(irled_gpios[GPIO_IRDA_EN].gpio, onoff);
	}
}

static bool irda_irq_state(void)
{
	return !!gpio_get_value(irled_gpios[GPIO_IRDA_IRQ].gpio);
}

static struct sec_irled_platform_data mc96_pdata = {
	.ir_wake_en	= irda_wake_en,
	.ir_vdd_onoff	= irda_vdd_onoff,
	.ir_irq_state	= irda_irq_state,
};

static int __init santos10_irled_gpio_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(irled_gpios); i++)
		irled_gpios[i].gpio = get_gpio_by_name(irled_gpios[i].label);
	return gpio_request_array(irled_gpios, ARRAY_SIZE(irled_gpios));
}

void *santos10_irled_platform_data(void *info)
{
	return &mc96_pdata;
}

int __init sec_santos10_irled_init(void)
{
	int err;

	err = santos10_irled_gpio_init();
	if (unlikely(err)) {
		pr_err("(%s): can't request gpios!\n", __func__);
		return -ENODEV;
	}

	return 0;
}
