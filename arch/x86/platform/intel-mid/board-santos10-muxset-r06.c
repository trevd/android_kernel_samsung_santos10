/* arch/x86/platform/intel-mid/board-santos10-muxset-r06.c
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

#include <linux/crc32.h>
#include <linux/lnw_gpio.h>
#include <linux/string.h>

#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

static struct intel_muxtbl muxtbl[] = {
	/* [-N-C-] GP_CORE_072 (gp_core_072) - NC */
	INTEL_MUXTBL("lfhclkph", "clv_gpio_1", lfhclkph,
		     "gp_core_072.nc", 168, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [IN---] GP_SPI_1_SS4 (gp_aon_020) - RF_TOUCH */
	INTEL_MUXTBL("spi_1_ss[4]", "clv_gpio_0", spi_1_ss4,
		     "RF_TOUCH", 20, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, NONE),
};

add_sec_muxtbl_to_list(santos103g, 6, muxtbl);
add_sec_muxtbl_to_list(santos10wifi, 6, muxtbl);
