/* arch/x86/platform/intel-mid/board-santos10-dev.c
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
#include <linux/mfd/intel_msic.h>

#include <asm/intel-mid.h>

#include "sec_libs/sec_common.h"
#include "board-santos10.h"

/* platform data by Intel */
#ifdef CONFIG_SND_CTP_MACHINE_WM1811
#include <asm/platform_ctp_audio_wm1811.h>
#else
#include <asm/platform_ctp_audio.h>
#endif
#include "device_libs/platform_ipc.h"
#include "device_libs/platform_msic_adc.h"
#include "device_libs/platform_msic_gpio.h"
#include "device_libs/platform_msic_power_btn.h"
#include "device_libs/platform_msic_thermal.h"
#include "device_libs/platform_msic_vdd.h"
#include "device_libs/platform_pmic_gpio.h"
#include "device_libs/platform_hsi_modem.h"
#include "device_libs/platform_camera.h"

/* platform data by Samsung */
#include "sec_libs/platform_max77693.h"
#include "sec_libs/platform_wm8994.h"
#include "sec_libs/platform_svnet_modem.h"
#include "sec_libs/platform_max8893.h"
#include "sec_libs/platform_s5k5ccgx.h"
#include "sec_libs/platform_db8131m.h"
#ifdef CONFIG_SAMSUNG_MHL
#include "sec_libs/platform_mhl.h"
#endif
#include "sec_libs/platform_k2dh.h"
#include "sec_libs/platform_yas532.h"
#include "sec_libs/platform_bh1730.h"
#include "sec_libs/platform_asp01.h"

#define SEC_DEV_IDS(_rev, _dev_ids)					\
{									\
	.rev = _rev,							\
	.devs = _dev_ids,						\
	.nr_devs = ARRAY_SIZE(_dev_ids),				\
}

#define SFI_DEV_TYPE_NONE  0xFF

struct sec_devs_id {
	unsigned int rev;
	struct devs_id *devs;
	size_t nr_devs;
};

static void __init *no_platform_data(void *info)
{
	return NULL;
}

static struct devs_id *device_ids __initdata;

static struct devs_id __initconst santos103g_devs_r00[] = {
	/* MODEM */
	{"hsi_svnet_modem", SFI_DEV_TYPE_HSI, 0, &svnet_modem_platform_data,
				NULL},

	/* Grip Sensor */
	{"asp01", SFI_DEV_TYPE_I2C, 0, &asp01_platform_data},

	/* terminator */
	{},
};

static struct devs_id __initconst santos10lte_devs_r00[] = {
	/* MODEM */
	{"hsi_svnet_modem", SFI_DEV_TYPE_HSI, 0, &svnet_modem_platform_data,
				NULL},

	/* Grip Sensor */
	{"asp01", SFI_DEV_TYPE_I2C, 0, &asp01_platform_data},

	/* terminator */
	{},
};

static struct devs_id __initconst santos10_devs_r00[] = {
	/* MSIC subdevices */
	{"msic_adc", SFI_DEV_TYPE_IPC, 1, &msic_adc_platform_data,
					&ipc_device_handler},
	{"msic_gpio", SFI_DEV_TYPE_IPC, 1, &msic_gpio_platform_data,
					&ipc_device_handler},
	{"msic_power_btn", SFI_DEV_TYPE_IPC, 1, &msic_power_btn_platform_data,
					&ipc_device_handler},
	{"msic_thermal", SFI_DEV_TYPE_IPC, 1, &msic_thermal_platform_data,
					&ipc_device_handler},
	{"msic_vdd", SFI_DEV_TYPE_IPC, 1, &msic_vdd_platform_data,
					&ipc_device_handler},

	/* IPC devices */
	{"pmic_gpio", SFI_DEV_TYPE_IPC, 1, &pmic_gpio_platform_data,
						&ipc_device_handler},
	{"a_gfreq",   SFI_DEV_TYPE_IPC, 0, &no_platform_data,
						&ipc_device_handler},

	{"scuLog", SFI_DEV_TYPE_IPC, 1, &scu_log_platform_data,
					&ipc_device_handler},
	/* Audio */
	{"clvcs_audio", SFI_DEV_TYPE_IPC, 1, &ctp_audio_wm1811_platform_data,
						&ipc_device_handler},
	{"wm8994", SFI_DEV_TYPE_I2C, 1, &wm8994_platform_data, NULL},

	/* MIPI-LVDS Bridge */
	{"mipi_tc358764", SFI_DEV_TYPE_I2C, 0, &tc35876x_platform_data},

	/* CMC624 */
	{"sec_cmc624_i2c", SFI_DEV_TYPE_I2C, 0, &cmc624_platform_data},

	/* TSP */
	{"synaptics_ts", SFI_DEV_TYPE_I2C, 0,
				&santos10_synaptics_ts_platform_data, NULL},

	/* MAX77693 (Charger, Fuel gauge, MUIC, Sub-PMIC) */
	{"max77693", SFI_DEV_TYPE_I2C, 1, &max77693_platform_data,
						&max77693_device_handler},
	{"sec-fuelgauge", SFI_DEV_TYPE_I2C, 1, &max17050_platform_data},

	/* Samsung ADC I2C */
	{"stmpe811", SFI_DEV_TYPE_I2C, 0, &no_platform_data},

