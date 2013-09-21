/* arch/x86/platform/intel-mid/sec_libs/common_initcall.h
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

#ifndef __COMMON_INITCALL_H__
#define __COMMON_INITCALL_H__

struct initcall_node {
	struct list_head list;
	int (*call)(void);
	char *name;
};

#define DEFINE_COMMON_INITCALL(__name, __call)				\
struct initcall_node __name __initdata = {				\
	.call = __call,							\
	.name = #__call,						\
}

/* rootfs_initcall*/
void common_initcall_add_rootfs(struct initcall_node *node);
/* device_initcall*/
void common_initcall_add_device(struct initcall_node *node);
/* device_initcall_sync */
void common_initcall_add_device_sync(struct initcall_node *node);
/* late_initcall */
void common_initcall_add_late(struct initcall_node *node);
/* late_initcall_sync */
void common_initcall_add_late_sync(struct initcall_node *node);

#endif /* __COMMON_INITCALL_H__ */
