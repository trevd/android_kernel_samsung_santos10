/* arch/x86/platform/intel-mid/board-santos10lte-modems.c
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <asm/intel-mid.h>

#include <linux/platform_data/sipc_def.h>
#include <linux/platform_data/modem.h>


/* lte target platform data */
static struct modem_io_t lte_io_devices[] = {
	[0] = {
		.name = "umts_ipc0",
		.id = SIPC5_CH_ID_FMT_0,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
		.attr = IODEV_ATTR(ATTR_SIPC5),
	},
	[1] = {
		.name = "umts_rfs0",
		.id = SIPC5_CH_ID_RFS_0,
		.format = IPC_RFS,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
		.attr = IODEV_ATTR(ATTR_SIPC5) | IODEV_ATTR(ATTR_LEGACY_RFS),
	},
	[2] = {
		.name = "umts_boot0",
		.id = 0x0,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[3] = {
		.name = "multipdp",
		.id = 0x0,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
		.attr = IODEV_ATTR(ATTR_SIPC5),
	},
	[4] = {
		.name = "umts_router",
		.id = SIPC_CH_ID_BT_DUN,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
		.attr = IODEV_ATTR(ATTR_SIPC5),
	},
	[5] = {
		.name = "umts_csd",
		.id = SIPC_CH_ID_CS_VT_DATA,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
		.attr = IODEV_ATTR(ATTR_SIPC5),
	},
	[6] = {
		.name = "umts_dm0",
		.id = SIPC_CH_ID_CPLOG1,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[7] = {
		.name = "umts_ramdump0",
		.id = 0x0,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[8] = {
		.name = "umts_loopback0",
		.id = SIPC_CH_ID_LOOPBACK2,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[9] = {
		.name = "rmnet0",
		.id = SIPC_CH_ID_PDP_0,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[10] = {
		.name = "rmnet1",
		.id = SIPC_CH_ID_PDP_1,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[11] = {
		.name = "rmnet2",
		.id = SIPC_CH_ID_PDP_2,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[12] = {
		.name = "rmnet3",
		.id = SIPC_CH_ID_PDP_3,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
	},
	[13] = {
		.name = "ipc_loopback0",
		.id = SIPC5_CH_ID_FMT_9,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI_EDLP),
		.attr = IODEV_ATTR(ATTR_SIPC5),
	},
};

enum {
	GPIO_CP_ON = 0,
	GPIO_CP_RST,
	GPIO_RESET_REQ,
	GPIO_PDA_ACTIVE,
	GPIO_PHONE_ACTIVE,
	GPIO_CP_DUMP_INT,
	GPIO_AP_DUMP_INT,
};

struct gpio lte_modem_gpios[] __initdata = {
	[GPIO_CP_ON] = {
		.flags  = GPIOF_OUT_INIT_LOW,
		.label  = "CP_ON",
	},
	[GPIO_CP_RST] = {
		.flags  = GPIOF_OUT_INIT_LOW,
		.label  = "CP_PMU_RST",
	},
	[GPIO_RESET_REQ] = {
		.flags  = GPIOF_OUT_INIT_LOW,
		.label  = "MODEM_RESET_BB_N",
	},
	[GPIO_PDA_ACTIVE] = {
		.flags  = GPIOF_OUT_INIT_LOW,
		.label  = "PDA_ACTIVE",
	},
	[GPIO_PHONE_ACTIVE] = {
		.flags  = GPIOF_IN,
		.label  = "PHONE_ACTIVE",
	},
	[GPIO_CP_DUMP_INT] = {
		.flags  = GPIOF_IN,
		.label  = "CP_DUMP_INT",
	},
	[GPIO_AP_DUMP_INT] = {
		.flags  = GPIOF_OUT_INIT_LOW,
		.label  = "AP_DUMP_INT",
	},
};

static struct modem_data lte_modem_data = {
	.name = "xmm7160",

	.modem_type = IMC_XMM7160,
	.link_types = LINKTYPE(LINKDEV_MIPI_EDLP),
	.modem_net = LTE_NETWORK,
	.use_handover = false,
	.ipc_version = SIPC_VER_50,

	.num_iodevs = ARRAY_SIZE(lte_io_devices),
	.iodevs = lte_io_devices,
};

static int __init lte_modem_cfg_gpio(void)
{
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(lte_modem_gpios); i++)
		lte_modem_gpios[i].gpio =
				get_gpio_by_name(lte_modem_gpios[i].label);
	err = gpio_request_array(lte_modem_gpios, ARRAY_SIZE(lte_modem_gpios));
	if (unlikely(err)) {
		pr_err("(%s): can't request gpios!\n", __func__);
		return -ENODEV;
	}

	lte_modem_data.gpio_cp_on = lte_modem_gpios[GPIO_CP_ON].gpio;
	lte_modem_data.gpio_reset_req_n = lte_modem_gpios[GPIO_RESET_REQ].gpio;
	lte_modem_data.gpio_cp_reset = lte_modem_gpios[GPIO_CP_RST].gpio;
	lte_modem_data.gpio_pda_active = lte_modem_gpios[GPIO_PDA_ACTIVE].gpio;
	lte_modem_data.gpio_phone_active =
				lte_modem_gpios[GPIO_PHONE_ACTIVE].gpio;
	lte_modem_data.gpio_cp_dump_int =
				lte_modem_gpios[GPIO_CP_DUMP_INT].gpio;
	lte_modem_data.gpio_ap_dump_int =
				lte_modem_gpios[GPIO_AP_DUMP_INT].gpio;

	mif_debug("lte_modem_cfg_gpio done\n");
	return 0;
}

/* if use more than one modem device, then set id num */
static struct platform_device lte_modem = {
	.name = "mif_sipc5",
	.id = -1,
	.dev = {
		.platform_data = &lte_modem_data,
	},
};

/* late_initcall */
int __init sec_santos10lte_modems_init(void)
{
	int err;

	err = lte_modem_cfg_gpio();
	if (unlikely(err))
		return err;

	platform_device_register(&lte_modem);

	mif_info("board init_lte_modem done\n");

	return 0;
}