	/* Samsung Camera device */
	{"s5k5ccgx", SFI_DEV_TYPE_I2C, 0, &s5k5ccgx_platform_data,
					&intel_register_i2c_camera_device},
	{"db8131m", SFI_DEV_TYPE_I2C, 0, &db8131m_platform_data,
					&intel_register_i2c_camera_device},
	{"max8893", SFI_DEV_TYPE_I2C, 0, &max8893_platform_data, NULL},

	/* Sensors */
	{"accelerometer", SFI_DEV_TYPE_I2C, 0, &acc_platform_data},
	{"geomagnetic", SFI_DEV_TYPE_I2C, 0, &mag_platform_data},
	{"bh1730fvc", SFI_DEV_TYPE_I2C, 0, &bh1730fvc_platform_data},

#ifdef CONFIG_SAMSUNG_MHL
	{"sii9234_mhl_tx", SFI_DEV_TYPE_I2C, 0, &sii9234_platform_data},
	{"sii9234_tpi", SFI_DEV_TYPE_I2C, 0, &sii9234_platform_data},
	{"sii9234_hdmi_rx", SFI_DEV_TYPE_I2C, 0, &sii9234_platform_data},
	{"sii9234_cbus", SFI_DEV_TYPE_I2C, 0, &sii9234_platform_data},
#endif
	/* termination */
	{},
};

/* 3G/WiFi/LTE : r03 or higher */
static struct devs_id __initconst santos10_devs_r03[] = {
	/* TSP */
	{"mxt1188s_ts", SFI_DEV_TYPE_I2C, 0,
				&santos10_mxt1188s_ts_platform_data, NULL},
	{"synaptics_ts", SFI_DEV_TYPE_NONE, 0, NULL, NULL},

	/* IrLED */
	{"mc96", SFI_DEV_TYPE_I2C, 0, &santos10_irled_platform_data, NULL},

	/* termination */
	{},
};

static struct sec_devs_id __initconst santos103g_device_ids[] = {
	/* TODO: the order MUST be from oldest to latest */
	SEC_DEV_IDS(0, santos10_devs_r00),
	SEC_DEV_IDS(0, santos103g_devs_r00),
	SEC_DEV_IDS(3, santos10_devs_r03),
};

static struct sec_devs_id __initconst santos10wifi_device_ids[] = {
	/* TODO: the order MUST be from oldest to latest */
	SEC_DEV_IDS(0, santos10_devs_r00),
	SEC_DEV_IDS(3, santos10_devs_r03),
};

static struct sec_devs_id __initconst santos10lte_device_ids[] = {
	/* TODO: the order MUST be from oldest to latest */
	SEC_DEV_IDS(0, santos10_devs_r00),
	SEC_DEV_IDS(0, santos10lte_devs_r00),
	SEC_DEV_IDS(3, santos10_devs_r03),
};

static void __init sec_santos10_rm_none_devs(struct devs_id *ids,
		const size_t cnt)
{
	unsigned int i, j;

	for (i = cnt - 1; i < cnt; i--) {
		if (unlikely(ids[i].type != SFI_DEV_TYPE_NONE))
			continue;

		for (j = i - 1; j >= 0; j--) {
			if (!strcmp(ids[j].name, ids[i].name)) {
				pr_info("board rev%d removes dev %s",
					sec_board_rev, ids[j].name);
				strcpy(ids[j].name, "NON");
				strcpy(ids[i].name, "NON");
				break;
			}
		}
	}
}

void __init sec_santos10_dev_init_early(void)
{
	struct sec_devs_id *dev_ids;
	unsigned int nr_dev_ids;
	unsigned int i;
	size_t count = 0;

	if (sec_board_id == sec_id_santos103g) {
		dev_ids = santos103g_device_ids;
		nr_dev_ids = ARRAY_SIZE(santos103g_device_ids);
	} else if (sec_board_id == sec_id_santos10wifi) {
		dev_ids = santos10wifi_device_ids;
		nr_dev_ids = ARRAY_SIZE(santos10wifi_device_ids);
	} else if (sec_board_id == sec_id_santos10lte) {
		dev_ids = santos10lte_device_ids;
		nr_dev_ids = ARRAY_SIZE(santos10lte_device_ids);
	} else {
		while (1)
			;	/* FIXME: never reached */
		return;
	}

	for (i = 0; i < nr_dev_ids; i++) {
		if (likely(dev_ids[i].rev <= sec_board_rev))
			count += dev_ids[i].nr_devs - 1;
		else
			break;
	}
	count += 1;	/* for the empty termination */

	/* FIXME: this can be a memory leak, because there's no way to free */
	device_ids = kzalloc(sizeof(struct devs_id) * count, GFP_KERNEL);
	if (unlikely(!device_ids))
		return /* -ENOMEM */;

	count = 0;
	for (i = 0; i < nr_dev_ids; i++) {
		if (likely(dev_ids[i].rev > sec_board_rev))
			break;

		memcpy(&device_ids[count], dev_ids[i].devs,
			sizeof(struct devs_id) * dev_ids[i].nr_devs);
		count += dev_ids[i].nr_devs - 1;
	}

	sec_santos10_rm_none_devs(device_ids, count + 1);
}

struct devs_id __init *sec_santos10_get_device_ptr(void)
{
	return device_ids;
}
