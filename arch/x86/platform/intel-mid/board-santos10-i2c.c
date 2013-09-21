/* arch/x86/platform/intel-mid/board-santos10-i2c.c
 *
 * Copyright (C) 2013 Samsung Electronics Co, Ltd.
 *
 * Based on 'board-clt3g-i2c.c'
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
#include <linux/kallsyms.h>
#include <linux/lnw_gpio.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>

#include <asm/intel-mid.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

#include "board-santos10.h"
#include "sec_libs/sec_common.h"

#include "../../../drivers/i2c/busses/i2c-designware-core.h"

struct sec_i2c_gpio {
	const char *sda;
	const char *scl;
	struct platform_device *pdev;
};

static struct i2c_gpio_platform_data santos10_gpio_i2c3_pdata = {
	/* .sda_pin     = (MHL_DSDA_1.2V), */
	/* .scl_pin     = (MHL_DSCL_1.2V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c3_pdev = {
	.name	= "i2c-gpio",
	.id	= 3,
	.dev = {
		.platform_data = &santos10_gpio_i2c3_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c3 __initdata = {
	.sda	= "MHL_DSDA_1.2V",
	.scl	= "MHL_DSCL_1.2V",
	.pdev	= &santos10_gpio_i2c3_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c5_pdata = {
	/* .sda_pin	= (SENSOR_I2C_SDA), */
	/* .scl_pin	= (SENSOR_I2C_SCL), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c5_pdev = {
	.name	= "i2c-gpio",
	.id	= 5,
	.dev  = {
		.platform_data = &santos10_gpio_i2c5_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c5 __initdata = {
	.sda	= "SENSOR_I2C_SDA",
	.scl	= "SENSOR_I2C_SCL",
	.pdev = &santos10_gpio_i2c5_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c6_pdata = {
	/* .sda_pin     = (IF_PMIC_SDA), */
	/* .scl_pin     = (IF_PMIC_SCL), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c6_pdev = {
	.name	= "i2c-gpio",
	.id	= 6,
	.dev = {
		.platform_data = &santos10_gpio_i2c6_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c6 __initdata = {
	.sda	= "IF_PMIC_SDA",
	.scl	= "IF_PMIC_SCL",
	.pdev	= &santos10_gpio_i2c6_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c7_pdata = {
	/* .sda_pin     = (FUEL_SDA_1.8V), */
	/* .scl_pin     = (FUEL_SCL_1.8V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c7_pdev = {
	.name	= "i2c-gpio",
	.id	= 7,
	.dev = {
		.platform_data = &santos10_gpio_i2c7_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c7 __initdata = {
	.sda	= "FUEL_SDA_1.8V",
	.scl	= "FUEL_SCL_1.8V",
	.pdev	= &santos10_gpio_i2c7_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c10_pdata = {
	/* .sda_pin	= (MHL_SDA_1.8V), */
	/* .scl_pin	= (MHL_SCL_1.8V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c10_pdev = {
	.name		= "i2c-gpio",
	.id		= 10,
	.dev = {
		.platform_data = &santos10_gpio_i2c10_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c10 __initdata = {
	.sda	= "MHL_SDA_1.8V",
	.scl	= "MHL_SCL_1.8V",
	.pdev	= &santos10_gpio_i2c10_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c11_pdata = {
	/* .sda_pin     = (LVDS_I2C_SDA), */
	/* .scl_pin     = (LVDS_I2C_CLK), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c11_pdev = {
	.name	= "i2c-gpio",
	.id	= 11,
	.dev = {
		.platform_data = &santos10_gpio_i2c11_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c11 __initdata = {
	.sda	= "LVDS_I2C_SDA",
	.scl	= "LVDS_I2C_CLK",
	.pdev	= &santos10_gpio_i2c11_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c12_pdata = {
	/* .sda_pin	= (GRIP_SDA), */
	/* .scl_pin	= (GRIP_SCL), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c12_pdev = {
	.name	= "i2c-gpio",
	.id	= 12,
	.dev = {
		.platform_data = &santos10_gpio_i2c12_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c12 __initdata = {
	.sda	= "GRIP_SDA",
	.scl	= "GRIP_SCL",
	.pdev	= &santos10_gpio_i2c12_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c13_pdata = {
	/* .sda_pin	= (IRDA_SDA), */
	/* .scl_pin	= (IRDA_SCL), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c13_pdev = {
	.name	= "i2c-gpio",
	.id	= 13,
	.dev  = {
		.platform_data = &santos10_gpio_i2c13_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c13 __initdata = {
	.sda	= "IRDA_SDA",
	.scl	= "IRDA_SCL",
	.pdev = &santos10_gpio_i2c13_pdev,
};

static struct i2c_gpio_platform_data santos10_gpio_i2c9_pdata = {
	/* .sda_pin     = (ADC_I2C_SDA_1.8V), */
	/* .scl_pin     = (ADC_I2C_SCL_1.8V), */
	.udelay		= 5,
	.timeout	= 0,
};

static struct platform_device santos10_gpio_i2c9_pdev = {
	.name		= "i2c-gpio",
	.id		= 9,
	.dev = {
		.platform_data = &santos10_gpio_i2c9_pdata,
	},
};

