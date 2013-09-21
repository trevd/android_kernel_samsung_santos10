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

#ifndef __BOARD_CLT3G_H__
#define __BOARD_CLT3G_H__

#include <linux/interrupt.h>
#include <linux/sfi.h>

#include <asm/intel-mid.h>

/** @category SFI */
void sec_clt3g_dev_init_early(void);
struct devs_id *sec_clt3g_get_device_ptr(void);

/** @category UART */
void sec_clt3g_hsu_init_rest(void);

/** @category I2C-GPIO */
void sec_clt3g_i2c_init_rest(void);

/** @category BUTTON / TSP */
void sec_clt3g_input_init_early(void);
void sec_clt3g_input_init_rest(void);

#endif /* __BOARD_CLT3G_H__ */
