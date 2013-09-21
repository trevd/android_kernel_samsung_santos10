/* arch/x86/platform/intel-mid/board-santos10-muxset-r01.c
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
	/* [--OUT] GP_AON_042 (gp_aon_042) - IMA_nRST */
	INTEL_MUXTBL("kbd_mkout[0]", "clv_gpio_0", kbd_mkout0,
		     "IMA_nRST", 42, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_043 (gp_aon_043) - HDMI_HPD */
	INTEL_MUXTBL("kbd_mkout[1]", "clv_gpio_0", kbd_mkout1,
		     "HDMI_HPD", 43, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_044 (gp_aon_044) - ADC_INT_1.8V */
	INTEL_MUXTBL("kbd_mkout[2]", "clv_gpio_0", kbd_mkout2,
		     "ADC_INT_1.8V", 44, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_045 (gp_aon_045) - CP_PMU_RST */
	INTEL_MUXTBL("kbd_mkout[3]", "clv_gpio_0", kbd_mkout3,
		     "CP_PMU_RST", 45, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_049 (gp_aon_049) - BACKLIGHT_PWM */
	INTEL_MUXTBL("kbd_mkout[7]", "clv_gpio_0", kbd_mkout7,
		     "BACKLIGHT_PWM", 49, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_AON_051 (gp_aon_051) - V_BUS */
	INTEL_MUXTBL("spi_3_ss[1]", "clv_gpio_0", spi_3_ss1,
		     "V_BUS", 51, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_AON_060 (gp_aon_060) - ACC_INT */
	INTEL_MUXTBL("uart_0_rx", "clv_gpio_0", uart_0_rx,
		     "ACC_INT", 60, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_061 (gp_aon_061) - ACC_INT1 */
	INTEL_MUXTBL("uart_0_tx", "clv_gpio_0", uart_0_tx,
		     "ACC_INT1", 61, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_062 (gp_aon_062) - TSP_INT */
	INTEL_MUXTBL("uart_0_cts", "clv_gpio_0", uart_0_cts,
		     "TSP_INT", 62, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [--OUT] GP_AON_063 (gp_aon_063) - ALS_INT_18 */
	INTEL_MUXTBL("uart_0_rts", "clv_gpio_0", uart_0_rts,
		     "ALS_INT_18", 63, LNW_GPIO,
		     OUTPUT_ONLY, OD_DISABLE, UP_20K),
	/* [-----] GP_AON_072 (gp_aon_072) - LVDS_LDO_EN2 */
	INTEL_MUXTBL("ded_gpio[4]", "clv_gpio_0", ded_gpio4,
		     "LVDS_LDO_EN2", 72, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_076 (gp_aon_076) - CODEC_LDO_EN */
	INTEL_MUXTBL("ded_gpio[8]", "clv_gpio_0", ded_gpio8,
		     "CODEC_LDO_EN", 76, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_077 (gp_aon_077) - LVDS_RST */
	INTEL_MUXTBL("ded_gpio[9]", "clv_gpio_0", ded_gpio9,
		     "LVDS_RST", 77, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_078 (gp_aon_078) - USB_PHY_RST_N */
	INTEL_MUXTBL("ded_gpio[10]", "clv_gpio_0", ded_gpio10,
		     "usb_otg_phy_rst", 78, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_079 (gp_aon_079) - IF_PMIC_IRQ */
	INTEL_MUXTBL("ded_gpio[11]", "clv_gpio_0", ded_gpio11,
		     "IF_PMIC_IRQ", 79, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_AON_084 (gp_aon_084) - PHONE_ACTIVE */
	INTEL_MUXTBL("lpc_frame_b", "clv_gpio_0", ulpi1lpc_lpc_frame_b,
		     "PHONE_ACTIVE", 84, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-R-S-] GP_AON_085 (gp_aon_085) - RS */
	INTEL_MUXTBL("lpc_ad[0]", "clv_gpio_0", ulpi1lpc_lpc_ad0,
		     "gp_aon_085", 85, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_086 (gp_aon_086) - BT_HOST_WAKE */
	INTEL_MUXTBL("lpc_ad[1]", "clv_gpio_0", ulpi1lpc_lpc_ad1,
		     "BT_HOST_WAKE", 86, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [IN---] GP_AON_087 (gp_aon_087) - TA_EN.nc */
	INTEL_MUXTBL("lpc_ad[2]", "clv_gpio_0", ulpi1lpc_lpc_ad2,
		     "TA_EN.nc", 87, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_AON_088 (gp_aon_088) - SIM_DETECT */
	INTEL_MUXTBL("lpc_ad[3]", "clv_gpio_0", ulpi1lpc_lpc_ad3,
		     "SIM_DETECT", 88, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_089 (gp_aon_089) - UART_SEL1 */
	INTEL_MUXTBL("lpc_serirq", "clv_gpio_0", ulpi1lpc_lpc_serirq,
		     "UART_SEL1", 89, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_AON_090 (gp_aon_090) - MHL_SCL_1.8V */
	INTEL_MUXTBL("lpc_clkrun", "clv_gpio_0", ulpi1lpc_lpc_clkrun,
		     "MHL_SCL_1.8V", 90, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_AON_091 (gp_aon_091) - MHL_SDA_1.8V */
	INTEL_MUXTBL("lpc_clkout", "clv_gpio_0", ulpi1lpc_lpc_clkout,
		     "MHL_SDA_1.8V", 91, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [IN---] GP_AON_092 (gp_aon_092) - EXT_WAKEUP */
	INTEL_MUXTBL("lpc_reset_b", "clv_gpio_0", ulpi1lpc_lpc_reset_b,
		     "EXT_WAKEUP", 92, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, NONE),
	/* [-----] GP_AON_093 (gp_aon_093) - TA_INT */
	INTEL_MUXTBL("smi_b", "clv_gpio_0", ulpi1lpc_lpc_smi_b,
		     "TA_INT", 93, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_AON_094 (gp_aon_094) - FUEL_ALERT */
	INTEL_MUXTBL("gpe_b", "clv_gpio_0", ulpi1lpc_gpe_b,
		     "FUEL_ALERT", 94, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_CAMERA_SB0 (gp_core_063) - 3M_nSTBY */
	INTEL_MUXTBL("camerasb[0]", "clv_gpio_1", camerasb0,
		     "3M_nSTBY", 159, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_CAMERA_SB1 (gp_core_064) - NC */
	INTEL_MUXTBL("camerasb[1]", "clv_gpio_1", camerasb1,
		     "gp_core_064.nc", 160, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CAMERA_SB2 (gp_core_065) - JIG_ON_18 */
	INTEL_MUXTBL("camerasb[2]", "clv_gpio_1", camerasb2,
		     "JIG_ON_18", 161, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_CAMERA_SB3 (gp_core_066) - NC */
	INTEL_MUXTBL("camerasb[3]", "clv_gpio_1", camerasb3,
		     "gp_core_066.nc", 162, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CAMERA_SB4 (gp_core_076) - VT_CAM_nSTBY */
	INTEL_MUXTBL("camerasb[4]", "clv_gpio_1", camerasb4,
		     "VT_CAM_nSTBY", 172, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CAMERA_SB5 (gp_core_077) - VT_CAM_nRST */
	INTEL_MUXTBL("camerasb[5]", "clv_gpio_1", camerasb5,
		     "VT_CAM_nRST", 173, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_CAMERA_SB6 (gp_core_078) - NC */
	INTEL_MUXTBL("camerasb[6]", "clv_gpio_1", camerasb6,
		     "gp_core_078.nc", 174, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_CAMERA_SB7 (gp_core_079) - NC */
	INTEL_MUXTBL("camerasb[7]", "clv_gpio_1", camerasb7,
		     "gp_core_079.nc", 175, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_CAMERA_SB8 (gp_core_080) - NC */
	INTEL_MUXTBL("camerasb[8]", "clv_gpio_1", camerasb8,
		     "gp_core_080.nc", 176, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CAMERA_SB9 (gp_core_081) - 3M_nRST */
	INTEL_MUXTBL("camerasb[9]", "clv_gpio_1", camerasb9,
		     "3M_nRST", 177, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [IN---] GP_COMS_INT0 (gp_aon_000) - SYS_BURST_DISABLE_N */
	INTEL_MUXTBL("coms_int[0]", "clv_gpio_0", coms_int0,
		     "SYS_BURST_DISABLE_N", 0, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, NONE),
	/* [-----] GP_COMS_INT1 (gp_aon_001) - IMA_INT */
	INTEL_MUXTBL("coms_int[1]", "clv_gpio_0", coms_int1,
		     "IMA_INT", 1, LNW_GPIO,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_COMS_INT2 (gp_aon_002) - WLAN_HOST_WAKE */
	INTEL_MUXTBL("coms_int[2]", "clv_gpio_0", coms_int2,
		     "WLAN_HOST_WAKE", 2, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_COMS_INT3 (gp_aon_003) - BT_WAKE */
	INTEL_MUXTBL("coms_int[3]", "clv_gpio_0", coms_int3,
		     "BT_WAKE", 3, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_007 (gp_core_007) - LVDS_I2C_CLK */
	INTEL_MUXTBL("intd_dsi_te2", "clv_gpio_1", intd_dsi_te2,
		     "LVDS_I2C_CLK", 103, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-N-C-] GP_CORE_012 (gp_core_012) - NC */
	INTEL_MUXTBL("ded_gpio[12]", "clv_gpio_1", ded_gpio12,
		     "gp_core_012.nc", 108, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CORE_015 (gp_core_015) - LVDS_I2C_SDA */
	INTEL_MUXTBL("ded_gpio[15]", "clv_gpio_1", ded_gpio15,
		     "LVDS_I2C_SDA", 111, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_CORE_016 (gp_core_016) - MOT_EN */
	INTEL_MUXTBL("ded_gpio[16]", "clv_gpio_1", ded_gpio16,
		     "MOT_EN", 112, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_017 (gp_core_017) - CP_ON */
	INTEL_MUXTBL("ded_gpio[17]", "clv_gpio_1", ded_gpio17,
		     "CP_ON", 113, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_018 (gp_core_018) - VPS_SOUND_EN */
	INTEL_MUXTBL("ded_gpio[18]", "clv_gpio_1", ded_gpio18,
		     "VPS_SOUND_EN", 114, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_020 (gp_core_020) - MICBIAS_EN */
	INTEL_MUXTBL("ded_gpio[20]", "clv_gpio_1", ded_gpio20,
		     "MICBIAS_EN", 116, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_CORE_030 (gp_core_030) - NC */
	INTEL_MUXTBL("ded_gpio[27]", "clv_gpio_1", ded_gpio27,
		     "gp_core_030.nc", 126, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CORE_031 (gp_core_031) - BACKLIGHT_RESET */
	INTEL_MUXTBL("ded_gpio[28]", "clv_gpio_1", ded_gpio28,
		     "BACKLIGHT_RESET", 127, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CORE_032 (gp_core_032) - ADC_I2C_SCL_1.8V */
	INTEL_MUXTBL("ded_gpio[29]", "clv_gpio_1", ded_gpio29,
		     "ADC_I2C_SCL_1.8V", 128, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_CORE_033 (gp_core_033) - ADC_I2C_SDA_1.8V */
	INTEL_MUXTBL("ded_gpio[30]", "clv_gpio_1", ded_gpio30,
		     "ADC_I2C_SDA_1.8V", 129, LNW_GPIO,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_CORE_037 (gp_core_037) - BT_EN */
	INTEL_MUXTBL("hdmi_cec", "clv_gpio_1", hdmi_cec,
		     "BT_EN", 133, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_CORE_067 (gp_core_067) - NC */
	INTEL_MUXTBL("aclkph", "clv_gpio_1", aclkph,
		     "gp_core_067.nc", 163, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_CORE_068 (gp_core_068) - NC */
	INTEL_MUXTBL("dclkph", "clv_gpio_1", dclkph,
		     "gp_core_068.nc", 164, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_CORE_069 (gp_core_069) - NC */
	INTEL_MUXTBL("lclkph", "clv_gpio_1", lclkph,
		     "gp_core_069.nc", 165, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CORE_070 (gp_core_070) - FW_STRAP1 */
	INTEL_MUXTBL("uclkph", "clv_gpio_1", uclkph,
		     "FW_STRAP1", 166, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_071 (gp_core_071) - KSEL_STRAP1 */
	INTEL_MUXTBL("dsiclkph", "clv_gpio_1", dsiclkph,
		     "KSEL_STRAP1.nc", 167, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_CORE_072 (gp_core_072) - NC */
	INTEL_MUXTBL("lfhclkph", "clv_gpio_1", lfhclkph,
		     "gp_core_072.nc", 168, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_CORE_073 (gp_core_073) - NC */
	INTEL_MUXTBL("ded_gpio[31]", "clv_gpio_1", ded_gpio31,
		     "gp_core_073.nc", 169, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_CORE_074 (gp_core_074) - WLAN_EN */
	INTEL_MUXTBL("ded_gpio[32]", "clv_gpio_1", ded_gpio32,
		     "WLAN_EN", 170, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_075 (gp_core_075) - USB_PHY_CS */
	INTEL_MUXTBL("ded_gpio[33]", "clv_gpio_1", ded_gpio33,
		     "usb_otg_phy_cs", 171, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_CORE_082 (gp_core_082) - MODEM_RESET_BB_N */
	INTEL_MUXTBL("camerasb[10]", "clv_gpio_1", camerasb10,
		     "MODEM_RESET_BB_N", 178, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_EMMC_0_RST# (gp_core_021) - NC */
	INTEL_MUXTBL("ded_gpio[21]", "clv_gpio_1", ded_gpio21,
		     "gp_core_021.nc", 117, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_EMMC_1_RST# (gp_core_022) - NC */
	INTEL_MUXTBL("ded_gpio[22]", "clv_gpio_1", ded_gpio22,
		     "gp_core_022.nc", 118, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_I2C_0_SCL (i2c_0_scl) - DNIE_SCL */
	INTEL_MUXTBL("i2c_0_scl", "clv_gpio_0", i2c_0_scl,
		     "DNIE_SCL", 25, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_0_SDA (i2c_0_sda) - DNIE_SDA */
	INTEL_MUXTBL("i2c_0_sda", "clv_gpio_0", i2c_0_sda,
		     "DNIE_SDA", 24, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_1_SCL (i2c_1_scl) - PHEONIX_I2C_SCL */
	INTEL_MUXTBL("i2c_1_scl", "clv_gpio_0", i2c_1_scl,
		     "PHEONIX_I2C_SCL", 27, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_1_SDA (i2c_1_sda) - PHEONIX_I2C_SDA */
	INTEL_MUXTBL("i2c_1_sda", "clv_gpio_0", i2c_1_sda,
		     "PHEONIX_I2C_SDA", 26, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_2_SCL (i2c_2_scl) - TSP_I2C_SCL_1.8V */
	INTEL_MUXTBL("i2c_2_scl", "clv_gpio_0", i2c_2_scl,
		     "TSP_I2C_SCL_1.8V", 29, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_2_SDA (i2c_2_sda) - TSP_I2C_SDA_1.8V */
	INTEL_MUXTBL("i2c_2_sda", "clv_gpio_0", i2c_2_sda,
		     "TSP_I2C_SDA_1.8V", 28, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_3_SCL_HDMI (i2c_3_scl_hdmi_ddc) - MHL_DSCL_1.2V */
	INTEL_MUXTBL("i2c_3_scl_hdmi_ddc", "clv_gpio_1", i2c_3_scl_hdmi_ddc,
		     "MHL_DSCL_1.2V", 131, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_3_SDA_HDMI (i2c_3_sda_hdmi_ddc) - MHL_DSDA_1.2V */
	INTEL_MUXTBL("i2c_3_sda_hdmi_ddc", "clv_gpio_1", i2c_3_sda_hdmi_ddc,
		     "MHL_DSDA_1.2V", 132, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_4_SCL (i2c_4_scl) - CAM_I2C_SCL */
	INTEL_MUXTBL("i2c_4_scl", "clv_gpio_1", i2c_4_scl,
		     "CAM_I2C_SCL", 135, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_4_SDA (i2c_4_sda) - CAM_I2C_SDA */
	INTEL_MUXTBL("i2c_4_sda", "clv_gpio_1", i2c_4_sda,
		     "CAM_I2C_SDA", 134, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_5_SCL (i2c_5_scl) - SENSOR_I2C_SCL */
	INTEL_MUXTBL("i2c_5_scl", "clv_gpio_1", i2c_5_scl,
		     "SENSOR_I2C_SCL", 137, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2C_5_SDA (i2c_5_sda) - SENSOR_I2C_SDA */
	INTEL_MUXTBL("i2c_5_sda", "clv_gpio_1", i2c_5_sda,
		     "SENSOR_I2C_SDA", 136, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_I2S_0_CLK (i2s_0_clk) - AP_I2S_CLK */
	INTEL_MUXTBL("i2s_0_clk", "clv_gpio_0", i2s_0_clk,
		     "AP_I2S_CLK", 4, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_0_FS (i2s_0_fs) - AP_I2S_SYNC */
	INTEL_MUXTBL("i2s_0_fs", "clv_gpio_0", i2s_0_fs,
		     "AP_I2S_SYNC", 5, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_0_RXD (i2s_0_rxd) - AP_I2S_DI */
	INTEL_MUXTBL("i2s_0_rxd", "clv_gpio_0", i2s_0_rxd,
		     "AP_I2S_DI", 7, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_0_TXD (i2s_0_txd) - AP_I2S_DO */
	INTEL_MUXTBL("i2s_0_txd", "clv_gpio_0", i2s_0_txd,
		     "AP_I2S_DO", 6, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_1_CLK (gp_aon_008) - FUEL_SCL_1.8V */
	INTEL_MUXTBL("i2s_1_clk", "clv_gpio_0", i2s_1_clk,
		     "FUEL_SCL_1.8V", 8, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_FS (gp_aon_009) - FUEL_SDA_1.8V */
	INTEL_MUXTBL("i2s_1_fs", "clv_gpio_0", i2s_1_fs,
		     "FUEL_SDA_1.8V", 9, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_RXD (gp_aon_011) - IF_PMIC_SCL */
	INTEL_MUXTBL("i2s_1_rxd", "clv_gpio_0", i2s_1_rxd,
		     "IF_PMIC_SCL", 11, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] GP_I2S_1_TXD (gp_aon_010) - IF_PMIC_SDA */
	INTEL_MUXTBL("i2s_1_txd", "clv_gpio_0", i2s_1_txd,
		     "IF_PMIC_SDA", 10, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-N-C-] GP_I2S_3_CLK (gp_aon_012) - NC */
	INTEL_MUXTBL("mslim_1_bclk", "clv_gpio_0", mslim_1_bclk,
		     "gp_aon_012.nc", 12, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_I2S_3_FS (gp_aon_013) - MHL_INT */
	INTEL_MUXTBL("mslim_1_bdat", "clv_gpio_0", mslim_1_bdat,
		     "MHL_INT", 13, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_I2S_3_RXD (gp_aon_075) - MHL_RST */
	INTEL_MUXTBL("ded_gpio[7]", "clv_gpio_0", ded_gpio7,
		     "MHL_RST", 75, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_I2S_3_TXD (gp_aon_074) - HDMI_EN */
	INTEL_MUXTBL("ded_gpio[6]", "clv_gpio_0", ded_gpio6,
		     "HDMI_EN", 74, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_MDSI_A_TE (gp_core_006) - NC */
	INTEL_MUXTBL("intd_dsi_te1", "clv_gpio_1", intd_dsi_te1,
		     "gp_core_006.nc", 102, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SD_0_CD# (gp_aon_069) - T_FLASH_DETECT */
	INTEL_MUXTBL("stio_0_cd_b", "clv_gpio_0", stio_0_cd_b,
		     "T_FLASH_DETECT", 69, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SD_0_CLK (stio_0_clk) - T_FLASH_CLK */
	INTEL_MUXTBL("stio_0_clk", "clv_gpio_1", stio_0_clk,
		     "T_FLASH_CLK", 147, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_SD_0_CMD (stio_0_cmd) - T_FLASH_CMD */
	INTEL_MUXTBL("stio_0_cmd", "clv_gpio_1", stio_0_cmd,
		     "T_FLASH_CMD", 146, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_SD_0_D0 (stio_0_dat[0]) - T_FLASH_D(0) */
	INTEL_MUXTBL("stio_0_dat[0]", "clv_gpio_1", stio_0_dat0,
		     "T_FLASH_D(0)", 138, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_SD_0_D1 (stio_0_dat[1]) - T_FLASH_D(1) */
	INTEL_MUXTBL("stio_0_dat[1]", "clv_gpio_1", stio_0_dat1,
		     "T_FLASH_D(1)", 139, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_SD_0_D2 (stio_0_dat[2]) - T_FLASH_D(2) */
	INTEL_MUXTBL("stio_0_dat[2]", "clv_gpio_1", stio_0_dat2,
		     "T_FLASH_D(2)", 140, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_SD_0_D3 (stio_0_dat[3]) - T_FLASH_D(3) */
	INTEL_MUXTBL("stio_0_dat[3]", "clv_gpio_1", stio_0_dat3,
		     "T_FLASH_D(3)", 141, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_SD_0_D4 (gp_core_046) - TSP_VENDOR1 */
	INTEL_MUXTBL("stio_0_dat[4]", "clv_gpio_1", stio_0_dat4,
		     "TSP_VENDOR1", 142, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D5 (gp_core_047) - TSP_VENDOR2 */
	INTEL_MUXTBL("stio_0_dat[5]", "clv_gpio_1", stio_0_dat5,
		     "TSP_VENDOR2", 143, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SD_0_D6 (gp_core_048) - TSP_VENDOR3 */
	INTEL_MUXTBL("stio_0_dat[6]", "clv_gpio_1", stio_0_dat6,
		     "TSP_VENDOR3", 144, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-N-C-] GP_SD_0_D7 (gp_core_049) - NC */
	INTEL_MUXTBL("stio_0_dat[7]", "clv_gpio_1", stio_0_dat7,
		     "gp_core_049.nc", 145, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_SD_0_PWR (gp_core_013) - NC */
	INTEL_MUXTBL("ded_gpio[13]", "clv_gpio_1", ded_gpio13,
		     "gp_core_013.nc", 109, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SD_0_WP (gp_core_052) - EAR_MICBIAS_EN */
	INTEL_MUXTBL("stio_0_wp_b", "clv_gpio_1", stio_0_wp_b,
		     "EAR_MICBIAS_EN", 148, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_SDIO_1_CLK (stio_1_clk) - WLAN_SD_CLK */
	INTEL_MUXTBL("stio_1_clk", "clv_gpio_1", stio_1_clk,
		     "WLAN_SD_CLK", 156, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SDIO_1_CMD (stio_1_cmd) - WLAN_SD_CMD */
	INTEL_MUXTBL("stio_1_cmd", "clv_gpio_1", stio_1_cmd,
		     "WLAN_SD_CMD", 155, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D0 (stio_1_dat[0]) - WLAN_SD_D(0) */
	INTEL_MUXTBL("stio_1_dat[0]", "clv_gpio_1", stio_1_dat0,
		     "WLAN_SD_D(0)", 149, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D1 (stio_1_dat[1]) - WLAN_SD_D(1) */
	INTEL_MUXTBL("stio_1_dat[1]", "clv_gpio_0", stio_1_dat1,
		     "WLAN_SD_D(1)", 70, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D2 (stio_1_dat[2]) - WLAN_SD_D(2) */
	INTEL_MUXTBL("stio_1_dat[2]", "clv_gpio_1", stio_1_dat2,
		     "WLAN_SD_D(2)", 150, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_D3 (stio_1_dat[3]) - WLAN_SD_D(3) */
	INTEL_MUXTBL("stio_1_dat[3]", "clv_gpio_1", stio_1_dat3,
		     "WLAN_SD_D(3)", 151, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SDIO_1_PWR (gp_core_014) - IRDA_EN */
	INTEL_MUXTBL("ded_gpio[14]", "clv_gpio_1", ded_gpio14,
		     "IRDA_EN", 110, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SDIO_2_CLK (gp_core_062) - TSP_LDO_ON */
	INTEL_MUXTBL("stio_2_clk", "clv_gpio_1", stio_2_clk,
		     "TSP_LDO_ON", 158, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SDIO_2_CMD (gp_core_061) - IRDA_CONTROL */
	INTEL_MUXTBL("stio_2_cmd", "clv_gpio_1", stio_2_cmd,
		     "IRDA_CONTROL", 157, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_SDIO_2_D0 (gp_core_056) - NC */
	INTEL_MUXTBL("stio_2_dat[0]", "clv_gpio_1", stio_2_dat0,
		     "gp_core_056.nc", 152, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SDIO_2_D1 (gp_aon_071) - OTG_EN */
	INTEL_MUXTBL("stio_2_dat[1]", "clv_gpio_0", stio_2_dat1,
		     "OTG_EN", 71, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SDIO_2_D2 (gp_core_057) - EAR_GND_SEL */
	INTEL_MUXTBL("stio_2_dat[2]", "clv_gpio_1", stio_2_dat2,
		     "EAR_GND_SEL", 153, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_SDIO_2_D3 (gp_core_058) - NC */
	INTEL_MUXTBL("stio_2_dat[3]", "clv_gpio_1", stio_2_dat3,
		     "gp_core_058.nc", 154, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_SDIO_2_PWR (gp_core_019) - NC */
	INTEL_MUXTBL("ded_gpio[19]", "clv_gpio_1", ded_gpio19,
		     "gp_core_019.nc", 115, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SPI_0_CLK (spi_0_clk) - SPI_0_CLK */
	INTEL_MUXTBL("spi_0_clk", "clv_gpio_0", spi_0_clk,
		     "SPI_0_CLK", 83, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SPI_0_SDI (spi_0_sdi) - SPI_0_SDI */
	INTEL_MUXTBL("spi_0_sdi", "clv_gpio_0", spi_0_sdi,
		     "SPI_0_SDI", 82, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_SPI_0_SDO (spi_0_sdo) - SPI_0_SDO */
	INTEL_MUXTBL("spi_0_sdo", "clv_gpio_0", spi_0_sdo,
		     "SPI_0_SDO", 81, LNW_ALT_1,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SPI_0_SS0 (spi_0_ss0) - SPI_0_SS0 */
	INTEL_MUXTBL("spi_0_ss0", "clv_gpio_0", spi_0_ss,
		     "SPI_0_SS0", 80, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-R-S-] GP_SPI_1_CLK (gp_aon_023) - RS */
	INTEL_MUXTBL("spi_1_clk", "clv_gpio_0", spi_1_clk,
		     "gp_aon_023.rs", 23, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-R-S-] GP_SPI_1_SDI (gp_aon_022) - RS */
	INTEL_MUXTBL("spi_1_sdi", "clv_gpio_0", spi_1_sdi,
		     "gp_aon_022.rs", 22, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-R-S-] GP_SPI_1_SDO (gp_aon_021) - RS */
	INTEL_MUXTBL("spi_1_sdo", "clv_gpio_0", spi_1_sdo,
		     "gp_aon_021.rs", 21, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-R-S-] GP_SPI_1_SS0 (gp_aon_016) - RS */
	INTEL_MUXTBL("spi_1_ss[0]", "clv_gpio_0", spi_1_ss0,
		     "gp_aon_016.rs", 16, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS1 (gp_aon_017) - NC */
	INTEL_MUXTBL("spi_1_ss[1]", "clv_gpio_0", spi_1_ss1,
		     "gp_aon_017.nc", 17, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [IN---] GP_SPI_1_SS2 (gp_aon_018) - eMMC_EN.nc */
	INTEL_MUXTBL("spi_1_ss[2]", "clv_gpio_0", spi_1_ss2,
		     "eMMC_EN.nc", 18, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS3 (gp_aon_019) - NC */
	INTEL_MUXTBL("spi_1_ss[3]", "clv_gpio_0", spi_1_ss3,
		     "gp_aon_019.nc", 19, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] GP_SPI_1_SS4 (gp_aon_020) - NC */
	INTEL_MUXTBL("spi_1_ss[4]", "clv_gpio_0", spi_1_ss4,
		     "gp_aon_020.nc", 20, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SPI_2_CLK (gp_aon_059) - GPS_EN */
	INTEL_MUXTBL("spi_2_clk", "clv_gpio_0", spi_2_clk,
		     "GPS_PWR_EN", 59, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_SPI_2_SDI (gp_aon_058) - EAR_SEND_END */
	INTEL_MUXTBL("spi_2_sdi", "clv_gpio_0", spi_2_sdi,
		     "EAR_SEND_END", 58, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_SPI_2_SDO (gp_aon_057) - NC */
	INTEL_MUXTBL("spi_2_sdo", "clv_gpio_0", spi_2_sdo,
		     "gp_aon_057.nc", 57, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_SPI_2_SS0 (gp_aon_055) - PDA_ACTIVE */
	INTEL_MUXTBL("spi_2_ss[0]", "clv_gpio_0", spi_2_ss0,
		     "PDA_ACTIVE", 55, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_SPI_2_SS1 (gp_aon_056) - NC */
	INTEL_MUXTBL("spi_2_ss[1]", "clv_gpio_0", spi_2_ss1,
		     "gp_aon_056.nc", 56, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [--OUT] GP_SPI_3_CLK (gp_aon_054) - MLCD_ON */
	INTEL_MUXTBL("spi_3_clk", "clv_gpio_0", spi_3_clk,
		     "MLCD_ON", 54, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [--OUT] GP_SPI_3_SDI (gp_aon_053) - IMA_PWR_EN */
	INTEL_MUXTBL("spi_3_sdi", "clv_gpio_0", spi_3_sdi,
		     "IMA_PWR_EN", 53, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [--OUT] GP_SPI_3_SDO (gp_aon_052) - IMA_CMC_EN */
	INTEL_MUXTBL("spi_3_sdo", "clv_gpio_0", spi_3_sdo,
		     "IMA_CMC_EN", 52, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [--OUT] GP_SPI_3_SS0 (gp_aon_050) - IMA_SLEEP */
	INTEL_MUXTBL("spi_3_ss[0]", "clv_gpio_0", spi_3_ss0,
		     "IMA_SLEEP", 50, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_UART_0_CTS (uart_0_cts) - BT_UART_CTS */
	INTEL_MUXTBL("ded_gpio[25]", "clv_gpio_1", ded_gpio25,
		     "BT_UART_CTS", 124, LNW_ALT_1,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_UART_0_RTS (uart_0_rts) - BT_UART_RTS */
	INTEL_MUXTBL("ded_gpio[26]", "clv_gpio_1", ded_gpio26,
		     "BT_UART_RTS", 125, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_UART_0_RX (uart_0_rx) - BT_UART_RXD */
	INTEL_MUXTBL("ded_gpio[23]", "clv_gpio_1", ded_gpio23,
		     "BT_UART_RXD", 122, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_UART_0_TX (uart_0_tx) - BT_UART_TXD */
	INTEL_MUXTBL("ded_gpio[24]", "clv_gpio_1", ded_gpio24,
		     "BT_UART_TXD", 123, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_UART_1_CTS (uart_1_cts) - GPS_UART_CTS */
	INTEL_MUXTBL("uart_2_tx", "clv_gpio_0", uart_2_tx,
		     "GPS_UART_CTS", 68, LNW_ALT_1,
		     BYPASS, OD_DISABLE, DOWN_20K),
	/* [-----] GP_UART_1_RTS (uart_1_rts) - GPS_UART_RTS */
	INTEL_MUXTBL("uart_1_sd", "clv_gpio_0", uart_1_sd,
		     "GPS_UART_RTS", 66, LNW_ALT_2,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] GP_UART_1_RX (uart_1_rx) - GPS_UART_RXD */
	INTEL_MUXTBL("uart_1_rx", "clv_gpio_0", uart_1_rx,
		     "GPS_UART_RXD", 64, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_UART_1_TX (uart_1_tx) - GPS_UART_TXD */
	INTEL_MUXTBL("uart_1_tx", "clv_gpio_0", uart_1_tx,
		     "GPS_UART_TXD", 65, LNW_ALT_1,
		     BYPASS, OD_DISABLE, UP_2K),
	/* [-----] GP_UART_2_RX (uart_2_rx) - AP_RXD */
	INTEL_MUXTBL("uart_2_rx", "clv_gpio_0", uart_2_rx,
		     "AP_RXD", 67, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_UART_2_TX (uart_2_tx) - AP_TXD */
	INTEL_MUXTBL("ded_gpio[5]", "clv_gpio_0", ded_gpio5,
		     "AP_TXD", 73, LNW_ALT_1,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_XDP_BLK_DN (gp_aon_041) - MSENSE_IRQ */
	INTEL_MUXTBL("kbd_mkin[7]", "clv_gpio_0", kbd_mkin7,
		     "MSENSE_IRQ", 41, LNW_GPIO,
		     BYPASS, OD_DISABLE, NONE),
	/* [-----] GP_XDP_BLK_DP (gp_aon_040) - CODEC_26M_EN */
	INTEL_MUXTBL("kbd_mkin[6]", "clv_gpio_0", kbd_mkin6,
		     "CODEC_26M_EN", 40, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_C0_BPM0# (gp_aon_030) - VOL_UP */
	INTEL_MUXTBL("kbd_dkin[0]", "clv_gpio_0", kbd_dkin0,
		     "VOL_UP", 30, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_C0_BPM1# (gp_aon_031) - VOL_DN */
	INTEL_MUXTBL("kbd_dkin[1]", "clv_gpio_0", kbd_dkin1,
		     "VOL_DN", 31, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] GP_XDP_C0_BPM2# (gp_aon_032) - NC */
	INTEL_MUXTBL("kbd_dkin[2]", "clv_gpio_0", kbd_dkin2,
		     "gp_aon_032.nc", 32, LNW_GPIO,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] GP_XDP_C0_BPM3# (gp_aon_033) - LCD_EN */
	INTEL_MUXTBL("kbd_dkin[3]", "clv_gpio_0", kbd_dkin3,
		     "LCD_EN", 33, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_C1_BPM0# (gp_aon_034) - DET_3.5 */
	INTEL_MUXTBL("kbd_mkin[0]", "clv_gpio_0", kbd_mkin0,
		     "DET_3.5", 34, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_C1_BPM1# (gp_aon_035) - MSIC_SPIDEBUG */
	INTEL_MUXTBL("kbd_mkin[1]", "clv_gpio_0", kbd_mkin1,
		     "MSIC_SPIDEBUG", 35, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_XDP_C1_BPM2# (gp_aon_036) - AP_DUMP_INT */
	INTEL_MUXTBL("kbd_mkin[2]", "clv_gpio_0", kbd_mkin2,
		     "AP_DUMP_INT", 36, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_C1_BPM3# (gp_aon_037) - CP_DUMP_INT */
	INTEL_MUXTBL("kbd_mkin[3]", "clv_gpio_0", kbd_mkin3,
		     "CP_DUMP_INT", 37, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_PRDY# (gp_aon_039) - XDP_PRDY_N */
	INTEL_MUXTBL("kbd_mkin[5]", "clv_gpio_0", kbd_mkin5,
		     "XDP_PRDY_N", 39, LNW_GPIO,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] GP_XDP_PREQ# (gp_aon_038) - XDP_PREQ_N */
	INTEL_MUXTBL("kbd_mkin[4]", "clv_gpio_0", kbd_mkin4,
		     "XDP_PREQ_N", 38, LNW_GPIO,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] GP_XDP_PWRMODE0 (gp_aon_046) - XDP_PWRMODE_0 */
	INTEL_MUXTBL("kbd_mkout[4]", "clv_gpio_0", kbd_mkout4,
		     "XDP_PWRMODE_0", 46, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_PWRMODE1 (gp_aon_047) - XDP_PWRMODE_1 */
	INTEL_MUXTBL("kbd_mkout[5]", "clv_gpio_0", kbd_mkout5,
		     "XDP_PWRMODE_1", 47, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] GP_XDP_PWRMODE2 (gp_aon_048) - XDP_PWRMODE_2 */
	INTEL_MUXTBL("kbd_mkout[6]", "clv_gpio_0", kbd_mkout6,
		     "XDP_PWRMODE_2", 48, LNW_GPIO,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] I2S_2_CLK (i2s_2_clk) - NC */
	INTEL_MUXTBL("i2s_2_clk", "unknown", i2s_2_clk,
		     "i2s_2_clk.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] I2S_2_FS (i2s_2_fs) - NC */
	INTEL_MUXTBL("i2s_2_fs", "unknown", i2s_2_fs,
		     "i2s_2_fs.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] I2S_2_RXD (i2s_2_rxd) - NC */
	INTEL_MUXTBL("i2s_2_rxd", "unknown", i2s_2_rxd,
		     "i2s_2_rxd.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] I2S_2_TXD (i2s_2_txd) - NC */
	INTEL_MUXTBL("i2s_2_txd", "unknown", i2s_2_txd,
		     "i2s_2_txd.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] IERR# (ierr) - TP_IERR */
	INTEL_MUXTBL("ierr", "unknown", ierr,
		     "TP_IERR", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, UP_20K),
	/* [-----] JTAG_TCK (jtag_tckc) - ITP_JTAG_TCK */
	INTEL_MUXTBL("jtag_tckc", "unknown", jtag_tckc,
		     "ITP_JTAG_TCK", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] JTAG_TDI (jtag_tdic) - ITP_JTAG_TDI */
	INTEL_MUXTBL("jtag_tdic", "unknown", jtag_tdic,
		     "ITP_JTAG_TDI", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] JTAG_TDO (jtag_tdoc) - ITP_JTAG_TDO */
	INTEL_MUXTBL("jtag_tdoc", "unknown", jtag_tdoc,
		     "ITP_JTAG_TDO", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, UP_2K),
	/* [-----] JTAG_TMS (jtag_tmsc) - ITP_JTAG_TMS */
	INTEL_MUXTBL("jtag_tmsc", "unknown", jtag_tmsc,
		     "ITP_JTAG_TMS", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] JTAG_TRST# (jtag_trst_b) - ITP_JTAG_TRST_N */
	INTEL_MUXTBL("jtag_trst_b", "unknown", jtag_trst_b,
		     "ITP_JTAG_TRST_N", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] MPTI_CLK (mpti_nidnt_clk) - MPTI_CLK */
	INTEL_MUXTBL("mpti_nidnt_clk", "unknown", mpti_nidnt_clk,
		     "MPTI_CLK", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] MPTI_D0 (mpti_nidnt_data[0]) - MPTI_D0 */
	INTEL_MUXTBL("mpti_nidnt_data[0]", "unknown", mpti_nidnt_data0,
		     "MPTI_D0", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] MPTI_D1 (mpti_nidnt_data[1]) - MPTI_D1 */
	INTEL_MUXTBL("mpti_nidnt_data[1]", "unknown", mpti_nidnt_data1,
		     "MPTI_D1", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] MPTI_D2 (mpti_nidnt_data[2]) - MPTI_D2 */
	INTEL_MUXTBL("mpti_nidnt_data[2]", "unknown", mpti_nidnt_data2,
		     "MPTI_D2", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] MPTI_D3 (mpti_nidnt_data[3]) - MPTI_D3 */
	INTEL_MUXTBL("mpti_nidnt_data[3]", "unknown", mpti_nidnt_data3,
		     "MPTI_D3", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] OSC_CLK_CTRL0 (osc_clk_ctrl[0]) - NC */
	INTEL_MUXTBL("osc_clk_ctrl[0]", "unknown", osc_clk_ctrl0,
		     "osc_clk_ctrl[0].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-N-C-] OSC_CLK_CTRL1 (osc_clk_ctrl[1]) - NC */
	INTEL_MUXTBL("osc_clk_ctrl[1]", "unknown", osc_clk_ctrl1,
		     "osc_clk_ctrl[1].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] OSC_CLK0 (osc_clk_out[0]) - CODEC_26MHZ */
	INTEL_MUXTBL("osc_clk_out[0]", "unknown", osc_clk_out0,
		     "CODEC_26MHZ", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] OSC_CLK1 (osc_clk_out[1]) - 3M_MCLK */
	INTEL_MUXTBL("osc_clk_out[1]", "unknown", osc_clk_out1,
		     "3M_MCLK", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] OSC_CLK2 (osc_clk_out[2]) - VT_CAM_MCLK */
	INTEL_MUXTBL("osc_clk_out[2]", "unknown", osc_clk_out2,
		     "VT_CAM_MCLK", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] OSC_CLK3 (osc_clk_out[3]) - NC */
	INTEL_MUXTBL("osc_clk_out[3]", "unknown", osc_clk_out3,
		     "osc_clk_out[3].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, DOWN_20K),
	/* [-----] PMIC_RESET# (msic_reset_b) - MSIC_RESET_N */
	INTEL_MUXTBL("msic_reset_b", "unknown", msic_reset_b,
		     "MSIC_RESET_N", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] PROCHOT# (prochot_b) - PROCHOT_N */
	INTEL_MUXTBL("prochot_b", "unknown", prochot_b,
		     "PROCHOT_N", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] RESETOUT# (resetout_b) - TP_RESETOUT */
	INTEL_MUXTBL("resetout_b", "unknown", resetout_b,
		     "TP_RESETOUT", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] SVID_CLKOUT (svid_clkout) - SVID_CLKOUT */
	INTEL_MUXTBL("svid_clkout", "unknown", svid_clkout,
		     "SVID_CLKOUT", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] SVID_CLKSYNCH (svid_clksynch) - SVID_CLKSYNC */
	INTEL_MUXTBL("svid_clksynch", "unknown", svid_clksynch,
		     "SVID_CLKSYNC", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] SVID_DIN (svid_din) - SVID_DIN */
	INTEL_MUXTBL("svid_din", "unknown", svid_din,
		     "SVID_DIN", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] SVID_DOUT (svid_dout) - SVID_DOUT */
	INTEL_MUXTBL("svid_dout", "unknown", svid_dout,
		     "SVID_DOUT", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] THERMTRIP# (thermtrip_b) - THERMTRIP_N */
	INTEL_MUXTBL("thermtrip_b", "unknown", thermtrip_b,
		     "THERMTRIP_N", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_DISABLE, UP_20K),
	/* [-----] ULPI_0_DIR (usb_ulpi_dir) - USB_ULPI_DIR */
	INTEL_MUXTBL("usb_ulpi_dir", "unknown", usb_ulpi_dir,
		     "USB_ULPI_DIR", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_CLK (usb_ulpi_clk) - USB_ULPI_CLK */
	INTEL_MUXTBL("usb_ulpi_clk", "unknown", usb_ulpi_clk,
		     "USB_ULPI_CLK", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D0 (usb_ulpi_data[0]) - USB_ULPI_D(0) */
	INTEL_MUXTBL("usb_ulpi_data[0]", "unknown", usb_ulpi_data0,
		     "USB_ULPI_D(0)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D1 (usb_ulpi_data[1]) - USB_ULPI_D(1) */
	INTEL_MUXTBL("usb_ulpi_data[1]", "unknown", usb_ulpi_data1,
		     "USB_ULPI_D(1)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D2 (usb_ulpi_data[2]) - USB_ULPI_D(2) */
	INTEL_MUXTBL("usb_ulpi_data[2]", "unknown", usb_ulpi_data2,
		     "USB_ULPI_D(2)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D3 (usb_ulpi_data[3]) - USB_ULPI_D(3) */
	INTEL_MUXTBL("usb_ulpi_data[3]", "unknown", usb_ulpi_data3,
		     "USB_ULPI_D(3)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D4 (usb_ulpi_data[4]) - USB_ULPI_D(4) */
	INTEL_MUXTBL("usb_ulpi_data[4]", "unknown", usb_ulpi_data4,
		     "USB_ULPI_D(4)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D5 (usb_ulpi_data[5]) - USB_ULPI_D(5) */
	INTEL_MUXTBL("usb_ulpi_data[5]", "unknown", usb_ulpi_data5,
		     "USB_ULPI_D(5)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D6 (usb_ulpi_data[6]) - USB_ULPI_D(6) */
	INTEL_MUXTBL("usb_ulpi_data[6]", "unknown", usb_ulpi_data6,
		     "USB_ULPI_D(6)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_D7 (usb_ulpi_data[7]) - USB_ULPI_D(7) */
	INTEL_MUXTBL("usb_ulpi_data[7]", "unknown", usb_ulpi_data7,
		     "USB_ULPI_D(7)", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_NXT (usb_ulpi_nxt) - USB_ULPI_NXT */
	INTEL_MUXTBL("usb_ulpi_nxt", "unknown", usb_ulpi_nxt,
		     "USB_ULPI_NXT", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_REFCLK (usb_ulpi_refclk) - USB_ULPI_REFCLK */
	INTEL_MUXTBL("usb_ulpi_refclk", "unknown", usb_ulpi_refclk,
		     "USB_ULPI_REFCLK", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-----] ULPI_0_STP (usb_ulpi_stp) - USB_ULPI_STP */
	INTEL_MUXTBL("usb_ulpi_stp", "unknown", usb_ulpi_stp,
		     "USB_ULPI_STP", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     BYPASS, OD_ENABLE, NONE),
	/* [-N-C-] ULPI_1_STP (usb_ulpi_1_stp) - NC */
	INTEL_MUXTBL("usb_ulpi_1_stp", "unknown", ulpi1lpc_usb_ulpi_1_stp,
		     "usb_ulpi_1_stp.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_CLK (usb_ulpi_1_clk) - NC */
	INTEL_MUXTBL("usb_ulpi_1_clk", "unknown", ulpi1lpc_usb_ulpi_1_clk,
		     "usb_ulpi_1_clk.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D0 (usb_ulpi_1_data[0]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[0]", "unknown", ulpi1lpc_usb_ulpi_1_data0,
		     "usb_ulpi_1_data[0].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D1 (usb_ulpi_1_data[1]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[1]", "unknown", ulpi1lpc_usb_ulpi_1_data1,
		     "usb_ulpi_1_data[1].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D2 (usb_ulpi_1_data[2]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[2]", "unknown", ulpi1lpc_usb_ulpi_1_data2,
		     "usb_ulpi_1_data[2].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D3 (usb_ulpi_1_data[3]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[3]", "unknown", ulpi1lpc_usb_ulpi_1_data3,
		     "usb_ulpi_1_data[3].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D4 (usb_ulpi_1_data[4]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[4]", "unknown", ulpi1lpc_usb_ulpi_1_data4,
		     "usb_ulpi_1_data[4].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D5 (usb_ulpi_1_data[5]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[5]", "unknown", ulpi1lpc_usb_ulpi_1_data5,
		     "usb_ulpi_1_data[5].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D6 (usb_ulpi_1_data[6]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[6]", "unknown", ulpi1lpc_usb_ulpi_1_data6,
		     "usb_ulpi_1_data[6].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_D7 (usb_ulpi_1_data[7]) - NC */
	INTEL_MUXTBL("usb_ulpi_1_data[7]", "unknown", ulpi1lpc_usb_ulpi_1_data7,
		     "usb_ulpi_1_data[7].nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_NXT (usb_ulpi_1_nxt) - NC */
	INTEL_MUXTBL("usb_ulpi_1_nxt", "unknown", ulpi1lpc_usb_ulpi_1_nxt,
		     "usb_ulpi_1_nxt.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_REFCLK (usb_ulpi_1_refclk) - NC */
	INTEL_MUXTBL("usb_ulpi_1_refclk", "unknown", ulpi1lpc_usb_ulpi_1_refclk,
		     "usb_ulpi_1_refclk.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-N-C-] ULPI_1_DIR (usb_ulpi_1_dir) - NC */
	INTEL_MUXTBL("usb_ulpi_1_dir", "unknown", ulpi1lpc_usb_ulpi_1_dir,
		     "usb_ulpi_1_dir.nc", -EINVAL, INTEL_MUXTBL_NO_CTRL,
		     INPUT_ONLY, OD_ENABLE, DOWN_20K),
	/* [-----] MSIC_GPIO (msic_gpio) - msic_gpio_base */
	INTEL_MUXTBL("msic_gpio", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "msic_gpio_base", 192, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
	/* [-----] GPIO1LV7 (gpio1lv7) - CP_VSD2_1.8V */
	INTEL_MUXTBL("gpio1lv7", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "CP_VSD2_1.8V", 192, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
	/* [-N-C-] GPIO0HV0 (gpio0hv0) - NC */
	INTEL_MUXTBL("gpio0hv0", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "gpio0hv0.nc", 196, LNW_GPIO,
		     BYPASS, MSIC_OD_ENABLE, MSIC_DOWN_200K),
	/* [-N-C-] GPIO0HV1 (gpio0hv1) - NC */
	INTEL_MUXTBL("gpio0hv1", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "gpio0hv1.nc", 195, LNW_GPIO,
		     BYPASS, MSIC_OD_ENABLE, MSIC_DOWN_200K),
	/* [-----] GPIO0HV2 (gpio0hv2) - LCD_SELECT.nc */
	INTEL_MUXTBL("gpio0hv2", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "LCD_SELECT.nc", 194, LNW_GPIO,
		     BYPASS, MSIC_OD_ENABLE, MSIC_DOWN_200K),
	/* [-----] GPIO0HV3 (gpio0hv3) - V_3P30_EXT_VR_EN */
	INTEL_MUXTBL("gpio0hv3", "msic_gpio.reserved", INTEL_MUXTBL_NO_CTRL,
		     "V_3P30_EXT_VR_EN", 193, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
	/* [-----] GPIO1HV0 (gpio1hv0) - HW_REV0 */
	INTEL_MUXTBL("gpio1hv0", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "HW_REV0", 200, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
	/* [-----] GPIO1HV1 (gpio1hv1) - HW_REV1 */
	INTEL_MUXTBL("gpio1hv1", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "HW_REV1", 199, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
	/* [-----] GPIO1HV2 (gpio1hv2) - HW_REV2 */
	INTEL_MUXTBL("gpio1hv2", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "HW_REV2", 198, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
	/* [-----] GPIO1HV3 (gpio1hv3) - HW_REV3 */
	INTEL_MUXTBL("gpio1hv3", "msic_gpio", INTEL_MUXTBL_NO_CTRL,
		     "HW_REV3", 197, LNW_GPIO,
		     BYPASS, MSIC_OD_DISABLE, MSIC_NONE),
};

add_sec_muxtbl_to_list(santos103g, 1, muxtbl);
add_sec_muxtbl_to_list(santos10wifi, 1, muxtbl);
