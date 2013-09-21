/* arch/x86/platform/intel-mid/board-santos10-muxset-r05.c
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
	/* [-----] GP_AON_079 (gp_aon_079) - SD_LDO_EN */
	INTEL_MUXTBL("ded_gpio[11]", "clv_gpio_0", ded_gpio11,
		     "SD_LDO_EN", 79, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SD_0_CMD (stio_0_cmd) - T_FLASH_CMD */
	INTEL_MUXTBL("stio_0_cmd", "clv_gpio_1", stio_0_cmd,
		     "T_FLASH_CMD", 146, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D0 (stio_0_dat[0]) - T_FLASH_D(0) */
	INTEL_MUXTBL("stio_0_dat[0]", "clv_gpio_1", stio_0_dat0,
		     "T_FLASH_D(0)", 138, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D1 (stio_0_dat[1]) - T_FLASH_D(1) */
	INTEL_MUXTBL("stio_0_dat[1]", "clv_gpio_1", stio_0_dat1,
		     "T_FLASH_D(1)", 139, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D2 (stio_0_dat[2]) - T_FLASH_D(2) */
	INTEL_MUXTBL("stio_0_dat[2]", "clv_gpio_1", stio_0_dat2,
		     "T_FLASH_D(2)", 140, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D3 (stio_0_dat[3]) - T_FLASH_D(3) */
	INTEL_MUXTBL("stio_0_dat[3]", "clv_gpio_1", stio_0_dat3,
		     "T_FLASH_D(3)", 141, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-N-C-] GP_XDP_BLK_DP (gp_aon_040) - NC */
	INTEL_MUXTBL("kbd_mkin[6]", "clv_gpio_0", kbd_mkin6,
		     "gp_aon_040.nc", 40, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
};

add_sec_muxtbl_to_list(santos103g, 5, muxtbl);
add_sec_muxtbl_to_list(santos10wifi, 5, muxtbl);
