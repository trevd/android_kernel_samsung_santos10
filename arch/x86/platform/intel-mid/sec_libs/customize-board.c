/* arch/x86/platform/intel-mid/sec_libs/customize-board.c
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
#include <linux/string.h>

#include "customize-board.h"
#include "sec_common.h"

static struct board_desc *pboard_init __initdata;
static struct board_desc desc_head __initdata;

static int __init customize_board_init_desc(void)
{
	INIT_LIST_HEAD(&desc_head.list);

	return 0;
}
pure_initcall(customize_board_init_desc);

void __init customize_board_add_desc(struct board_desc *desc)
{
	list_add(&desc->list, &desc_head.list);
}

static int __init customize_board_find_desc(void)
{
	struct board_desc *desc;
	struct list_head *entry;

	list_for_each(entry, &desc_head.list) {
		desc = list_entry(entry, struct board_desc, list);
		if (!strcmp(desc->name, sec_board_name)) {
			pboard_init = desc;
			return 0;
		}
	}

	pr_err("Can't find a valid bosrd-descriptor!!!\n");

	/* do NOT boot!!! */
	while (1)
		;

	return 0;
}
core_initcall_sync(customize_board_find_desc);

static int __init customize_board_init_early(void)
{
	pr_info("Board Name : %s (0x%08X)\n",
			sec_board_name, sec_board_id);
	pr_info("Board Rev  : %d\n", sec_board_rev);

	if (pboard_init && pboard_init->init_early)
		pboard_init->init_early();

	return 0;
}
postcore_initcall_sync(customize_board_init_early);

static int __init customize_board_init_rest(void)
{
	if (pboard_init && pboard_init->init_rest)
		pboard_init->init_rest();

	return 0;
}
arch_initcall_sync(customize_board_init_rest);

static int __init customize_board_init_board(void)
{
	if (pboard_init && pboard_init->board_init)
		pboard_init->board_init();

	return 0;
}
subsys_initcall(customize_board_init_board);

struct devs_id __init *get_device_ptr(void)
{
	if (pboard_init && pboard_init->get_device_ptr)
		return pboard_init->get_device_ptr();

	return NULL;
}
