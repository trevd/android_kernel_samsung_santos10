/* arch/x86/platform/intel-mid/board-santos10.h
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

#ifndef __BOARD_SANTOS10_H__
#define __BOARD_SANTOS10_H__

#include <linux/interrupt.h>
#include <linux/sfi.h>

#include <asm/intel-mid.h>

extern unsigned int sec_id_santos103g;
extern unsigned int sec_id_santos10wifi;
extern unsigned int sec_id_santos10lte;

/** @category SFI */
void sec_santos10_dev_init_early(void);
struct devs_id *sec_santos10_get_device_ptr(void);
struct sfi_device_table_entry *sec_santos10_get_sfi_table_ptr(void);
int sec_santos10_get_sfi_table_num(void);

/** @category UART */
void sec_santos10_hsu_init_rest(void);

/** @category I2C-GPIO */
void sec_santos10_i2c_init_rest(void);
int sec_santos10_hw_i2c_init(void);

/** @category BUTTON / TSP */
extern void *santos10_synaptics_ts_platform_data(void *info);
extern void *santos10_mxt1188s_ts_platform_data(void *info);
int sec_santos10_keyled_init(void);
void sec_santos10_input_init_early(void);
int sec_santos10_button_init(void);

/** @category CONNECTOR */
int sec_santos10_usb_init(void);
int sec_santos10_max77693_gpio_init(void);

/** @category FUELGAUGE */
int sec_santos10_battery_init(void);
void *max17050_platform_data(void *info);

#if defined(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_charging_common.h>
extern sec_battery_platform_data_t sec_battery_pdata;
extern int current_cable_type;
#endif

/** @category DISPLAY */
extern void *tc35876x_platform_data(void *info);
extern void *cmc624_platform_data(void *info);
extern int sec_santos10_backlight_init(void);
extern void __init *scu_log_platform_data(void *info);

/** @category MODEM */
int sec_santos103g_modems_init(void);
int sec_santos10lte_modems_init(void);
void sec_santos10wifi_no_modems_init_rest(void);

/** @category Vibrator */
int max77693_haptic_enable(bool);
int sec_santos10_vibrator_init(void);
void sec_santos10wifi_no_vibrator_init_rest(void);

/** @category WLAN */
int sec_santo10_brcm_wlan_init(void);

/** @category IrLED */
extern void *santos10_irled_platform_data(void *info);
int sec_santos10_irled_init(void);

/** @category Thermister */
int sec_santos10_thermistor_init(void);

#endif /* __BOARD_SANTOS10_H__ */
