/* arch/x86/platform/intel-mid/sec_libs/customize-board.h
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

#ifndef __CUSTOMIZE_BOARD_H__
#define __CUSTOMIZE_BOARD_H__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>

#include <asm/intel-mid.h>

struct board_desc {
	struct list_head list;
	const char *name;
	void	(*init_early)(void);
	void	(*init_rest)(void);
	void	(*board_init)(void);
	struct devs_id *(*get_device_ptr)(void);
};

#define BOARD_START(_type, _name)					\
static struct board_desc board_##_type __initdata = {			\
	.name	= _name,

#define BOARD_END(_type)						\
};									\
									\
static int __init register_##_type##_board(void)			\
{									\
	customize_board_add_desc(&board_##_type);			\
	return 0;							\
}									\
core_initcall(register_##_type##_board);

extern void customize_board_add_desc(struct board_desc *desc);

#endif	/* __CUSTOMIZE_BOARD_H__ */
