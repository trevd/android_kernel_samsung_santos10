/* arch/x86/platform/intel-mid/sec_libs/sec_common.c
 *
 * Copyright (C) 2010-2013 Samsung Electronics Co, Ltd.
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

#include <linux/crc32.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/kallsyms.h>
#include <linux/reboot.h>
#include <linux/sort.h>
#include <linux/sfi.h>
#include <linux/string.h>
#include <linux/stat.h>

#include <asm/intel-mid.h>
#include <asm/reboot.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel_scu_ipc.h>
#include <asm/intel_mid_rpmsg.h>
#include <linux/delay.h>

#include "sec_common.h"
#include "sec_debug.h"

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct class *camera_class;
EXPORT_SYMBOL(camera_class);

static struct device *firmware_dev;

struct cpuinfo_x86 *c_info;

#define BOARD_NAME_LEN			16

static char __sec_board_name[BOARD_NAME_LEN];	/* androidboot.hardware */
char *sec_board_name = __sec_board_name;
EXPORT_SYMBOL(sec_board_name);
unsigned int sec_board_id;	/* crc32 of androidboot.hardware */
EXPORT_SYMBOL(sec_board_id);
unsigned char sec_ifwi_version;
EXPORT_SYMBOL(sec_ifwi_version);

static int __init sec_common_set_board_name(char *str)
{
	strncpy(sec_board_name, str, BOARD_NAME_LEN - 1);

	sec_board_id = crc32(0, sec_board_name, strlen(sec_board_name));

	return 0;
}

early_param("androidboot.hardware", sec_common_set_board_name);

unsigned int sec_board_rev;	/* androidboot.revision */
EXPORT_SYMBOL(sec_board_rev);

static int __init sec_common_set_board_rev(char *str)
{
	unsigned int rev;

	if (!kstrtouint(str, 0, &rev))
		sec_board_rev = rev;

	return 0;
}

early_param("androidboot.revision", sec_common_set_board_rev);

static ssize_t sec_ifwi_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", sec_ifwi_version);
}
static DEVICE_ATTR(ifwi, S_IRUGO, sec_ifwi_version_show, NULL);

int __init sec_common_init_ifwi_sysfs(void)
{
	firmware_dev = device_create(sec_class,
					NULL, 0, NULL, "firmware");
	if (IS_ERR(firmware_dev))
		pr_err("Failed to create device(firmware)!\n");

	if (device_create_file(firmware_dev, &dev_attr_ifwi))
		pr_err("Failed to create device file(ifwi)!\n");

	return 0;
}

int __init camera_class_init(void)
{
	camera_class = class_create(THIS_MODULE, "camera");
	if (IS_ERR(camera_class))
		pr_err("Failed to create class(camera)!\n");

	return 0;
}

char sec_androidboot_mode[16];
EXPORT_SYMBOL(sec_androidboot_mode);

static __init int setup_androidboot_mode(char *opt)
{
	strncpy(sec_androidboot_mode, opt, 15);
	return 0;
}

__setup("androidboot.mode=", setup_androidboot_mode);

u32 sec_bootmode;
EXPORT_SYMBOL(sec_bootmode);

static __init int setup_boot_mode(char *opt)
{
	unsigned int bootmode;

	if (!kstrtouint(opt, 0, &bootmode))
		sec_bootmode = bootmode;

	return 0;
}

__setup("bootmode=", setup_boot_mode);

struct sec_reboot_code {
	char *cmd;
	int mode;
};

static int __sec_common_cmp_reboot_cmd(const void *code0, const void *code1)
{
	struct sec_reboot_code *r_code0 = (struct sec_reboot_code *)code0;
	struct sec_reboot_code *r_code1 = (struct sec_reboot_code *)code1;

	return (strlen(r_code0->cmd) > strlen(r_code1->cmd)) ? -1 : 1;
}

