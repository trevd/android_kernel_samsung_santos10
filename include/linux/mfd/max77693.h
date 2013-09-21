/*
 * max77693.h - Driver for the Maxim 77693
 *
 *  Copyright (C) 2012 Samsung Electrnoics
 *  SangYoung Son <hello.son@samsung.com>
 *
 * This program is not provided / owned by Maxim Integrated Products.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max8997.h
 *
 * MAX77693 has PMIC, Charger, Flash LED, Haptic, MUIC devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */

#ifndef __LINUX_MFD_MAX77693_H
#define __LINUX_MFD_MAX77693_H

#include <linux/regulator/consumer.h>

#include <linux/battery/charger/max77693_charger.h>
#include <linux/battery/sec_charging_common.h>

#define MOTOR_LRA			(1<<7)
#define MOTOR_EN			(1<<6)
#define INT_PWM				(1<<5)
#define EXT_PWM				(0<<5)
#define DIVIDER_256			DIVIDER_128 | DIVIDER_64
#define DIVIDER_128			(1<<1)
#define DIVIDER_64			(1<<0)
#define DIVIDER_32			(0<<0)

/* MAX77686 regulator IDs */
enum max77693_regulators {
	MAX77693_ESAFEOUT1 = 0,
	MAX77693_ESAFEOUT2,

	MAX77693_CHARGER,

	MAX77693_REG_MAX,
};

struct max77693_regulator_data {
	int id;
	struct regulator_init_data *initdata;
};

struct max77693_reg_data {
	u8 addr;
	u8 data;
};

enum {
	PATH_OPEN = 0,
	PATH_USB_AP,
	PATH_AUDIO,
	PATH_UART_AP,
	PATH_USB_CP,
	PATH_UART_CP,
};

enum {
	USB_SEL_AP = 0,
	USB_SEL_CP,
};

enum {
	UART_SEL_AP = 0,
	UART_SEL_CP,
};

struct max77693_muic_platform_data {
	struct max77693_reg_data *init_data;
	int num_init_data;

	int usb_sel;
	int uart_sel;
	bool support_cardock;
	bool support_jig_auto;
	bool support_btld_auto;

	void (*init_cb) (void);
	int (*set_safeout) (int path);
};

struct max77693_platform_data {
	/* IRQ */
	int (*get_irq_gpio) (void);
#if defined(CONFIG_CHARGER_MAX77803)
	int wc_irq_gpio;
#endif
	int irq_base;
	int wakeup;

	/* muic data */
	struct max77693_muic_platform_data *muic_data;
	/* pmic data */
	struct max77693_regulator_data *regulator_data;
	int num_regulators;
#if defined(CONFIG_CHARGER_MAX77693)
	sec_battery_platform_data_t *charger_data;
#endif
	struct regmap *regmap_max77693;
	struct regmap *regmap_max77693_haptic;
};
#endif	/* __LINUX_MFD_MAX77693_H */
