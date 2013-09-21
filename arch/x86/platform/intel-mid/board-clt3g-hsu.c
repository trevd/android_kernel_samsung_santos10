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

#include <asm/intel-mid.h>
#include <asm/intel_mid_hsu.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

#include "board-clt3g.h"
#include "device_libs/platform_hsu.h"
#include "sec_libs/sec_common.h"

enum {
	CLT3G_GPS = 0,
	CLG3G_BT,
	CLG3G_DEBUG,
};

static struct mfld_hsu_info clt3g_hsu_info[] = {
	[CLT3G_GPS] = {
		.id = CLT3G_GPS,
		.name = DEVICE_NAME_0,
	},
	[CLG3G_BT] = {
		.id = CLG3G_BT,
		.name = DEVICE_NAME_1,
	},
	[CLG3G_DEBUG] = {
		.id = CLG3G_DEBUG,
		.name = DEVICE_NAME_2,
	},

};

/* GPS */
static void __init clt3g_hsu_info_hsu_gps(struct mfld_hsu_info *info)
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
static void __init clt3g_hsu_info_hsu_bt(struct mfld_hsu_info *info)
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
/* FIXME: enable this after implemented */
#if 0
	info->wake_peer = bcm_bt_lpm_exit_lpm_locked;
#endif
}

/* AP */
static void __init clt3g_hsu_info_hsu_debug(struct mfld_hsu_info *info)
{
	struct intel_muxtbl *muxtbl;

	/* RX */
	muxtbl = intel_muxtbl_find_by_net_name("AP_RXD");
	info->rx_gpio = muxtbl->gpio.gpio;
	info->rx_alt = muxtbl->mux.alt_func;
	info->wake_gpio = muxtbl->gpio.gpio;
/* FIXME: when TX is configured, CON_PRINTBUFFER does not work correctly..
 * Intel's BUG? */
#if 0
	/* TX */
	muxtbl = intel_muxtbl_find_by_net_name("AP_TXD");
	info->tx_gpio = muxtbl->gpio.gpio;
	info->tx_alt = muxtbl->mux.alt_func;
#endif
}

void __init sec_clt3g_hsu_init_rest(void)
{
	clt3g_hsu_info_hsu_gps(&clt3g_hsu_info[CLT3G_GPS]);
	clt3g_hsu_info_hsu_bt(&clt3g_hsu_info[CLG3G_BT]);
	clt3g_hsu_info_hsu_debug(&clt3g_hsu_info[CLG3G_DEBUG]);

	platform_hsu_info = clt3g_hsu_info;
	platform_hsu_num = ARRAY_SIZE(clt3g_hsu_info);
}