static int __sec_common_reboot_call(struct notifier_block *this,
				    unsigned long code, void *cmd)
{
	int mode = REBOOT_MODE_PREFIX | REBOOT_MODE_NONE;
	unsigned int value;
	size_t cmd_len;

	struct sec_reboot_code reboot_tbl[] = {
		{"arm11_fota", REBOOT_MODE_PREFIX | REBOOT_MODE_FOTA},
		{"arm9_fota", REBOOT_MODE_PREFIX | REBOOT_MODE_FOTA},
		{"recovery", REBOOT_MODE_PREFIX | REBOOT_MODE_RECOVERY},
		{"fota", REBOOT_MODE_PREFIX | REBOOT_MODE_FOTA},
		{"fota_bl", REBOOT_MODE_PREFIX | REBOOT_MODE_FOTA_BL},
		{"charging", REBOOT_MODE_PREFIX | REBOOT_MODE_CHARGING},
		{"download", REBOOT_MODE_PREFIX | REBOOT_MODE_DOWNLOAD},
		{"debug", REBOOT_SET_PREFIX | REBOOT_SET_DEBUG},
		{"swsel", REBOOT_SET_PREFIX | REBOOT_SET_SWSEL},
		{"sud", REBOOT_SET_PREFIX | REBOOT_SET_SUD},
	};
	size_t i, n;

	pr_info("[%s] code=%ld cmd=%s\n", __func__, code, (char *)cmd);
	sort(reboot_tbl, ARRAY_SIZE(reboot_tbl),
	     sizeof(struct sec_reboot_code), __sec_common_cmp_reboot_cmd, NULL);

	if ((code == SYS_RESTART) && cmd) {
		n = ARRAY_SIZE(reboot_tbl);
		for (i = 0; i < n; i++) {
			cmd_len = strlen(reboot_tbl[i].cmd);
			if (!strncmp((char *)cmd, reboot_tbl[i].cmd, cmd_len)) {
				mode = reboot_tbl[i].mode;
				break;
			}
		}

		if (mode == (REBOOT_SET_PREFIX | REBOOT_SET_DEBUG) ||
		    mode == (REBOOT_SET_PREFIX | REBOOT_SET_SWSEL) ||
		    mode == (REBOOT_SET_PREFIX | REBOOT_SET_SUD)) {
			if (!kstrtouint((char *)cmd + cmd_len, 0, &value))
				mode |= value;
			else
				mode = REBOOT_MODE_NONE;
		}
		pr_info("(%s): mode = 0x%x, cmd = %s\n",
			__func__, mode, (char *)cmd);

	}
	if (this)
		intel_scu_ipc_write_oemnib((u8 *) &mode, 0x4, SEC_REBOOT_CMD_ADDR);


	if (mode == (REBOOT_MODE_PREFIX | REBOOT_MODE_CHARGING)) {
		intel_scu_ipc_emergency_write_oemnib((u8 *)&mode,
				0x4, SEC_REBOOT_CMD_ADDR);
		pr_info("(%s) will reboot!!!!\n", __func__);
		rpmsg_send_generic_simple_command(IPCMSG_COLD_RESET, 0);

		while(true) {
			pr_info("(%s) waiting for power-down....\n", __func__);
			mdelay(1000);
		}
	}

	return NOTIFY_DONE;
}				/* end fn __sec_common_reboot_call */

int sec_common_reboot_call(unsigned long code, void *cmd)
{
	return __sec_common_reboot_call(NULL, code, cmd);
}

static struct notifier_block __sec_common_reboot_notifier = {
	.notifier_call = __sec_common_reboot_call,
};

/*
 * Store a handy board information string which we can use elsewhere like
 * like in panic situation
 */
static char sec_panic_string[256];
static void __init sec_common_set_panic_string(void)
{
	snprintf(sec_panic_string, ARRAY_SIZE(sec_panic_string),
		 "%s - Samsung Board (0x%08X): %02X, cpu %s stepping %d",
		 sec_board_name, sec_board_id, sec_board_rev,
		 c_info->x86_model_id[0] ? c_info->x86_model_id : "unknown",
		 c_info->x86_mask);

	mach_panic_string = sec_panic_string;
}

int sec_common_init_early(void)
{
	c_info = &boot_cpu_data;
	sec_common_set_panic_string();

	return 0;
}				/* end fn sec_common_init_early */

int __init sec_common_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class))
		pr_err("Class(sec) Creating Fail!!!\n");

	return 0;
}				/* end fn sec_common_init */

int __init sec_common_init_post(void)
{
	register_reboot_notifier(&__sec_common_reboot_notifier);

	return 0;
}				/* end fn sec_common_init_post */
