/* arch/x86/platform/intel-mid/intel_muxtbl.h
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

#ifndef __INTEL_MUXTBL_H__
#define __INTEL_MUXTBL_H__

#include <linux/gpio.h>
#include <linux/rbtree.h>

#include <asm/intel_scu_flis.h>

#define INTEL_MUXTBL_ERR_INSERT_TREE	1
#define INTEL_MUXTBL_ERR_ADD_MUX	2
#define INTEL_MUXTBL_ERR_INIT_GPIO	3

#define INTEL_MUXTBL_IN			(1 << 0)
#define INTEL_MUXTBL_OUT		(1 << 1)

#define INTEL_MUXTBL_NAME_LEN		32

#define INTEL_MUXTBL_NO_CTRL		0xFFFFFFFF

#define INTEL_MUXTBL(_BALL, _CTRL, _FLIS_NO,				\
		     _NET, _GPIO, _ALT,					\
		     _DIR, _OD, _PULL)					\
{									\
	.gpio = {							\
		.gpio = _GPIO,						\
		.label = _NET,						\
	},								\
	.mux = {							\
		.ball = _BALL,						\
		.controller = _CTRL,					\
		.alt_func = _ALT,					\
		.pinname = _FLIS_NO,					\
		.pull = _PULL,						\
		.pin_direction = _DIR,					\
		.open_drain = _OD					\
	},								\
}

/* additional mux (direction settings) */
#define BYPASS		0
#define INPUT_ONLY	(MUX_EN_INPUT_EN | INPUT_EN | MUX_EN_OUTPUT_EN)
#define OUTPUT_ONLY	(MUX_EN_INPUT_EN | MUX_EN_OUTPUT_EN | OUTPUT_EN)
#define TRISTATE	(MUX_EN_INPUT_EN | MUX_EN_OUTPUT_EN)

/* additional gpio configurations for msic */
#define MSIC_MUX_SHIFT		1
#define MSIC_MUX_MASK		(0xF << MSIC_MUX_SHIFT)

#define MSIC_OD_SHIFT		3
#define MSIC_OD_ENABLE		(0 << MSIC_OD_SHIFT)
#define MSIC_OD_DISABLE		(1 << MSIC_OD_SHIFT)

#define MSIC_PULL_SHIFT		2
#define MSIC_PULL_EN		(1 << MSIC_PULL_SHIFT)
#define MSIC_PULL_DIS		(0 << 2)

#define MSIC_NONE		(MSIC_PULL_DIS | 0)
#define MSIC_DOWN_200K		(MSIC_PULL_EN | 0)
#define MSIC_UP_2K		(MISC_PULL_EN | 1)
#define MSIC_UP_20K		(MSIC_PULL_EN | 2)
#define MSIC_DOWN_20K		(MSIC_PULL_EN | 3)

struct intel_mux {
	char *ball;
	char *controller;

	/* GPIO Block */
	unsigned int alt_func;

	/* FLIS Block */
	enum pinname_t pinname;
	u8 pull;
	u8 pin_direction;
	u8 open_drain;
};

struct intel_muxtbl {
	/* rb-tree by net name */
	struct rb_node node_n;
	unsigned int crc32_n;

	/* rb-tree by ball name */
	struct rb_node  node_b;
	unsigned int crc32_b;

	struct gpio gpio;
	struct intel_mux mux;
};

struct intel_muxset {
	unsigned int rev;
	struct intel_muxtbl *muxtbl;
	unsigned int size;
};

extern int intel_muxtbl_init(int flags);

extern int intel_muxtbl_add_muxset(struct intel_muxset *muxset);

extern struct intel_muxtbl *intel_muxtbl_find_by_net_name(const char *name);

extern struct intel_muxtbl *intel_muxtbl_find_by_ball_name(const char *name);

extern int intel_muxtbl_get_gpio_by_net_name(const char *name);

extern int intel_muxtbl_run_iterator(
		int (*iterator)(struct intel_muxtbl *, void *),
		void *private);

#endif /* __INTEL_MUXTBL_H__ */
