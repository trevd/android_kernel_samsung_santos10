/* arch/x86/platform/intel-mid/board-santos10-sfi.c
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

#include "board-santos10.h"

static struct sfi_device_table_entry __initconst santos10_sfi_table[] = {
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x49, 25000000, "a_gfreq"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x4a, 25000000, "a_socdts"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x4b, 25000000, "a_dramr"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x4c, 25000000, "a_dispon"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x22, 25000000, "a_imrv"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x10, 25000000, "msic_adc"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x12, 25000000, "msic_gpio"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0xFF, 25000000, "clvcs_audio"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0xFF, 25000000, "msic_thermal"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x43, 25000000, "msic_power_btn"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x24, 25000000, "msic_vdd"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x27, 25000000, "msic_hdmi"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0xFF,        0, "gpio-keys"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0xFF,        0, "hdmi-i2c-gpio"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0xFF,        0, "switch-mid"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0xFF,        0, "intel_mid_pwm"},
	{SFI_DEV_TYPE_IPC,  0, 0x00, 0x05, 25000000, "scuLog"},

	{SFI_DEV_TYPE_HSI,  0, 0x00, 0x00, 200000000, "hsi_svnet_modem"},

	{SFI_DEV_TYPE_I2C,  0, 0x08, 0xFF, 400000, "gpsi2c"},
	{SFI_DEV_TYPE_I2C,  0, 0x38, 0xFF, 400000, "sec_cmc624_i2c"},
	{SFI_DEV_TYPE_I2C,  1, 0x1A, 0xFF, 400000, "wm8994"},
	{SFI_DEV_TYPE_I2C,  2, 0x20, 0xFF, 400000, "synaptics_ts"},
	{SFI_DEV_TYPE_I2C,  2, 0x4A, 0xFF, 400000, "mxt1188s_ts"},
	{SFI_DEV_TYPE_I2C,  3, 0x50, 0xFF, 400000, "i2c_msic_hdmi"},
	{SFI_DEV_TYPE_I2C,  4, 0x2D, 0xFF, 400000, "s5k5ccgx"},
	{SFI_DEV_TYPE_I2C,  4, 0x45, 0xFF, 400000, "db8131m"},
	{SFI_DEV_TYPE_I2C,  4, 0x3E, 0xFF, 400000, "max8893"},
	{SFI_DEV_TYPE_I2C,  5, 0x2E, 0xFF, 400000, "geomagnetic"},
	{SFI_DEV_TYPE_I2C,  5, 0x29, 0xFF, 400000, "bh1730fvc"},
	{SFI_DEV_TYPE_I2C,  5, 0x19, 0xFF, 400000, "accelerometer"},
	{SFI_DEV_TYPE_I2C,  6, 0x66, 0xFF, 400000, "max77693"},
	{SFI_DEV_TYPE_I2C,  7, 0x36, 0xFF, 400000, "sec-fuelgauge"},
	{SFI_DEV_TYPE_I2C,  9, 0x41, 0xFF, 400000, "stmpe811"},
	{SFI_DEV_TYPE_I2C, 10, 0x39, 0xFF, 400000, "sii9234_mhl_tx"},
	{SFI_DEV_TYPE_I2C, 10, 0x3D, 0xFF, 400000, "sii9234_tpi"},
	{SFI_DEV_TYPE_I2C, 10, 0x49, 0xFF, 400000, "sii9234_hdmi_rx"},
	{SFI_DEV_TYPE_I2C, 10, 0x64, 0xFF, 400000, "sii9234_cbus"},
	{SFI_DEV_TYPE_I2C, 11, 0x0F, 0xFF, 400000, "mipi_tc358764"},
	{SFI_DEV_TYPE_I2C, 12, 0x48 >> 1, 0xFF, 100000, "asp01"},
	{SFI_DEV_TYPE_I2C, 13, 0x50, 0xFF, 400000, "mc96"},

	/* termination */
	{},
};

struct sfi_device_table_entry __init *sec_santos10_get_sfi_table_ptr(void)
{
	return santos10_sfi_table;
}

int __init sec_santos10_get_sfi_table_num(void)
{
	return ARRAY_SIZE(santos10_sfi_table) - 1;
}
