/* arch/x86/platform/intel-mid/board-clt3g-hsu.c
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

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/serial_core.h>
#include <linux/suspend.h>

#include <asm/intel-mid.h>
#include <asm/intel_mid_hsu.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

#include "board-santos10.h"
#include "device_libs/platform_hsu.h"
#include "sec_libs/platform_bcm_bluetooth.h"
#include "sec_libs/sec_common.h"

enum {
	SANTOS10_BT = 0,
	SANTOS10_GPS,
	SANTOS10_DEBUG,
};

static struct mfld_hsu_info santos10_hsu_info[] = {
	[SANTOS10_BT] = {
		.id = SANTOS10_BT,
		.name = DEVICE_NAME_0,
	},
	[SANTOS10_GPS] = {
		.id = SANTOS10_GPS,
		.name = DEVICE_NAME_1,
	},
	[SANTOS10_DEBUG] = {
		.id = SANTOS10_DEBUG,
		.name = DEVICE_NAME_2,
	},
};

/* GPS */
static void __init santos10_hsu_info_hsu_gps(struct mfld_hsu_info *info)
{
	struct intel_muxtbl *muxtbl;

	/* RX */
	muxtbl = intel_muxtbl_find_by_net_name("GPS_UART_RXD");
	if (!muxtbl)
		return;
	info->rx_gpio = muxtbl->gpio.gpio;
	info->rx_alt = muxtbl->mux.alt_func;
	info->wake_gpio = muxtbl->gpio.gpio;
	/* TX */
	muxtbl = intel_muxtbl_find_by_net_name("GPS_UART_TXD");
	if (!muxtbl)
		return;
	info->tx_gpio = muxtbl->gpio.gpio;
	info->tx_alt = muxtbl->mux.alt_func;
	/* CTS */
	muxtbl = intel_muxtbl_find_by_net_name("GPS_UART_CTS");
	if (!muxtbl)
		return;
	info->cts_gpio = muxtbl->gpio.gpio;
	info->cts_alt = muxtbl->mux.alt_func;
	/* RTS */
	muxtbl = intel_muxtbl_find_by_net_name("GPS_UART_RTS");
	if (!muxtbl)
		return;
	info->rts_gpio = muxtbl->gpio.gpio;
	info->rts_alt = muxtbl->mux.alt_func;
}

/* BT */
static void __init santos10_hsu_info_hsu_bt(struct mfld_hsu_info *info)
{
	struct intel_muxtbl *muxtbl;

	/* RX */
	muxtbl = intel_muxtbl_find_by_net_name("BT_UART_RXD");
	info->rx_gpio = muxtbl->gpio.gpio;
	info->rx_alt = muxtbl->mux.alt_func;
	/* TX */
	muxtbl = intel_muxtbl_find_by_net_name("BT_UART_TXD");
	info->tx_gpio = muxtbl->gpio.gpio;
	info->tx_alt = muxtbl->mux.alt_func;
	/* CTS */
	muxtbl = intel_muxtbl_find_by_net_name("BT_UART_CTS");
	info->cts_gpio = muxtbl->gpio.gpio;
	info->cts_alt = muxtbl->mux.alt_func;
	/* RTS */
	muxtbl = intel_muxtbl_find_by_net_name("BT_UART_RTS");
	info->rts_gpio = muxtbl->gpio.gpio;
	info->rts_alt = muxtbl->mux.alt_func;

	info->wake_peer = bcm_bt_lpm_exit_lpm_locked;
}

/* AP */
static void __init santos10_hsu_info_hsu_debug(struct mfld_hsu_info *info)
{
	struct intel_muxtbl *muxtbl;

	/* RX */
	muxtbl = intel_muxtbl_find_by_net_name("AP_RXD");
	info->rx_gpio = muxtbl->gpio.gpio;
	info->rx_alt = muxtbl->mux.alt_func;
	info->wake_gpio = muxtbl->gpio.gpio;
	/* TX */
	muxtbl = intel_muxtbl_find_by_net_name("AP_TXD");
	info->tx_gpio = muxtbl->gpio.gpio;
	info->tx_alt = muxtbl->mux.alt_func;
}

void __init sec_santos10_hsu_init_rest(void)
{
	santos10_hsu_info_hsu_bt(&santos10_hsu_info[SANTOS10_BT]);
	santos10_hsu_info_hsu_gps(&santos10_hsu_info[SANTOS10_GPS]);
	santos10_hsu_info_hsu_debug(&santos10_hsu_info[SANTOS10_DEBUG]);

	platform_hsu_info = santos10_hsu_info;
	platform_hsu_num = ARRAY_SIZE(santos10_hsu_info);
}
