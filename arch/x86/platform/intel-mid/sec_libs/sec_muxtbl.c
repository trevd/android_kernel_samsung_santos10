/* arch/x86/platform/intel-mid/sec_muxtbl.c
 *
 * Copyright (C) 2011-2013 Samsung Electronics Co, Ltd.
 *
 * Based on arch/arm/mach-omap2/sec_muxtbl.c from Samsung OMAP4
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

#include <linux/gpio.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/sfi.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

static struct list_head sec_muxtbl_head __initdata;

static int __init __sec_muxtbl_sort_cmp(void *priv, struct list_head *a,
					struct list_head *b)
{
	struct sec_muxtbl_node *entry_a =
	    list_entry(a, struct sec_muxtbl_node, list);
	struct sec_muxtbl_node *entry_b =
	    list_entry(b, struct sec_muxtbl_node, list);

	return (entry_a->muxset_ptr->rev > entry_b->muxset_ptr->rev) ? -1 : 1;
}

void __init sec_muxtbl_add_node(struct sec_muxtbl_node *node)
{
	list_add(&(node->list), &sec_muxtbl_head);
}

int __init sec_muxtbl_init(unsigned int board_id, unsigned int rev)
{
	struct sec_muxtbl_node *cur;
	struct list_head *pos;
	int err;

	if (list_empty(&sec_muxtbl_head)) {
		pr_warn("(%s): no available mux_data.\n", __func__);
		while (1)
			;
	}

	list_sort(NULL, &sec_muxtbl_head, __sec_muxtbl_sort_cmp);

	list_for_each(pos, &sec_muxtbl_head) {
		cur = list_entry(pos, struct sec_muxtbl_node, list);
		err = intel_muxtbl_add_muxset(cur->muxset_ptr);
		if (unlikely(err))
			return err;
	}

	return 0;
}

int __init sec_muxtbl_init_list(void)
{
	INIT_LIST_HEAD(&sec_muxtbl_head);

	return 0;
}
pure_initcall(sec_muxtbl_init_list);
