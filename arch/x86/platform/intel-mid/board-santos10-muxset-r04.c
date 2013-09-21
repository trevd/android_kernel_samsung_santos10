/* arch/x86/platform/intel-mid/board-santos10-muxset-r04.c
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
	/* [IN---] GP_AON_088 (gp_aon_088) - SIM_DETECT.nc */
	INTEL_MUXTBL("lpc_ad[3]", "clv_gpio_0", ulpi1lpc_lpc_ad3,
		     "SIM_DETECT.nc", 88, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_I2C_2_SCL (i2c_2_scl) - TSP_SCL_1.8V */
	INTEL_MUXTBL("i2c_2_scl", "clv_gpio_0", i2c_2_scl,
		     "TSP_SCL_1.8V", 29, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_I2C_2_SDA (i2c_2_sda) - TSP_SDA_1.8V */
	INTEL_MUXTBL("i2c_2_sda", "clv_gpio_0", i2c_2_sda,
		     "TSP_SDA_1.8V", 28, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-N-C-] GP_SPI_1_SS1 (gp_aon_017) - NC */
	INTEL_MUXTBL("spi_1_ss[1]", "clv_gpio_0", spi_1_ss1,
		     "gp_aon_017.nc", 17, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SPI_2_SDO (gp_aon_057) - TOUCHKEY_LED_EN */
	INTEL_MUXTBL("spi_2_sdo", "clv_gpio_0", spi_2_sdo,
		     "TOUCHKEY_LED_EN", 57, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
};

add_sec_muxtbl_to_list(santos103g, 4, muxtbl);
add_sec_muxtbl_to_list(santos10wifi, 4, muxtbl);
