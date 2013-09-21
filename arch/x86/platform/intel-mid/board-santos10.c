/* arch/x86/platform/intel-mid/board-santos10.c
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
#include <linux/input.h>
#include <linux/reboot.h>
#include <linux/sfi.h>

#include <asm/sec_muxtbl.h>
#include <linux/intel_mid_pm.h>

#include "sec_libs/common_initcall.h"
#include "sec_libs/customize-board.h"
#include "sec_libs/sec_common.h"
#include "sec_libs/sec_debug.h"

#include "board-santos10.h"

unsigned int sec_id_santos103g;
unsigned int sec_id_santos10wifi;
unsigned int sec_id_santos10lte;

static void __init sec_santos10_init_board_id(void)
{
	sec_id_santos103g =
		crc32(0, "santos103g", strlen("santos103g"));
	sec_id_santos10wifi =
		crc32(0, "santos10wifi", strlen("santos10wifi"));
	sec_id_santos10lte =
		crc32(0, "santos10lte", strlen("santos10lte"));
}

static unsigned int santos10_crash_keycode[] = {
	KEY_VOLUMEDOWN, KEY_POWER, KEY_POWER,
};

static struct sec_crash_key santos10_crash_key = {
	.keycode	= santos10_crash_keycode,
	.size		= ARRAY_SIZE(santos10_crash_keycode),
};

static void santos10_power_off_charger(void)
{
	pr_emerg("Rebooting into bootloader for charger.\n");
	sec_common_reboot_call(SYS_RESTART, "charging");
}

static int gpio_ta_int;

static int santos10_reboot_call(struct notifier_block *this,
		unsigned long code, void *cmd)
{
	union power_supply_propval value;

	psy_do_property("battery", get, POWER_SUPPLY_PROP_ONLINE, value);

	/* 1. get the cable type using get_property func
	 * 2. if the cable type is not POWER_SUPPLY_TYPE_BATTERY, will reboot
	 */
	if (code == SYS_POWER_OFF &&
		(gpio_get_value(gpio_ta_int) &&
		(value.intval != POWER_SUPPLY_TYPE_BATTERY)))
		pm_power_off = santos10_power_off_charger;

	return 0;
}

static struct notifier_block santos10_reboot_notifier = {
	.notifier_call	= santos10_reboot_call,
};

static void __init santos10_reboot_board_init(void)
{
	gpio_ta_int = get_gpio_by_name("TA_INT");

	if (likely(gpio_ta_int >= 0))
		register_reboot_notifier(&santos10_reboot_notifier);
}

static int __init santos10_set_ifwi_version(struct sfi_table_header *table)
{
	struct sfi_table_oemb *oemb;

	oemb = (struct sfi_table_oemb *) table;
	if (!oemb) {
		pr_err("Fail to read SFI OEMB Layout\n");
		sec_ifwi_version = 0;
		return -ENODEV;
	}
	sec_ifwi_version = oemb->ifwi_minor_version;

	return 0;
}

static int __init santos10_sfi_hack_table(struct sfi_table_header *table)
{
	struct sfi_table_simple *sb;
	struct sfi_device_table_entry *pentry;
	int num, tbl_len;

	sb = (struct sfi_table_simple *)table;
	num = SFI_GET_NUM_ENTRIES(sb, struct sfi_device_table_entry);
	tbl_len = sec_santos10_get_sfi_table_num();

	if (num < tbl_len) {
		pr_err("Fail to hack sfi table. (SFI_TABLE_SIZE < %d)\n",
				tbl_len);
		dump_stack();
		return -1;
	}

	pentry = sec_santos10_get_sfi_table_ptr();
	sb->header.len = sizeof(struct sfi_table_header)
			+ (tbl_len * sizeof(struct sfi_device_table_entry));
	memcpy(sb->pentry, pentry,
			tbl_len * sizeof(struct sfi_device_table_entry));

	return 0;
}

