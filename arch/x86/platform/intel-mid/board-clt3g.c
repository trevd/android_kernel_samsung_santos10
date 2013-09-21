/* arch/x86/platform/intel-mid/board-clt3g.h
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
#include <linux/kernel.h>
#include <linux/crc32.h>
#include <linux/gpio.h>
#include <linux/reboot.h>

#include <asm/sec_muxtbl.h>

#include "sec_libs/customize-board.h"
#include "sec_libs/sec_common.h"
#include "sec_libs/sec_debug.h"

#include "board-clt3g.h"

static void clt3g_power_off_charger(void)
{
	pr_emerg("Rebooting into bootloader for charger.\n");
	sec_common_reboot_call(SYS_RESTART, "charging");
}

static unsigned int gpio_ta_nconnected;

static int clt3g_reboot_call(struct notifier_block *this,
		unsigned long code, void *cmd)
{
	if (code == SYS_POWER_OFF && !gpio_get_value(gpio_ta_nconnected))
		pm_power_off = clt3g_power_off_charger;

	return 0;
}

static struct notifier_block clt3g_reboot_notifier = {
	.notifier_call = clt3g_reboot_call,
};

static void __init clt3g_reboot_board_init(void)
{
	gpio_ta_nconnected = get_gpio_by_name("TA_nCONNECTED");

	if (likely(gpio_ta_nconnected >= 0))
			register_reboot_notifier(&clt3g_reboot_notifier);
}

static int __init clt3g_sfi_hack_devs(struct sfi_table_header *table)
{
	struct sfi_table_simple *sb;
	struct sfi_device_table_entry *pentry;
	int num, i;

	sb = (struct sfi_table_simple *)table;
	num = SFI_GET_NUM_ENTRIES(sb, struct sfi_device_table_entry);
	pentry = (struct sfi_device_table_entry *)sb->pentry;

	for (i = 0; i < num; i++, pentry++) {
		if (!strcmp(pentry->name, "smb347-charger"))
			strcpy(pentry->name, "sec-charger");
		if (!strcmp(pentry->name, "max17042"))
			strcpy(pentry->name, "sec-fuelgauge");
		if (!strcmp(pentry->name, "wm1811"))
			strcpy(pentry->name, "wm8994");
	}

	return 0;
}

static int __init clt3g_sfi_hack_gpio(struct sfi_table_header *table)
{
	struct sfi_table_simple *sb;

	sb = (struct sfi_table_simple *)table;
	/* this will make the number of gpios to 0 */
	sb->header.len = sizeof(struct sfi_table_header);

	return 0;
}

static void __init sec_clt3g_init_early(void)
{
	/* TODO: this function is called in 'postcore_initcall_sync' */
	sec_common_init_early();
	sec_common_init();

	sec_muxtbl_init(sec_board_id, sec_board_rev);

	sec_clt3g_dev_init_early();
	sec_clt3g_input_init_early();

	/* FIXME: this is an ugly hack to modify FW data.
	 * to avoid this, FW should be updated according to HW changes */
	sfi_table_parse(SFI_SIG_DEVS, NULL, NULL, clt3g_sfi_hack_devs);
	sfi_table_parse(SFI_SIG_GPIO, NULL, NULL, clt3g_sfi_hack_gpio);
}

static void __init sec_clt3g_init_rest(void)
{
	/* TODO: this funtion is called in 'arch_initcall_sync' */
	sec_common_init_post();

	sec_debug_init_crash_key(NULL);

	sec_clt3g_hsu_init_rest();
	sec_clt3g_i2c_init_rest();
	sec_clt3g_input_init_rest();
}

static void __init sec_clt3g_board_init(void)
{
	/* TODO: this funtion is called in 'subsys_initcall' */
	clt3g_reboot_board_init();
}

BOARD_START(SAMSUNG_CTP, "ctp_pr1")
	.init_early	= sec_clt3g_init_early,
	.init_rest	= sec_clt3g_init_rest,
	.board_init	= sec_clt3g_board_init,
	.get_device_ptr	= sec_clt3g_get_device_ptr,
BOARD_END(SAMSUNG_CTP)
