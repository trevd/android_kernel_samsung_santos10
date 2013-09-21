/* arch/x86/platform/intel-mid/board-clt3g-i2c.c
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
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <asm/intel-mid.h>

#include "board-clt3g.h"
#include "sec_libs/sec_common.h"

struct sec_i2c_gpio {
	const char *sda;
	const char *scl;
	struct platform_device *pdev;
};

static struct i2c_gpio_platform_data clt3g_gpio_i2c6_pdata = {
	/* .sda_pin     = (CHG_SDA_1.8V), */
	/* .scl_pin     = (CHG_SCL_1.8V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device clt3g_gpio_i2c6_pdev = {
	.name	= "i2c-gpio",
	.id	= 6,
	.dev = {
		.platform_data = &clt3g_gpio_i2c6_pdata,
	},
};

static struct sec_i2c_gpio clt3g_gpio_i2c6 __initdata = {
	.sda	= "CHG_SDA_1.8V",
	.scl	= "CHG_SCL_1.8V",
	.pdev	= &clt3g_gpio_i2c6_pdev,
};

static struct i2c_gpio_platform_data clt3g_gpio_i2c7_pdata = {
	/* .sda_pin     = (FUEL_SDA_1.8V), */
	/* .scl_pin     = (FUEL_SCL_1.8V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device clt3g_gpio_i2c7_pdev = {
	.name	= "i2c-gpio",
	.id	= 7,
	.dev = {
		.platform_data = &clt3g_gpio_i2c7_pdata,
	},
};

static struct sec_i2c_gpio clt3g_gpio_i2c7 __initdata = {
	.sda	= "FUEL_SDA_1.8V",
	.scl	= "FUEL_SCL_1.8V",
	.pdev	= &clt3g_gpio_i2c7_pdev,
};

static struct i2c_gpio_platform_data clt3g_gpio_i2c9_pdata = {
	/* .sda_pin     = (ADC_I2C_SDA_1.8V), */
	/* .scl_pin     = (ADC_I2C_SCL_1.8V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device clt3g_gpio_i2c9_pdev = {
	.name		= "i2c-gpio",
	.id		= 9,
	.dev = {
		.platform_data = &clt3g_gpio_i2c9_pdata,
	},
};

static struct sec_i2c_gpio clt3g_gpio_i2c9 __initdata = {
	.sda	= "ADC_I2C_SDA_1.8V",
	.scl	= "ADC_I2C_SCL_1.8V",
	.pdev	= &clt3g_gpio_i2c9_pdev,
};

static struct i2c_gpio_platform_data clt3g_gpio_i2c11_pdata = {
	/* .sda_pin     = (LVDS_I2C_SDA), */
	/* .scl_pin     = (LVDS_I2C_CLK), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device clt3g_gpio_i2c11_pdev = {
	.name	= "i2c-gpio",
	.id	= 11,
	.dev = {
		.platform_data = &clt3g_gpio_i2c11_pdata,
	},
};

static struct sec_i2c_gpio clt3g_gpio_i2c11 __initdata = {
	.sda	= "LVDS_I2C_SDA",
	.scl	= "LVDS_I2C_CLK",
	.pdev	= &clt3g_gpio_i2c11_pdev,
};

static struct sec_i2c_gpio *clt3g_gpio_i2c_devs[] __initdata = {
	&clt3g_gpio_i2c6,
	&clt3g_gpio_i2c7,
	&clt3g_gpio_i2c9,
	&clt3g_gpio_i2c11,
};

void __init sec_clt3g_i2c_init_rest(void)
{
	struct i2c_gpio_platform_data *pdata;
	struct platform_device *pdev;
	unsigned int i;
	int sda_pin;
	int scl_pin;

	for (i = 0; i < ARRAY_SIZE(clt3g_gpio_i2c_devs); i++) {
		if ((sec_board_rev < 3) &&
			(clt3g_gpio_i2c_devs[i]->pdev->id == 11))
			continue;
		sda_pin = get_gpio_by_name(clt3g_gpio_i2c_devs[i]->sda);
		scl_pin = get_gpio_by_name(clt3g_gpio_i2c_devs[i]->scl);
		if (unlikely(sda_pin < 0 || scl_pin < 0)) {
			pr_warning("(%s): can't find gpio : %s(%d) - %s(%d)\n",
				   __func__,
				   clt3g_gpio_i2c_devs[i]->sda, sda_pin,
				   clt3g_gpio_i2c_devs[i]->scl, scl_pin);
			continue;
		}
		pdev = clt3g_gpio_i2c_devs[i]->pdev;
		pdata = (struct i2c_gpio_platform_data *)
		    pdev->dev.platform_data;
		pdata->sda_pin = sda_pin;
		pdata->scl_pin = scl_pin;

		platform_device_register(pdev);
	}
}
