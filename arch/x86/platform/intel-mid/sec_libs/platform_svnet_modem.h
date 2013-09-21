/* arch/x86/platform/intel-mid/sec_libs/platform_svnet_modem.h
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

#ifndef _PLATFORM_SVNET_MODEM_H_
#define _PLATFORM_SVNET_MODEM_H_

#define SVNET_HSI_CLIENT_CNT	2

extern void *svnet_modem_platform_data(void *data) __attribute__((weak));
#endif
