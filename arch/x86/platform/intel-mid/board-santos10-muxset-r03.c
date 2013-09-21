/* arch/x86/platform/intel-mid/board-santos10-muxset-r03.c
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
	/* [IN---] GP_AON_049 (gp_aon_049) - BACKLIGHT_PWM.nc */
	INTEL_MUXTBL("kbd_mkout[7]", "clv_gpio_0", kbd_mkout7,
		     "BACKLIGHT_PWM.nc", 49, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [--OUT] GP_AON_063 (gp_aon_063) - ALS_NRST */
	INTEL_MUXTBL("uart_0_rts", "clv_gpio_0", uart_0_rts,
		     "ALS_NRST", 63, LNW_GPIO,
		     OUTPUT_ONLY, OD_DISABLE, UP_20K),
	/* [-N-C-] GP_AON_079 (gp_aon_079) - NC */
	INTEL_MUXTBL("ded_gpio[11]", "clv_gpio_0", ded_gpio11,
		     "gp_aon_079.nc", 79, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_089 (gp_aon_089) - IF_PMIC_IRQ */
	INTEL_MUXTBL("lpc_serirq", "clv_gpio_0", ulpi1lpc_lpc_serirq,
		     "IF_PMIC_IRQ", 89, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_090 (gp_aon_090) - MHL_SCL_1.8V */
	INTEL_MUXTBL("lpc_clkrun", "clv_gpio_0", ulpi1lpc_lpc_clkrun,
		     "MHL_SCL_1.8V", 90, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_AON_091 (gp_aon_091) - MHL_SDA_1.8V */
	INTEL_MUXTBL("lpc_clkout", "clv_gpio_0", ulpi1lpc_lpc_clkout,
		     "MHL_SDA_1.8V", 91, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_CORE_012 (gp_core_012) - GRIP_SCL */
	INTEL_MUXTBL("ded_gpio[12]", "clv_gpio_1", ded_gpio12,
		     "GRIP_SCL", 108, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_CORE_020 (gp_core_020) - EAR_MICBIAS_EN */
	INTEL_MUXTBL("ded_gpio[20]", "clv_gpio_1", ded_gpio20,
		     "EAR_MICBIAS_EN", 116, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_030 (gp_core_030) - GRIP_SDA */
	INTEL_MUXTBL("ded_gpio[27]", "clv_gpio_1", ded_gpio27,
		     "GRIP_SDA", 126, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-N-C-] GP_CORE_037 (gp_core_037) - NC */
	INTEL_MUXTBL("hdmi_cec", "clv_gpio_1", hdmi_cec,
		     "gp_core_037.nc", 133, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CORE_067 (gp_core_067) - TSP_VENDOR1 */
	INTEL_MUXTBL("aclkph", "clv_gpio_1", aclkph,
		     "TSP_VENDOR1", 163, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_CORE_068 (gp_core_068) - TSP_VENDOR2 */
	INTEL_MUXTBL("dclkph", "clv_gpio_1", dclkph,
		     "TSP_VENDOR2", 164, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [IN---] GP_CORE_072 (gp_core_072) - RF_TOUCH */
	INTEL_MUXTBL("lfhclkph", "clv_gpio_1", lfhclkph,
		     "RF_TOUCH", 168, LNW_GPIO,
		     INPUT_ONLY, OD_DISABLE, UP_2K),
	/* [IN---] GP_CORE_073 (gp_core_073) - ADJ_DET_AP */
	INTEL_MUXTBL("ded_gpio[31]", "clv_gpio_1", ded_gpio31,
		     "ADJ_DET_AP", 169, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, NONE),
	/* [-----] GP_I2C_0_SCL (i2c_0_scl) - DNIE_SCL */
	INTEL_MUXTBL("i2c_0_scl", "clv_gpio_0", i2c_0_scl,
		     "DNIE_SCL", 25, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_I2C_0_SDA (i2c_0_sda) - DNIE_SDA */
	INTEL_MUXTBL("i2c_0_sda", "clv_gpio_0", i2c_0_sda,
		     "DNIE_SDA", 24, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_I2C_1_SCL (i2c_1_scl) - CODEC_SCL_1.8V */
	INTEL_MUXTBL("i2c_1_scl", "clv_gpio_0", i2c_1_scl,
		     "CODEC_SCL_1.8V", 27, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_1_SDA (i2c_1_sda) - CODEC_SDA_1.8V */
	INTEL_MUXTBL("i2c_1_sda", "clv_gpio_0", i2c_1_sda,
		     "CODEC_SDA_1.8V", 26, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_2_SCL (i2c_2_scl) - TSP_SCL_1.8V */
	INTEL_MUXTBL("i2c_2_scl", "clv_gpio_0", i2c_2_scl,
		     "TSP_SCL_1.8V", 29, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_2_SDA (i2c_2_sda) - TSP_SDA_1.8V */
	INTEL_MUXTBL("i2c_2_sda", "clv_gpio_0", i2c_2_sda,
		     "TSP_SDA_1.8V", 28, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2S_0_CLK (i2s_0_clk) - MM_I2S_CLK */
	INTEL_MUXTBL("i2s_0_clk", "clv_gpio_0", i2s_0_clk,
		     "MM_I2S_CLK", 4, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_0_FS (i2s_0_fs) - MM_I2S_SYNC */
	INTEL_MUXTBL("i2s_0_fs", "clv_gpio_0", i2s_0_fs,
		     "MM_I2S_SYNC", 5, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_0_RXD (i2s_0_rxd) - MM_I2S_DI */
	INTEL_MUXTBL("i2s_0_rxd", "clv_gpio_0", i2s_0_rxd,
		     "MM_I2S_DI", 7, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_0_TXD (i2s_0_txd) - MM_I2S_DO */
	INTEL_MUXTBL("i2s_0_txd", "clv_gpio_0", i2s_0_txd,
		     "MM_I2S_DO", 6, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SD_0_D4 (gp_core_046) - IRDA_SCL */
	INTEL_MUXTBL("stio_0_dat[4]", "clv_gpio_1", stio_0_dat4,
		     "IRDA_SCL", 142, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SD_0_D5 (gp_core_047) - IRDA_SDA */
	INTEL_MUXTBL("stio_0_dat[5]", "clv_gpio_1", stio_0_dat5,
		     "IRDA_SDA", 143, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SD_0_D6 (gp_core_048) - TSP_SW3_EN */
	INTEL_MUXTBL("stio_0_dat[6]", "clv_gpio_1", stio_0_dat6,
		     "TSP_SW3_EN", 144, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D7 (gp_core_049) - IRDA_WAKE */
	INTEL_MUXTBL("stio_0_dat[7]", "clv_gpio_1", stio_0_dat7,
		     "IRDA_WAKE", 145, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SD_0_WP (gp_core_052) - IRDA_IRQ */
	INTEL_MUXTBL("stio_0_wp_b", "clv_gpio_1", stio_0_wp_b,
		     "IRDA_IRQ", 148, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SDIO_1_CLK (stio_1_clk) - WLAN_SDIO_CLK */
	INTEL_MUXTBL("stio_1_clk", "clv_gpio_1", stio_1_clk,
		     "WLAN_SDIO_CLK", 156, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SDIO_1_CMD (stio_1_cmd) - WLAN_SDIO_CMD */
	INTEL_MUXTBL("stio_1_cmd", "clv_gpio_1", stio_1_cmd,
		     "WLAN_SDIO_CMD", 155, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D0 (stio_1_dat[0]) - WLAN_SDIO_D(0) */
	INTEL_MUXTBL("stio_1_dat[0]", "clv_gpio_1", stio_1_dat0,
		     "WLAN_SDIO_D(0)", 149, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D1 (stio_1_dat[1]) - WLAN_SDIO_D(1) */
	INTEL_MUXTBL("stio_1_dat[1]", "clv_gpio_0", stio_1_dat1,
		     "WLAN_SDIO_D(1)", 70, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D2 (stio_1_dat[2]) - WLAN_SDIO_D(2) */
	INTEL_MUXTBL("stio_1_dat[2]", "clv_gpio_1", stio_1_dat2,
		     "WLAN_SDIO_D(2)", 150, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D3 (stio_1_dat[3]) - WLAN_SDIO_D(3) */
	INTEL_MUXTBL("stio_1_dat[3]", "clv_gpio_1", stio_1_dat3,
		     "WLAN_SDIO_D(3)", 151, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-N-C-] GP_SDIO_2_CMD (gp_core_061) - NC */
	INTEL_MUXTBL("stio_2_cmd", "clv_gpio_1", stio_2_cmd,
		     "gp_core_061.nc", 157, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [IN---] GP_SDIO_2_D0 (gp_core_056) - IF_PMIC_IRQ.nc */
	INTEL_MUXTBL("stio_2_dat[0]", "clv_gpio_1", stio_2_dat0,
		     "IF_PMIC_IRQ.nc", 152, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SDIO_2_D2 (gp_core_057) - TSP_RST */
	INTEL_MUXTBL("stio_2_dat[2]", "clv_gpio_1", stio_2_dat2,
		     "TSP_RST", 153, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SPI_1_SS1 (gp_aon_017) - TOUCHKEY_LED_EN */
	INTEL_MUXTBL("spi_1_ss[1]", "clv_gpio_0", spi_1_ss1,
		     "TOUCHKEY_LED_EN", 17, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SPI_1_SS3 (gp_aon_019) - BT_EN */
	INTEL_MUXTBL("spi_1_ss[3]", "clv_gpio_0", spi_1_ss3,
		     "BT_EN", 19, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_BLK_DN (gp_aon_041) - HALL_INT_N */
	INTEL_MUXTBL("kbd_mkin[7]", "clv_gpio_0", kbd_mkin7,
		     "HALL_INT_N", 41, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_C0_BPM2# (gp_aon_032) - HOME_KEY */
	INTEL_MUXTBL("kbd_dkin[2]", "clv_gpio_0", kbd_dkin2,
		     "HOME_KEY", 32, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
};

add_sec_muxtbl_to_list(santos103g, 3, muxtbl);
add_sec_muxtbl_to_list(santos10wifi, 3, muxtbl);