static struct sec_i2c_gpio santos10_gpio_i2c9 __initdata = {
	.sda	= "ADC_I2C_SDA_1.8V",
	.scl	= "ADC_I2C_SCL_1.8V",
	.pdev	= &santos10_gpio_i2c9_pdev,
};

static struct sec_i2c_gpio *santos10_gpio_i2c_devs[] __initdata = {
	&santos10_gpio_i2c3,
	&santos10_gpio_i2c5, /* Sensors */
	&santos10_gpio_i2c6,
	&santos10_gpio_i2c7,
	&santos10_gpio_i2c9,
	&santos10_gpio_i2c10,
	&santos10_gpio_i2c11,
	/* santos10-3g/wifi r03
	 * santos10-LTE r03 */
	&santos10_gpio_i2c12, /* GRIP Sensor*/
	&santos10_gpio_i2c13, /* IRDA */
};

void __init santos10_i2c_disable_i2c12(void)
{
	char *grip_gpios[] = {
		"GRIP_SDA", "GRIP_SCL",
		/* children of GRIP sensor */
		"RF_TOUCH", "ADJ_DET_AP",
	};
	unsigned int i;
	struct intel_muxtbl *muxtbl;

	for (i = 0; i < ARRAY_SIZE(grip_gpios); i++) {
		muxtbl = intel_muxtbl_find_by_net_name(grip_gpios[i]);
		if (unlikely(!muxtbl))
			continue;
		muxtbl->mux.alt_func = LNW_GPIO;
		muxtbl->mux.pull = DOWN_20K;
		muxtbl->mux.pin_direction = INPUT_ONLY;
		muxtbl->mux.open_drain = OD_ENABLE;
		muxtbl->gpio.label = kasprintf(GFP_KERNEL, "%s.nc",
				muxtbl->gpio.label);
	}

	/* FIXME: in spite of changing the label of muxtbl, the crc32 value
	 * of the update muxtbl still has old one of the label before
	 * changed. this is a current limitation of the implementation, we
	 * should rename labels which are not used for i2c. */
	santos10_gpio_i2c12.sda = "GRIP_SDA.nc";
	santos10_gpio_i2c12.scl = "GRIP_SCL.nc";
}

void __init sec_santos10_i2c_init_rest(void)
{
	struct i2c_gpio_platform_data *pdata;
	struct platform_device *pdev;
	unsigned int nr_i2c = ARRAY_SIZE(santos10_gpio_i2c_devs);
	unsigned int i;
	int sda_pin;
	int scl_pin;

	if (sec_board_id == sec_id_santos10wifi)
		santos10_i2c_disable_i2c12();

	for (i = 0; i < nr_i2c; i++) {
		sda_pin = get_gpio_by_name(santos10_gpio_i2c_devs[i]->sda);
		scl_pin = get_gpio_by_name(santos10_gpio_i2c_devs[i]->scl);
		if (unlikely(sda_pin < 0 || scl_pin < 0)) {
			pr_warning("(%s): can't find gpio : %s(%d) - %s(%d)\n",
				   __func__,
				   santos10_gpio_i2c_devs[i]->sda, sda_pin,
				   santos10_gpio_i2c_devs[i]->scl, scl_pin);
			continue;
		}
		pdev = santos10_gpio_i2c_devs[i]->pdev;
		pdata = (struct i2c_gpio_platform_data *)
		    pdev->dev.platform_data;
		pdata->sda_pin = sda_pin;
		pdata->scl_pin = scl_pin;

		platform_device_register(pdev);
	}
}

static void __init santos10_i2c3_init(void)
{
	struct pci_device_id *clt_i2c = (struct pci_device_id *)
		kallsyms_lookup_name("i2c_designware_pci_ids");

	if (unlikely(!clt_i2c)) {
		pr_err("(%s): can't find PCI device table for i2c!\n",
				__func__);
		return;		/* ignore error */
	}

	/* lol.. this is a crazy hack for MHL */
	clt_i2c[12 /* cloverview_3 */].driver_data = 256;
			/* make larger than ARRAY_SIZE(dw_pci_controllers) */

	lnw_gpio_set_alt(santos10_gpio_i2c3_pdata.sda_pin, LNW_GPIO);
	lnw_gpio_set_alt(santos10_gpio_i2c3_pdata.scl_pin, LNW_GPIO);
}

static void __init santos10_i2c5_init(void)
{
	struct pci_device_id *clt_i2c = (struct pci_device_id *)
		kallsyms_lookup_name("i2c_designware_pci_ids");

	if (unlikely(!clt_i2c)) {
		pr_err("(%s): can't find PCI device table for i2c!\n",
				__func__);
		return;		/* ignore error */
	}

	/* lol.. this is a crazy hack for MHL */
	clt_i2c[14 /* cloverview_3 */].driver_data = 256;
			/* make larger than ARRAY_SIZE(dw_pci_controllers) */

	lnw_gpio_set_alt(santos10_gpio_i2c5_pdata.sda_pin, LNW_GPIO);
	lnw_gpio_set_alt(santos10_gpio_i2c5_pdata.scl_pin, LNW_GPIO);
}

/* rootfs_initcall */
int __init sec_santos10_hw_i2c_init(void)
{
	santos10_i2c3_init();
	santos10_i2c5_init();

	return 0;
}