static int __init santos10_sfi_hack_gpio(struct sfi_table_header *table)
{
	struct sfi_table_simple *sb;

	sb = (struct sfi_table_simple *)table;
	/* this will make the number of gpios to 0 */
	sb->header.len = sizeof(struct sfi_table_header);

	return 0;
}

static void santos10_power_off(void)
{
	pr_info("(%s) power-off !!!!\n", __func__);
	pmu_power_off();
}

/* late_initcall_sync */
static int __init sec_santos10_hw_rev_init(void)
{
	struct gpio hw_rev_gpios[] = {
		[0] = {
			.flags	= GPIOF_IN,
			.label	= "HW_REV0",
		},
		[1] = {
			.flags	= GPIOF_IN,
			.label	= "HW_REV1",
		},
		[2] = {
			.flags	= GPIOF_IN,
			.label	= "HW_REV2",
		},
		[3] = {
			.flags	= GPIOF_IN,
			.label	= "HW_REV3",
		},
	};
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(hw_rev_gpios); i++)
		hw_rev_gpios[i].gpio =
			get_gpio_by_name(hw_rev_gpios[i].label);

	err = gpio_request_array(hw_rev_gpios, ARRAY_SIZE(hw_rev_gpios));
	if (unlikely(err)) {
		pr_warning("(%s): can't initialize HW_REV pins!\n", __func__);
		return -ENODEV;
	}

	return 0;
}

/* late_initcall */
static int __init sec_santos10_fix_gpios(void)
{
	struct gpio fix_gpios[] = {
		/* TODO: add gpios to fix initial setting in here. */
		[0] = {
			.label	= "SYS_BURST_DISABLE_N",
			.flags	= GPIOF_IN,
		}
	};
	struct gpio *this;
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(fix_gpios); i++) {
		this = &fix_gpios[i];
		this->gpio = get_gpio_by_name(this->label);
		if (unlikely(this->gpio < 0)) {
			pr_warning("(%s): %s (%d) not found!!\n",
					__func__, this->label, this->gpio);
			continue;
		}

		err = gpio_request_one(this->gpio, this->flags, this->label);
		if (unlikely(err)) {
			pr_warning("(%s): failed to request %s (%d)!!\n",
					__func__, this->label, this->gpio);
			continue;
		}
		gpio_free(this->gpio);
	}

	return 0;
}

/* rootfs_initcall */
static DEFINE_COMMON_INITCALL(santos10_battery, sec_santos10_battery_init);
static DEFINE_COMMON_INITCALL(santos10_keyled, sec_santos10_keyled_init);
static DEFINE_COMMON_INITCALL(santos10_backlight, sec_santos10_backlight_init);
static DEFINE_COMMON_INITCALL(santos10_hw_i2c, sec_santos10_hw_i2c_init);
static DEFINE_COMMON_INITCALL(santos10_irled, sec_santos10_irled_init);
static DEFINE_COMMON_INITCALL(santos10_button, sec_santos10_button_init);
/* device_initcall */
static DEFINE_COMMON_INITCALL(santos10_usb, sec_santos10_usb_init);
/* device_initcall_sync */
static DEFINE_COMMON_INITCALL(santos10_max77693_gpio_init,
		sec_santos10_max77693_gpio_init);
/* late_initcall */
static DEFINE_COMMON_INITCALL(santos10_vibrator, sec_santos10_vibrator_init);
static DEFINE_COMMON_INITCALL(santos103g_modems, sec_santos103g_modems_init);
static DEFINE_COMMON_INITCALL(santos10lte_modems, sec_santos10lte_modems_init);
static DEFINE_COMMON_INITCALL(santos10_brcm_wlan, sec_santo10_brcm_wlan_init);
static DEFINE_COMMON_INITCALL(santos10_thermistor,
		sec_santos10_thermistor_init);
static DEFINE_COMMON_INITCALL(santos10_fix_gpios, sec_santos10_fix_gpios);
/* late_initcall_sync */
static DEFINE_COMMON_INITCALL(santos10_hw_rev, sec_santos10_hw_rev_init);

