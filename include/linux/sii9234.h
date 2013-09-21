/*
 * Copyright (C) 2011 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _SII9234_H_
#define _SII9234_H_


#ifdef __KERNEL__
struct sii9234_platform_data {
	u8 power_state;
	u8	swing_level;
	u8	factory_test;
	int ddc_i2c_num;
	int (*get_irq)(void);
	void (*init)(void);
	void (*mhl_sel)(bool enable);
	void (*hw_onoff)(bool on);
	void (*hw_reset)(void);
	void (*enable_vbus)(bool enable);
	void (*vbus_present)(bool on, int value);
#ifdef CONFIG_SAMSUNG_MHL_UNPOWERED
	int (*get_vbus_status)(void);
	void (*sii9234_otg_control)(bool onoff);
#endif
	struct i2c_client *mhl_tx_client;
	struct i2c_client *tpi_client;
	struct i2c_client *hdmi_rx_client;
	struct i2c_client *cbus_client;

	int (*reg_notifier)(struct notifier_block *nb);
	int (*unreg_notifier)(struct notifier_block *nb);
#ifdef CONFIG_EXTCON
	const char *extcon_name;
#endif
};

int acc_notify(int event);
extern u8 mhl_onoff_ex(bool onoff);
#endif
#endif /* _SII9234_H_ */
