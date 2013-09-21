/*
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

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/irq.h>
#include <asm/intel-mid.h>

#define DEFAULT_UART_SEL_CP	0
#define DEFAULT_USB_SEL_AP	1

#define MASK_SWITCH_USB_AP	BIT(0)
#define MASK_SWITCH_UART_AP	BIT(1)

#define CLT_MAX77693_IRQ_BASE  448

static int max77693_set_safeout(int path)
{
	struct regulator *regulator;

	/* Superior board support only safeout2 ldo for CP USB. */
	regulator = regulator_get(NULL, "safeout2");	/* USB_VBUS_CP_4.9V */
	if (IS_ERR(regulator))
		return -ENODEV;

	if (path == PATH_USB_CP) {
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	} else {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
	}

	regulator_put(regulator);

	return 0;
}

static struct max77693_muic_platform_data max77693_muic = {
	.init_data		= NULL,
	.num_init_data		= 0,

	.usb_sel		= DEFAULT_USB_SEL_AP,
	.uart_sel		= DEFAULT_UART_SEL_CP,
	.support_cardock	= false,
	.support_jig_auto	= true,
	.support_btld_auto	= false,
	.set_safeout		= max77693_set_safeout,
};

static int __init max77693_save_init_switch_param(char *str)
{
	int init_switch_sel;
	int ret;

	ret = kstrtoint(str, 10, &init_switch_sel);
	if (ret < 0)
		return ret;

	if (init_switch_sel & MASK_SWITCH_USB_AP)
		max77693_muic.usb_sel = USB_SEL_AP;
	else
		max77693_muic.usb_sel = USB_SEL_CP;

	if (init_switch_sel & MASK_SWITCH_UART_AP)
		max77693_muic.uart_sel = UART_SEL_AP;
	else
		max77693_muic.uart_sel = UART_SEL_CP;

	return 0;
}
__setup("switch_sel=", max77693_save_init_switch_param);

static struct regulator_consumer_supply safeout1_supply[] = {
	REGULATOR_SUPPLY("safeout1", NULL),
};


static struct regulator_consumer_supply safeout2_supply[] = {
	REGULATOR_SUPPLY("safeout2", NULL),
};

static struct regulator_init_data safeout1_init_data = {
	.constraints    = {
		.name           = "safeout1 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on      = 0,
		.boot_on        = 1,
		.state_mem      = {
			.enabled = 1,
		},
	},
	.num_consumer_supplies  = ARRAY_SIZE(safeout1_supply),
	.consumer_supplies      = safeout1_supply,
};


static struct regulator_init_data safeout2_init_data = {
	.constraints    = {
		.name           = "safeout2 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on      = 0,
		.boot_on        = 0,
		.state_mem      = {
			.enabled = 1,
		},
	},
	.num_consumer_supplies  = ARRAY_SIZE(safeout2_supply),
	.consumer_supplies      = safeout2_supply,
};

static struct max77693_regulator_data max77693_regulator_data[] = {
	{MAX77693_ESAFEOUT1, &safeout1_init_data,},
	{MAX77693_ESAFEOUT2, &safeout2_init_data,},
};

static int max77693_get_irq_gpio(void)
{
	int irq_gpio;

	irq_gpio = get_gpio_by_name("IF_PMIC_IRQ");
	if (unlikely(irq_gpio < 0)) {
		pr_err("%s: failed to get max77693 irq gpio (ret=%d)\n",
				__func__, irq_gpio);
		return -1;
	}

	return irq_gpio;
}

static struct max77693_platform_data clt_max77693_pdata = {
	.wakeup			= 1,
	.get_irq_gpio		= max77693_get_irq_gpio,
	.irq_base		= NR_VECTORS + CPU_VECTOR_LIMIT, /* 512 */

	.muic_data		= &max77693_muic,
	.regulator_data		= max77693_regulator_data,
	.num_regulators		= ARRAY_SIZE(max77693_regulator_data),
#if defined(CONFIG_CHARGER_MAX77693)
	.charger_data   = &sec_battery_pdata,
#endif
};

void *max77693_platform_data(void *info)
{
	return &clt_max77693_pdata;
}

void max77693_device_handler(struct sfi_device_table_entry *pentry,
				struct devs_id *dev)
{
	struct i2c_board_info i2c_info;
	void *pdata = NULL;

	memset(&i2c_info, 0, sizeof(i2c_info));
	strncpy(i2c_info.type, pentry->name, SFI_NAME_LEN);
	i2c_info.irq = 345;	/* base + offset = 256 + 89 */
	i2c_info.addr = pentry->addr;
	pr_info("I2C bus = %d, name = %16.16s, irq = 0x%2x, addr = 0x%x\n",
		pentry->host_num,
		i2c_info.type,
		i2c_info.irq,
		i2c_info.addr);
	pdata = dev->get_platform_data(&i2c_info);
	i2c_info.platform_data = pdata;

	i2c_register_board_info(pentry->host_num, &i2c_info, 1);
}

int max77693_haptic_enable(bool on)
{
	int ret;
	u8 value = MOTOR_LRA | EXT_PWM | DIVIDER_32;
	u8 lscnfg_val = 0x00;

	if (on) {
		value |= MOTOR_EN;
		lscnfg_val = 0x80;
	}

	if (!clt_max77693_pdata.regmap_max77693 ||
				!clt_max77693_pdata.regmap_max77693_haptic)
		return -ENODATA;

	ret = max77693_update_reg(clt_max77693_pdata.regmap_max77693,
			MAX77693_PMIC_REG_LSCNFG, lscnfg_val, 0x80);
	if (ret) {
		pr_err("max77693: pmic i2c error %d\n", ret);
		return ret;
	}

	ret = max77693_write_reg(clt_max77693_pdata.regmap_max77693_haptic,
			MAX77693_HAPTIC_REG_CONFIG2, value);
	if (ret) {
		pr_err("max77693: haptic i2c error %d\n", ret);
		return ret;
	}

	return 0;
}
