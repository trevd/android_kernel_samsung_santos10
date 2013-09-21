/* arch/x86/platform/intel-mid/board-clt3g-dev.c
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

#include "board-clt3g.h"

/* platform data by Intel */
#include <asm/platform_ctp_audio.h>
#include "device_libs/platform_ipc.h"
#include "device_libs/platform_max17042.h"
#include "device_libs/platform_max3111.h"
#include "device_libs/platform_msic_adc.h"
#include "device_libs/platform_msic_gpio.h"
#include "device_libs/platform_msic_power_btn.h"
#include "device_libs/platform_pmic_gpio.h"
#include "device_libs/platform_tc35876x.h"
#include "device_libs/platform_camera.h"
#include "device_libs/platform_hsi_modem.h"
#include "device_libs/platform_ffl_modem.h"
#include "device_libs/platform_edlp_modem.h"
#include "device_libs/platform_s7301.h"
#include "sec_libs/platform_wm8994.h"

#define SEC_DEV_IDS(_rev, _dev_ids)					\
{									\
	.devs = _dev_ids,						\
	.nr_devs = ARRAY_SIZE(_dev_ids),				\
}

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

static struct devs_id __initconst clt3g_devs_r00[] = {
	/* MSIC subdevices */
	{"msic_adc", SFI_DEV_TYPE_IPC, 1, &msic_adc_platform_data,
					&ipc_device_handler},
	{"msic_gpio", SFI_DEV_TYPE_IPC, 1, &msic_gpio_platform_data,
					&ipc_device_handler},
	{"msic_power_btn", SFI_DEV_TYPE_IPC, 1, &msic_power_btn_platform_data,
					&ipc_device_handler},

	/* IPC devices */
	{"pmic_gpio", SFI_DEV_TYPE_IPC, 1, &pmic_gpio_platform_data,
						&ipc_device_handler},
	{"a_gfreq",   SFI_DEV_TYPE_IPC, 0, &no_platform_data,
						&ipc_device_handler},

	/* Audio */
	{"clvcs_audio", SFI_DEV_TYPE_IPC, 1, &ctp_audio_platform_data,
						&ipc_device_handler},
	{"wm8994", SFI_DEV_TYPE_I2C, 1, &wm8994_platform_data, NULL},


	/* I2C devices */
	{"synaptics_ts", SFI_DEV_TYPE_I2C, 0, &synaptics_s7301_platform_data,
					NULL},

	/* Modem */
#ifndef CONFIG_HSI_NO_MODEM
	{"hsi_ifx_modem", SFI_DEV_TYPE_HSI, 0, &hsi_modem_platform_data, NULL},
	{"hsi_ffl_modem", SFI_DEV_TYPE_HSI, 0, &ffl_modem_platform_data, NULL},
	{"hsi_edlp_modem", SFI_DEV_TYPE_HSI, 0, &edlp_modem_platform_data,
						NULL},
#endif

	/* LVDS Bridge */
	{"mipi_tc358764", SFI_DEV_TYPE_I2C, 0, &tc35876x_platform_data, NULL},

	/* termination */
	{},
};

static struct sec_devs_id __initconst clt3g_device_ids[] = {
	/* TODO: the order MUST be from latest to oldest */
	SEC_DEV_IDS(0, clt3g_devs_r00),
};

void __init sec_clt3g_dev_init_early(void)
{
	unsigned int i;
	size_t count = 0;

	for (i = 0; i < ARRAY_SIZE(clt3g_device_ids); i++)
		if (likely(clt3g_device_ids[i].rev <= sec_board_rev))
			count += clt3g_device_ids[i].nr_devs - 1;
	count += 1;	/* for the empty termination */

	/* FIXME: this can be a memory leak, because there's no way to free */
	device_ids = kzalloc(sizeof(struct devs_id) * count, GFP_KERNEL);
	if (unlikely(!device_ids))
		return /* -ENOMEM */;

	count = 0;
	for (i = 0; i < ARRAY_SIZE(clt3g_device_ids); i++) {
		if (likely(clt3g_device_ids[i].rev > sec_board_rev))
			continue;

		memcpy(&device_ids[count], clt3g_device_ids[i].devs,
			sizeof(struct devs_id) * clt3g_device_ids[i].nr_devs);
		count += clt3g_device_ids[i].nr_devs - 1;
	}
}

struct devs_id *sec_clt3g_get_device_ptr(void)
{
	return device_ids;
}
