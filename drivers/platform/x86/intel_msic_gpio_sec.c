/* drivers/platform/x86/intel_msic_gpio_sec.c
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
#include <linux/kallsyms.h>
#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <linux/string.h>

#include <asm/intel-mid.h>
#include <asm/intel_muxtbl.h>
#include <asm/intel_scu_pmic.h>
#include <asm/sec_muxtbl.h>

#define GPIO0LV0CTLO		0x048
#define GPIO0HV0CTLO		0x06D

struct msic_gpio {
	struct gpio_chip chip;
	int ngpio_lv; /* number of low voltage gpio */
};

static struct msic_gpio *p_msic_gpio;

static int msic_gpio_base;

static int inline __intel_msic_gpio_set_nc_pins(struct intel_muxtbl *muxtbl)
{
	struct gpio *this = &muxtbl->gpio;
	const u32 nc_pin = *(u32 *)".nc\0";
	size_t len;

	len = strlen(this->label);
	/* ex) gp_core_066.nc */
	if (*(u32 *)&this->label[len - 3] != nc_pin)
		return 0;

	return -(!!gpio_request_one(this->gpio, GPIOF_IN, this->label));
}

static int inline __intel_msic_gpio_set_mux(struct intel_muxtbl *muxtbl)
{
	struct gpio *this = &muxtbl->gpio;
	int offset;
	u16 ctlo;
	u8 value;
	u8 mux;
	int err;

	offset = this->gpio - msic_gpio_base;
	ctlo = (u16)(offset < p_msic_gpio->ngpio_lv ? GPIO0LV0CTLO + offset
			: GPIO0HV0CTLO + (offset - p_msic_gpio->ngpio_lv));

	err = intel_scu_ipc_ioread8(ctlo, &value);
	if (err) {
		pr_warning("(%s): can't get current reg value! (%s - %d)\n",
				__func__, this->label, this->gpio);
		return -1;	/* -EIO */
	}

	mux = muxtbl->mux.open_drain | muxtbl->mux.pull;

	value &= ~MSIC_MUX_MASK;
	value |= mux << MSIC_MUX_SHIFT;

	err = intel_scu_ipc_iowrite8(ctlo, value);
	if (err) {
		pr_warning("(%s): can't set new reg value! (%s - %d)\n",
				__func__, this->label, this->gpio);
		return -1;	/* -EIO */
	}

	return 0;
}

static int __init intel_msic_gpio_muxtbl_iterator(struct intel_muxtbl *muxtbl,
		void *private)
{
	int *err = (int *)private;

	if (likely(muxtbl->gpio.gpio < msic_gpio_base))
		return 0;	/* this means lnw gpio */

	if (strcmp(muxtbl->mux.controller, "msic_gpio"))
		return 0;	/* not a part of msic_gpio */

	*err += __intel_msic_gpio_set_nc_pins(muxtbl);
	*err += __intel_msic_gpio_set_mux(muxtbl);

	return 0;
}

static int __init msic_gpio_sec(void)
{
	int err = 0;

	p_msic_gpio = (struct msic_gpio *)kallsyms_lookup_name("msic_gpio");
	if (!p_msic_gpio) {
		pr_warning("(%s): can't find msic_gpio data!\n", __func__);
		return -ENODEV;
	}
	if (!p_msic_gpio->chip.ngpio) {
		pr_warning("(%s): msic_gpio is not used in this board!\n",
				__func__);
		return -ENODEV;
	}

	msic_gpio_base = get_gpio_by_name("msic_gpio_base");
	if (unlikely(msic_gpio_base < 0)) {
		pr_warning("(%s): gpio_base of MSIC_GPIO is not correct!\n",
				__func__);
		return 0;
	}

	intel_muxtbl_run_iterator(intel_msic_gpio_muxtbl_iterator, &err);
	if (unlikely(err < 0))
		pr_warning("(%s): error is occurred while setting MSIC_GPIO!\n",
				__func__);

	return 0;
}
late_initcall_sync(msic_gpio_sec);
