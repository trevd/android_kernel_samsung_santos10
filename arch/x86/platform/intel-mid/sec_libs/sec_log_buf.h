/* arch/x86/platform/intel-mid/sec_libs/sec_logbuf.h
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

#ifndef __SEC_LOG_BUF_H__
#define __SEC_LOG_BUF_H__

#include <linux/kmsg_dump.h>
struct sec_log_buf {
	unsigned int *flag;
	unsigned int *count;
	char *data;
	phys_addr_t phys_data;
	bool enable;
	struct kmsg_dumper dump;
	spinlock_t dump_lock;
};

#ifdef CONFIG_SAMSUNG_USE_SEC_LOG_BUF
void sec_log_buf_reserve(void);
#else
#define sec_log_buf_reserve()
#endif

extern phys_addr_t arm_lowmem_limit;

#endif /* __SEC_LOG_BUF_H__ */
