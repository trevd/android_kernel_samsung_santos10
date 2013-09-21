/* drivers/gpio/gpio-langwell-sec.c
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
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <linux/spinlock.h>

#include <asm/intel-mid.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

struct lnw_gpio {
	struct gpio_chip		chip;
	void				*reg_base;
	void				*reg_gplr;
	void				*flis_base;
	spinlock_t			lock;
	unsigned			irq_base;
	int				wakeup;
	struct				pci_dev	*pdev;
	u32				(*get_flis_offset)(int gpio);
};

static int platform;			/* Platform type */
static int nr_gpio;			/* number of gpios */

#define GAFR			7	/* alt function */

static void __iomem *gpio_reg(struct gpio_chip *chip, unsigned offset,
		unsigned int reg_type)
{
	struct lnw_gpio *lnw = container_of(chip, struct lnw_gpio, chip);
	unsigned nreg = chip->ngpio / 32;
	u8 reg = offset / 32;
	void __iomem *ptr;

	ptr = (void __iomem *)(lnw->reg_gplr + reg_type * nreg * 4 + reg * 4);
	return ptr;
}

/* TODO: implement lnw_gpio_get_alt function based on lnw_gpio_set_alt.
 * this function will be used while applying OEM configuration to compare
 * setting in FW and MUXTBL. */
static int lnw_gpio_get_alt(int gpio)
{
	struct lnw_gpio *lnw;
	u32 __iomem *mem;
	int reg;
	int bit;
	u32 offset;
	u32 value;
	unsigned long flags;

	/* use this trick to get memio */
	lnw = irq_get_chip_data(gpio_to_irq(gpio));
	if (unlikely(!lnw)) {
		pr_err("langwell_gpio: can not find pin %d\n", gpio);
		return -EINVAL;
	}
	if (unlikely(gpio < lnw->chip.base ||
	    gpio >= lnw->chip.base + lnw->chip.ngpio)) {
		dev_err(lnw->chip.dev,
			"langwell_gpio: wrong pin %d to config alt\n", gpio);
		return -EINVAL;
	}
	if (unlikely(lnw->irq_base + gpio - lnw->chip.base !=
	    gpio_to_irq(gpio))) {
		dev_err(lnw->chip.dev,
			"langwell_gpio: wrong chip data for pin %d\n", gpio);
		return -EINVAL;
	}
	gpio -= lnw->chip.base;

	if (platform != INTEL_MID_CPU_CHIP_TANGIER) {
		reg = gpio / 16;
		bit = gpio % 16;

		mem = gpio_reg(&lnw->chip, 0, GAFR);
		spin_lock_irqsave(&lnw->lock, flags);
		value = readl(mem + reg);
		value = value >> (bit * 2);
		value &= 0x03;
		spin_unlock_irqrestore(&lnw->lock, flags);
		dev_dbg(lnw->chip.dev, "ALT: writing 0x%x to %p\n",
				value, mem + reg);
	} else {
		offset = lnw->get_flis_offset(gpio);
		if (WARN(offset == -EINVAL, "invalid pin %d\n", gpio))
			return -EINVAL;

		mem = (void __iomem *)(lnw->flis_base + offset);

		spin_lock_irqsave(&lnw->lock, flags);
		value = readl(mem);
		value &= 0x07;
		spin_unlock_irqrestore(&lnw->lock, flags);
	}

	return value;
}

static int inline lnw_gpio_disable_nc_pin(struct gpio *gpio)
{
	const u32 nc_pin = *(u32 *)".nc\0";
	size_t len;

	len = strlen(gpio->label);
	/* ex) gp_core_066.nc */
	if (*(u32 *)&gpio->label[len - 3] != nc_pin)
		return -EINVAL;

	lnw_gpio_set_alt(gpio->gpio, LNW_GPIO);

	return gpio_request_one(gpio->gpio, GPIOF_IN, gpio->label);
}

static int __init lnw_gpio_muxtbl_iterator(struct intel_muxtbl *muxtbl,
		void *private)
{
	int gpio;
	int alt_old;
	int *err = (int *)private;

	gpio = muxtbl->gpio.gpio;
	if (gpio == -EINVAL)
		return 0;	/* keep going iteration */

	/* if out of range */
	if (unlikely(gpio >= nr_gpio))
		return 0;

	if (!lnw_gpio_disable_nc_pin(&muxtbl->gpio))
		return 0;	/* we don't need to any more reconfig */

	alt_old = lnw_gpio_get_alt(gpio);
	if (unlikely(alt_old == -EINVAL)) {
		pr_warning("(%s): can't read current ALT value - (%d) %s!\n",
				__func__, gpio, muxtbl->gpio.label);
		*err -= 1;
		return 0;	/* keep going iteration */
	}

	/* we assume most of pin configuration are same between FW and
	 * MUXTBL */
	if (unlikely(alt_old != muxtbl->mux.alt_func)) {
#ifdef	CONFIG_GPIO_LANGWELL_SEC_DEBUG
		pr_info("[MUXTBL] : (%03d) %-24s: ALT%d -> ALT%d\n",
				gpio, muxtbl->gpio.label,
				alt_old, muxtbl->mux.alt_func);
#endif
		lnw_gpio_set_alt(gpio, muxtbl->mux.alt_func);
	}

	return 0;
}

static int __init lnw_gpio_sec(void)
{
	int err = 0;

	platform = intel_mid_identify_cpu();
	nr_gpio = lnw_gpio_get_ngpio();
	if (unlikely(!nr_gpio)) {
		pr_warning("(%s): gpio-langwell is not used!!!\n", __func__);
		return 0;
	}

	intel_muxtbl_run_iterator(lnw_gpio_muxtbl_iterator, &err);
	if (unlikely(err < 0))
		pr_warning("(%s): error is occurred while setting ALT!\n",
				__func__);

	return 0;
}
/* because of lnw_gpio_init is called in fs_initcall, thie extension should
 * be called in fs_initcall_sync */
fs_initcall_sync(lnw_gpio_sec);
