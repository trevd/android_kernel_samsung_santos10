/* arch/x86/platform/intel-mid/device_libs/platform_max77693.h
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

#ifndef _PLATFORM_MAX776936_H_
#define _PLATFORM_MAX776936_H_

void *max77693_platform_data(void *info);
void max77693_device_handler(struct sfi_device_table_entry *pentry,
				struct devs_id *dev);

#endif
