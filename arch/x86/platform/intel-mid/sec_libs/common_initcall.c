/* arch/x86/platform/intel-mid/sec_libs/common_initcall.c
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
#include <linux/list.h>

#include "common_initcall.h"

#define make_common_initcall_set(__stage)				\
static struct list_head head_##__stage __initdata;			\
									\
void __init common_initcall_add_##__stage(struct initcall_node *node)	\
{									\
	list_add(&(node->list), &head_##__stage);			\
}									\
									\
static int __init common_initcall_do_##__stage(void)			\
{									\
	struct initcall_node *node;					\
	struct list_head *entry;					\
	int err;							\
									\
	list_for_each(entry, &head_##__stage) {				\
		node = list_entry(entry, struct initcall_node, list);	\
		err = node->call();					\
		if (unlikely(err))					\
			pr_err("(%s): error while calling %s (%d)!\n",	\
					__func__, node->name, err);	\
	}								\
									\
	return 0;							\
}

/* rootfs_initcall */
make_common_initcall_set(rootfs);
device_initcall(common_initcall_do_rootfs);

/* device_initcall */
make_common_initcall_set(device);
device_initcall(common_initcall_do_device);

/* device_initcall_sync */
make_common_initcall_set(device_sync);
device_initcall_sync(common_initcall_do_device_sync);

/* late_initcall */
make_common_initcall_set(late);
late_initcall(common_initcall_do_late);

/* late_initcall_sync */
make_common_initcall_set(late_sync);
late_initcall_sync(common_initcall_do_late_sync);

static int __init common_initcall_init(void)
{
	INIT_LIST_HEAD(&head_rootfs);
	INIT_LIST_HEAD(&head_device);
	INIT_LIST_HEAD(&head_device_sync);
	INIT_LIST_HEAD(&head_late);
	INIT_LIST_HEAD(&head_late_sync);

	return 0;
}
arch_initcall(common_initcall_init);
