/* arch/x86/platform/intel-mid/intel_muxtbl.c
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

/* This is an implementation of the Associative Array managing
 * MUX configuration data. */

#include <linux/crc32.h>
#include <linux/errno.h>
#include <linux/rbtree.h>
#include <linux/string.h>

#include <asm/intel_muxtbl.h>

static struct rb_root rb_muxtbl_n = RB_ROOT;	/* net name */
static struct rb_root rb_muxtbl_b = RB_ROOT;	/* ball name */

#define make_fn_intel_muxtbl_rb_lookup_x(__x)				\
static struct intel_muxtbl *__intel_muxtbl_rb_lookup_##__x		\
	(unsigned int crc32)						\
{									\
	struct rb_node *node = rb_muxtbl_##__x.rb_node;			\
	struct intel_muxtbl *muxtbl;					\
									\
	while (node) {							\
		muxtbl = container_of(node, struct intel_muxtbl,	\
				node_##__x);				\
									\
		if (crc32 < muxtbl->crc32_##__x)			\
			node = node->rb_left;				\
		else if (crc32 > muxtbl->crc32_##__x)			\
			node = node->rb_right;				\
		else							\
			return muxtbl;					\
	}								\
									\
	return NULL;							\
}

make_fn_intel_muxtbl_rb_lookup_x(n);
make_fn_intel_muxtbl_rb_lookup_x(b);

#define make_fn_intel_muxtbl_rb_insert_x(__x)				\
static int __init __intel_muxtbl_rb_insert_##__x			\
	(struct intel_muxtbl *muxtbl)					\
{									\
	struct rb_node **new = &(rb_muxtbl_##__x.rb_node);		\
	struct rb_node *parent = NULL;					\
	struct intel_muxtbl *this;					\
									\
	while (*new) {							\
		this = container_of(*new, struct intel_muxtbl,		\
				node_##__x);				\
									\
		parent = *new;						\
		if (muxtbl->crc32_##__x < this->crc32_##__x)		\
			new = &((*new)->rb_left);			\
		else if (muxtbl->crc32_##__x > this->crc32_##__x)	\
			new = &((*new)->rb_right);			\
		else							\
			return -INTEL_MUXTBL_ERR_INSERT_TREE;		\
	}								\
									\
	rb_link_node(&muxtbl->node_##__x, parent, new);			\
	rb_insert_color(&muxtbl->node_##__x, &rb_muxtbl_##__x);		\
									\
	return 0;							\
}

make_fn_intel_muxtbl_rb_insert_x(n);
make_fn_intel_muxtbl_rb_insert_x(b);

/* TODO: currently 'reassign' and 'delete' operations are not implemented
 * because they are not required for managing mux-table and mux-set. */

static struct intel_muxtbl *__intel_muxtbl_rb_lookup(
		unsigned crc32_n, unsigned crc32_b)
{
	struct intel_muxtbl *muxtbl_n = __intel_muxtbl_rb_lookup_n(crc32_n);
	struct intel_muxtbl *muxtbl_b = __intel_muxtbl_rb_lookup_b(crc32_b);

	if (muxtbl_n)
		return muxtbl_n;
	else if (muxtbl_b)
		return muxtbl_b;
	else
		return NULL;
}

static int __init __intel_muxtbl_rb_insert(struct intel_muxtbl *muxtbl)
{
	int err;

	err = __intel_muxtbl_rb_insert_n(muxtbl);
	if (unlikely(err))
		return err;

	err = __intel_muxtbl_rb_insert_b(muxtbl);
	if (unlikely(err))
		return err;

	return 0;
}

int __init intel_muxtbl_init(int flags)
{
	return 0;
}

int __init intel_muxtbl_add_mux(struct intel_muxtbl *muxtbl)
{
	return 0;
}

int __init intel_muxtbl_add_muxset(struct intel_muxset *muxset)
{
	struct intel_muxtbl *muxtbl = muxset->muxtbl;
	struct intel_muxtbl *this;
	unsigned int i = muxset->size;
	unsigned int crc32_n;
	unsigned int crc32_b;
	int err;

	if (!muxtbl)
		return 0;

	do {
		crc32_n = crc32(0, muxtbl->gpio.label,
				strlen(muxtbl->gpio.label));
		crc32_b = crc32(0, muxtbl->mux.ball, strlen(muxtbl->mux.ball));
		muxtbl->crc32_n = crc32_n;
		muxtbl->crc32_b = crc32_b;

		this = __intel_muxtbl_rb_lookup(crc32_n, crc32_b);
		if (this)
			continue;

		err = __intel_muxtbl_rb_insert(muxtbl);
		if (unlikely(err))
			return -INTEL_MUXTBL_ERR_INSERT_TREE;

		err = intel_muxtbl_add_mux(muxtbl);
		if (unlikely(err))
			return -INTEL_MUXTBL_ERR_ADD_MUX;
	} while (--i && ++muxtbl);

	return 0;
}

struct intel_muxtbl *intel_muxtbl_find_by_net_name(const char *name)
{
	unsigned int crc32 = crc32(0, name, strlen(name));

	return __intel_muxtbl_rb_lookup(crc32, 0);
}

struct intel_muxtbl *intel_muxtbl_find_by_ball_name(const char *name)
{
	unsigned int crc32 = crc32(0, name, strlen(name));

	return __intel_muxtbl_rb_lookup(0, crc32);
}

int intel_muxtbl_get_gpio_by_net_name(const char *name)
{
	int gpio;
	struct intel_muxtbl *muxtbl = intel_muxtbl_find_by_net_name(name);

	if (unlikely(!muxtbl)) {
		gpio = -1;	/* for the compatibility of Intel */
		pr_warning("(%s): gpio not found (%s)!\n", __func__, name);
	} else {
		gpio = muxtbl->gpio.gpio;
	}

	return gpio;
}

int get_gpio_by_name(const char *name)
	__attribute__((alias("intel_muxtbl_get_gpio_by_net_name")));

/* if the net-name is endded with '.rs', this pin is assumed as a internally
 * reserved pin by Intel. */
static bool __init __intel_muxtbl_check_reserved_pin(struct intel_muxtbl *muxtbl)
{
	const u32 rs_pin = *(u32 *)".rs\0";
	struct gpio *this = &muxtbl->gpio;
	size_t len;

	len = strlen(this->label);
	if (*(u32 *)&this->label[len - 3] == rs_pin)
		return true;

	return false;
}

int __init intel_muxtbl_run_iterator(
		int (*iterator)(struct intel_muxtbl *, void *),
		void *private)
{
	struct rb_node *node;
	struct intel_muxtbl *muxtbl;
	int err;
	bool is_rs_pin;

	/* rb_muxtbl_n and rb_muxtbl_b have same contents with different
	 * order */
	for (node = rb_first(&rb_muxtbl_n); node; node = rb_next(node)) {
		muxtbl = rb_entry(node, struct intel_muxtbl, node_n);

		is_rs_pin = __intel_muxtbl_check_reserved_pin(muxtbl);
		if (is_rs_pin)
			continue;

		err = iterator(muxtbl, private);
		if (unlikely(err))
			return -1;
	}

	return 0;
}
