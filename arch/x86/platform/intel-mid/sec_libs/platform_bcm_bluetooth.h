/* arch/x86/platform/intel-mid/sec_libs/platform_bcm_bluetooth.h
 *
 * Copyright (C) 2011 Samsung Electronics Co, Ltd.
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

#ifndef __BOARD_BCM_BLUETOOTH_H__
#define __BOARD_BCM_BLUETOOTH_H__
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/wakelock.h>
#include <linux/serial_core.h>
#include <linux/pm_runtime.h>

#include <net/bluetooth/bluetooth.h>
#include <linux/platform_data/sec_ts.h>
#include <asm/intel-mid.h>
#include <asm/intel_muxtbl.h>

void bcm_bt_lpm_exit_lpm_locked(struct device *tty);

#endif
