/* arch/x86/platform/intel-mid/board-santos10lte-muxset-r07.c
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
	/* [-N-C-] GP_AON_087 (gp_aon_087) - NC */
	INTEL_MUXTBL("lpc_ad[2]", "clv_gpio_0", ulpi1lpc_lpc_ad2,
		     "gp_aon_087.nc", 87, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_AON_088 (gp_aon_088) - NC */
	INTEL_MUXTBL("lpc_ad[3]", "clv_gpio_0", ulpi1lpc_lpc_ad3,
		     "gp_aon_088.nc", 88, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GPIO0HV2 (gpio0hv2) - LCD_SELECT.nc */
	INTEL_MUXTBL("gpio0hv2", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "LCD_SELECT.nc", 194, LNW_GPIO,
		     BYPASS, MSIC_OD_ENABLE, MSIC_DOWN_200K),
};

add_sec_muxtbl_to_list(santos10lte, 7, muxtbl);
