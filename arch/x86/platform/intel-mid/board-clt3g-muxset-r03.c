/* arch/x86/platform/intel-mid/board-clt3g-muxset-r03.c
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
	/* [-N-C-] GP_AON_042 (gp_aon_042) - NC */
	INTEL_MUXTBL("kbd_mkout[0]", "clv_gpio_0", kbd_mkout0,
		     "gp_aon_042.nc", 42, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_AON_051 (gp_aon_051) - V_ACCESSORY_OUT_5.0V */
	INTEL_MUXTBL("spi_3_ss[1]", "clv_gpio_0", spi_3_ss1,
		     "V_ACCESSORY_OUT_5.0V", 51, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_AON_085 (gp_aon_085) - PHONE_ACTIVE */
	INTEL_MUXTBL("lpc_ad[0]", "clv_gpio_0", ulpi1lpc_lpc_ad0,
		     "PHONE_ACTIVE", 85, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_AON_090 (gp_aon_090) - MHL_SCL_1.8V */
	INTEL_MUXTBL("lpc_clkrun", "clv_gpio_0", ulpi1lpc_lpc_clkrun,
		     "MHL_SCL_1.8V", 90, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_AON_091 (gp_aon_091) - MHL_SDA_1.8V */
	INTEL_MUXTBL("lpc_clkout", "clv_gpio_0", ulpi1lpc_lpc_clkout,
		     "MHL_SDA_1.8V", 91, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-N-C-] GP_COMS_INT1 (gp_aon_001) - NC */
	INTEL_MUXTBL("coms_int[1]", "clv_gpio_0", coms_int1,
		     "gp_aon_001.nc", 1, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_CORE_007 (gp_core_007) - LVDS_I2C_CLK */
	INTEL_MUXTBL("intd_dsi_te2", "clv_gpio_1", intd_dsi_te2,
		     "LVDS_I2C_CLK", 103, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_CORE_012 (gp_core_012) - GPS_HOST_REQ */
	INTEL_MUXTBL("ded_gpio[12]", "clv_gpio_1", ded_gpio12,
		     "GPS_HOST_REQ", 108, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_CORE_015 (gp_core_015) - LVDS_I2C_SDA */
	INTEL_MUXTBL("ded_gpio[15]", "clv_gpio_1", ded_gpio15,
		     "LVDS_I2C_SDA", 111, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-N-C-] GP_CORE_018 (gp_core_018) - NC */
	INTEL_MUXTBL("ded_gpio[18]", "clv_gpio_1", ded_gpio18,
		     "gp_core_018.nc", 114, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_CORE_030 (gp_core_030) - VUSB_3.3V_EN */
	INTEL_MUXTBL("ded_gpio[27]", "clv_gpio_1", ded_gpio27,
		     "VUSB_3.3V_EN", 126, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_CORE_073 (gp_core_073) - NC */
	INTEL_MUXTBL("ded_gpio[31]", "clv_gpio_1", ded_gpio31,
		     "gp_core_073.nc", 169, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_I2C_0_SCL (i2c_0_scl) - GPS_SCL_1.8V */
	INTEL_MUXTBL("i2c_0_scl", "clv_gpio_0", i2c_0_scl,
		     "GPS_SCL_1.8V", 25, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_I2C_0_SDA (i2c_0_sda) - GPS_SDA_1.8V */
	INTEL_MUXTBL("i2c_0_sda", "clv_gpio_0", i2c_0_sda,
		     "GPS_SDA_1.8V", 24, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_I2C_1_SCL (i2c_1_scl) - PHEONIX_I2C_SCL */
	INTEL_MUXTBL("i2c_1_scl", "clv_gpio_0", i2c_1_scl,
		     "PHEONIX_I2C_SCL", 27, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2C_1_SDA (i2c_1_sda) - PHEONIX_I2C_SDA */
	INTEL_MUXTBL("i2c_1_sda", "clv_gpio_0", i2c_1_sda,
		     "PHEONIX_I2C_SDA", 26, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2C_2_SCL (i2c_2_scl) - TSP_I2C_SCL_1.8V */
	INTEL_MUXTBL("i2c_2_scl", "clv_gpio_0", i2c_2_scl,
		     "TSP_I2C_SCL_1.8V", 29, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2C_2_SDA (i2c_2_sda) - TSP_I2C_SDA_1.8V */
	INTEL_MUXTBL("i2c_2_sda", "clv_gpio_0", i2c_2_sda,
		     "TSP_I2C_SDA_1.8V", 28, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_CLK (gp_aon_008) - FUEL_SCL_1.8V */
	INTEL_MUXTBL("i2s_1_clk", "clv_gpio_0", i2s_1_clk,
		     "FUEL_SCL_1.8V", 8, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_FS (gp_aon_009) - FUEL_SDA_1.8V */
	INTEL_MUXTBL("i2s_1_fs", "clv_gpio_0", i2s_1_fs,
		     "FUEL_SDA_1.8V", 9, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_RXD (gp_aon_011) - CHG_SCL_1.8V */
	INTEL_MUXTBL("i2s_1_rxd", "clv_gpio_0", i2s_1_rxd,
		     "CHG_SCL_1.8V", 11, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_TXD (gp_aon_010) - CHG_SDA_1.8V */
	INTEL_MUXTBL("i2s_1_txd", "clv_gpio_0", i2s_1_txd,
		     "CHG_SDA_1.8V", 10, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-N-C-] GP_I2S_3_CLK (gp_aon_012) - NC */
	INTEL_MUXTBL("mslim_1_bclk", "clv_gpio_0", mslim_1_bclk,
		     "gp_aon_012.nc", 12, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SD_0_D7 (gp_core_049) - NC */
	INTEL_MUXTBL("stio_0_dat[7]", "clv_gpio_1", stio_0_dat7,
		     "gp_core_049.nc", 145, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SD_0_PWR (gp_core_013) - NC */
	INTEL_MUXTBL("ded_gpio[13]", "clv_gpio_1", ded_gpio13,
		     "gp_core_013.nc", 109, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_CLK (gp_aon_023) - NC */
	INTEL_MUXTBL("spi_1_clk", "clv_gpio_0", spi_1_clk,
		     "gp_aon_023.nc", 23, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SDI (gp_aon_022) - NC */
	INTEL_MUXTBL("spi_1_sdi", "clv_gpio_0", spi_1_sdi,
		     "gp_aon_022.nc", 22, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SDO (gp_aon_021) - NC */
	INTEL_MUXTBL("spi_1_sdo", "clv_gpio_0", spi_1_sdo,
		     "gp_aon_021.nc", 21, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS0 (gp_aon_016) - NC */
	INTEL_MUXTBL("spi_1_ss[0]", "clv_gpio_0", spi_1_ss0,
		     "gp_aon_016.nc", 16, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS1 (gp_aon_017) - NC */
	INTEL_MUXTBL("spi_1_ss[1]", "clv_gpio_0", spi_1_ss1,
		     "gp_aon_017.nc", 17, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS3 (gp_aon_019) - NC */
	INTEL_MUXTBL("spi_1_ss[3]", "clv_gpio_0", spi_1_ss3,
		     "gp_aon_019.nc", 19, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS4 (gp_aon_020) - NC */
	INTEL_MUXTBL("spi_1_ss[4]", "clv_gpio_0", spi_1_ss4,
		     "gp_aon_020.nc", 20, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_2_SDO (gp_aon_057) - NC */
	INTEL_MUXTBL("spi_2_sdo", "clv_gpio_0", spi_2_sdo,
		     "gp_aon_057.nc", 57, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_2_SS1 (gp_aon_056) - NC */
	INTEL_MUXTBL("spi_2_ss[1]", "clv_gpio_0", spi_2_ss1,
		     "gp_aon_056.nc", 56, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_SPI_3_CLK (gp_aon_054) - ADC_I2C_SCL_1.8V */
	INTEL_MUXTBL("spi_3_clk", "clv_gpio_0", spi_3_clk,
		     "ADC_I2C_SCL_1.8V", 54, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_SPI_3_SDI (gp_aon_053) - ADC_I2C_SDA_1.8V */
	INTEL_MUXTBL("spi_3_sdi", "clv_gpio_0", spi_3_sdi,
		     "ADC_I2C_SDA_1.8V", 53, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_UART_0_CTS (uart_0_cts) - BT_UART_CTS */
	INTEL_MUXTBL("ded_gpio[25]", "clv_gpio_1", ded_gpio25,
		     "BT_UART_CTS", 124, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_UART_0_RTS (uart_0_rts) - BT_UART_RTS */
	INTEL_MUXTBL("ded_gpio[26]", "clv_gpio_1", ded_gpio26,
		     "BT_UART_RTS", 125, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_UART_0_RX (uart_0_rx) - BT_UART_RXD */
	INTEL_MUXTBL("ded_gpio[23]", "clv_gpio_1", ded_gpio23,
		     "BT_UART_RXD", 122, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_UART_0_TX (uart_0_tx) - BT_UART_TXD */
	INTEL_MUXTBL("ded_gpio[24]", "clv_gpio_1", ded_gpio24,
		     "BT_UART_TXD", 123, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-N-C-] GP_UART_1_CTS (gp_aon_068) - NC */
	INTEL_MUXTBL("uart_2_tx", "clv_gpio_0", uart_2_tx,
		     "gp_aon_068.nc", 68, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] GP_UART_1_RTS (gp_aon_066) - NC */
	INTEL_MUXTBL("uart_1_sd", "clv_gpio_0", uart_1_sd,
		     "gp_aon_066.nc", 66, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_UART_1_RX (uart_1_rx) - DOCK_RXD */
	INTEL_MUXTBL("uart_1_rx", "clv_gpio_0", uart_1_rx,
		     "DOCK_RXD", 64, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_UART_1_TX (uart_1_tx) - DOCK_TXD */
	INTEL_MUXTBL("uart_1_tx", "clv_gpio_0", uart_1_tx,
		     "DOCK_TXD", 65, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-N-C-] ULPI_1_STP (usb_ulpi_1_stp) - NC */
	INTEL_MUXTBL("usb_ulpi_1_stp", "unknown", ulpi1lpc_usb_ulpi_1_stp,
		     "usb_ulpi_1_stp.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_CLK (usb_ulpi_1_clk) - NC */
	INTEL_MUXTBL("usb_ulpi_1_clk", "unknown", ulpi1lpc_usb_ulpi_1_clk,
		     "usb_ulpi_1_clk.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D0 (usb_ulpi_1_data[0]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[0]", "unknown", ulpi1lpc_usb_ulpi_1_data0,
		     "usb_ulpi_1_data[0].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D1 (usb_ulpi_1_data[1]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[1]", "unknown", ulpi1lpc_usb_ulpi_1_data1,
		     "usb_ulpi_1_data[1].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D2 (usb_ulpi_1_data[2]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[2]", "unknown", ulpi1lpc_usb_ulpi_1_data2,
		     "usb_ulpi_1_data[2].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D3 (usb_ulpi_1_data[3]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[3]", "unknown", ulpi1lpc_usb_ulpi_1_data3,
		     "usb_ulpi_1_data[3].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D4 (usb_ulpi_1_data[4]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[4]", "unknown", ulpi1lpc_usb_ulpi_1_data4,
		     "usb_ulpi_1_data[4].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D5 (usb_ulpi_1_data[5]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[5]", "unknown", ulpi1lpc_usb_ulpi_1_data5,
		     "usb_ulpi_1_data[5].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D6 (usb_ulpi_1_data[6]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[6]", "unknown", ulpi1lpc_usb_ulpi_1_data6,
		     "usb_ulpi_1_data[6].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D7 (usb_ulpi_1_data[7]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[7]", "unknown", ulpi1lpc_usb_ulpi_1_data7,
		     "usb_ulpi_1_data[7].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_NXT (usb_ulpi_1_nxt) - NC */
	INTEL_MUXTBL("usb_ulpi_1_nxt", "unknown", ulpi1lpc_usb_ulpi_1_nxt,
		     "usb_ulpi_1_nxt.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_REFCLK (usb_ulpi_1_refclk) - NC */
	INTEL_MUXTBL("usb_ulpi_1_refclk", "unknown", ulpi1lpc_usb_ulpi_1_refclk,
		     "usb_ulpi_1_refclk.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_DIR (usb_ulpi_1_dir) - NC */
	INTEL_MUXTBL("usb_ulpi_1_dir", "unknown", ulpi1lpc_usb_ulpi_1_dir,
		     "usb_ulpi_1_dir.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, DOWN_20K),
};

add_sec_muxtbl_to_list(ctp_pr1, 3, muxtbl);