static void __init sec_santos10_init_early(void)
{
	/* TODO: this function is called in 'postcore_initcall_sync' */
	sec_common_init_early();
	sec_common_init();

	sec_santos10_init_board_id();

	sec_muxtbl_init(sec_board_id, sec_board_rev);

	sec_santos10_dev_init_early();
	sec_santos10_input_init_early();

	camera_class_init();

	sfi_table_parse(SFI_SIG_OEMB, NULL, NULL,
					santos10_set_ifwi_version);
	sec_common_init_ifwi_sysfs();

	/* FIXME: this is an ugly hack to modify FW data.
	 * to avoid this, FW should be updated according to HW changes */
	sfi_table_parse(SFI_SIG_DEVS, NULL, NULL, santos10_sfi_hack_table);
	sfi_table_parse(SFI_SIG_GPIO, NULL, NULL, santos10_sfi_hack_gpio);

	/*overwrite pm poweroff*/
	pm_power_off = santos10_power_off;
}

static void __init sec_santos10_init_rest(void)
{
	/* TODO: this function is called in 'arch_initcall_sync' */
	sec_common_init_post();

	sec_debug_init_crash_key(&santos10_crash_key);

	sec_santos10_hsu_init_rest();
	sec_santos10_i2c_init_rest();

	if (sec_board_id == sec_id_santos10wifi) {
		sec_santos10wifi_no_modems_init_rest();
		sec_santos10wifi_no_vibrator_init_rest();
	}
}

static void __init sec_santos10_board_init(void)
{
	/* TODO: this function is called in 'subsys_initcall' */
	santos10_reboot_board_init();

	/* rootfs_initcall */
	common_initcall_add_rootfs(&santos10_battery);
	common_initcall_add_rootfs(&santos10_keyled);
	common_initcall_add_rootfs(&santos10_backlight);
	common_initcall_add_rootfs(&santos10_hw_i2c);
	common_initcall_add_rootfs(&santos10_irled);
	common_initcall_add_rootfs(&santos10_button);

	/* device_initcall */
	common_initcall_add_device(&santos10_usb);

	/* device_initcall_sync */
	common_initcall_add_device_sync(&santos10_max77693_gpio_init);

	/* late_initcall */
	common_initcall_add_late(&santos10_brcm_wlan);
	common_initcall_add_late(&santos10_thermistor);
	common_initcall_add_late(&santos10_fix_gpios);
	if (sec_board_id == sec_id_santos103g)
		common_initcall_add_late(&santos103g_modems);
	if (sec_board_id == sec_id_santos10lte)
		common_initcall_add_late(&santos10lte_modems);
	if (sec_board_id != sec_id_santos10wifi)
		common_initcall_add_late(&santos10_vibrator);

	/* late_initcall_sync */
	common_initcall_add_late_sync(&santos10_hw_rev);
}

BOARD_START(SAMSUNG_SANTOS103G, "santos103g")
	.init_early	= sec_santos10_init_early,
	.init_rest	= sec_santos10_init_rest,
	.board_init	= sec_santos10_board_init,
	.get_device_ptr	= sec_santos10_get_device_ptr,
BOARD_END(SAMSUNG_SANTOS103G)

BOARD_START(SAMSUNG_SANTOS10WIFI, "santos10wifi")
	.init_early	= sec_santos10_init_early,
	.init_rest	= sec_santos10_init_rest,
	.board_init	= sec_santos10_board_init,
	.get_device_ptr	= sec_santos10_get_device_ptr,
BOARD_END(SAMSUNG_SANTOS10WIFI)

BOARD_START(SAMSUNG_SANTOS10LTE, "santos10lte")
	.init_early	= sec_santos10_init_early,
	.init_rest	= sec_santos10_init_rest,
	.board_init	= sec_santos10_board_init,
	.get_device_ptr	= sec_santos10_get_device_ptr,
BOARD_END(SAMSUNG_SANTOS10LTE)
