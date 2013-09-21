/* arch/x86/platform/intel-mid/sec_libs/sec_weak_decls.h
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

#ifndef __SEC_WEAK_DECLS_H__
#define __SEC_WEAK_DECLS_H__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sfi.h>

/* __attribute__((weak)) makes these declarations overridable */
extern int get_gpio_by_name(const char *name) __attribute__((weak));

#endif /* __SEC_WEAK_DECLS_H__ */
