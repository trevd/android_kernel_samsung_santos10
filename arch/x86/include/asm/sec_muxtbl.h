/* arch/x86/platform/intel-mid/sec_muxtbl.h
 *
 * Copyright (C) 2011-2013 Samsung Electronics Co, Ltd.
 *
 * Based on arch/arm/mach-omap2/sec_muxtbl.h from Samsung OMAP4
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

#ifndef __SEC_MUXTBL_H__
#define __SEC_MUXTBL_H__

#include <linux/sfi.h>

#define SEC_MUXTBL_TYPE_ANY		0xFFFFFFFF

#define CONFIG_SEC_MUXTBL_DEBUG

/* defined in sec_commone.c */
extern unsigned int sec_board_id;
extern unsigned int sec_board_rev;

#define add_sec_muxtbl_to_list(_board, _rev, _muxtbl)			\
static struct intel_muxset						\
__sec_muxset_data_##_board##_##_rev __initdata = {			\
	.rev = _rev,							\
	.muxtbl = _muxtbl,						\
	.size = ARRAY_SIZE(_muxtbl),					\
};									\
									\
static struct sec_muxtbl_node						\
__sec_muxtbl_data_##_board##_##_rev __initdata = {			\
	.board_name = #_board,						\
	.muxset_ptr = &__sec_muxset_data_##_board##_##_rev,		\
};									\
									\
static int __init __sec_muxtbl_add_node##_board##_##_rev(void)		\
{									\
	u32 board_id;							\
	if (__sec_muxset_data_##_board##_##_rev.rev > sec_board_rev)	\
		return 0;						\
	board_id = crc32(0, #_board, strlen(#_board));			\
	if (sec_board_id != board_id)					\
		return 0;						\
	__sec_muxtbl_data_##_board##_##_rev.board_id = board_id;	\
	sec_muxtbl_add_node(&__sec_muxtbl_data_##_board##_##_rev);	\
	return 0;							\
}									\
postcore_initcall(__sec_muxtbl_add_node##_board##_##_rev);

struct sec_muxtbl_node {
	char *board_name;
	unsigned int board_id;
	struct intel_muxset *muxset_ptr;
	struct list_head list;
};

/**
 * sec_muxtbl_init - initialize Intel MUX Table
 * @type: hardware type
 * @rev: hardware revision
 */
int __init sec_muxtbl_init(unsigned int board_id, unsigned int rev);

void __init sec_muxtbl_add_node(struct sec_muxtbl_node *node);

#endif /* __SEC_MUXTBL_H__ */
