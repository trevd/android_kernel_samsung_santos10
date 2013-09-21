/* arch/x86/platform/intel-mid/board-santos10-modems.c
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
#include <linux/lnw_gpio.h>

#include <asm/intel_scu_flis.h>
#include <asm/intel-mid.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

#include <linux/platform_data/modem_v2.h>


/* umts target platform data */
static struct modem_io_t umts_io_devices[] = {
	[0] = {
		.name = "umts_ipc0",
		.id = 0x1,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[1] = {
		.name = "umts_rfs0",
		.id = 0x41,
		.format = IPC_RFS,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[2] = {
		.name = "rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[3] = {
		.name = "umts_boot0",
		.id = 0x0,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[4] = {
		.name = "rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[5] = {
		.name = "rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[6] = {
		.name = "multipdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[7] = {
		.name = "umts_ramdump0",
		.id = 0x0,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[8] = {
		.name = "umts_router", /* AT Iface & Dial-up */
		.id = 0x39,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[9] = {
		.name = "umts_csd",
		.id = 0x21,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[10] = {
		.name = "umts_loopback0",
		.id = 0x3F,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
	},
	[11] = {
		.name = "ipc_loopback0",
		.id = 0x40,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_MIPI),
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

struct gpio modem_gpios[] __initdata = {
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

static struct modem_data umts_modem_data = {
	.name = "xmm6262",

	.modem_type = IMC_XMM6262,
	.link_types = LINKTYPE(LINKDEV_MIPI),
	.modem_net = UMTS_NETWORK,
	.use_handover = false,
	.ipc_version = SIPC_VER_40,

	.num_iodevs = ARRAY_SIZE(umts_io_devices),
	.iodevs = umts_io_devices,
};

static int __init umts_modem_cfg_gpio(void)
{
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(modem_gpios); i++)
		modem_gpios[i].gpio = get_gpio_by_name(modem_gpios[i].label);
	err = gpio_request_array(modem_gpios, ARRAY_SIZE(modem_gpios));
	if (unlikely(err)) {
		pr_err("(%s): can't request gpios!\n", __func__);
		return -ENODEV;
	}

	umts_modem_data.gpio_cp_on = modem_gpios[GPIO_CP_ON].gpio;
	umts_modem_data.gpio_reset_req_n = modem_gpios[GPIO_RESET_REQ].gpio;
	umts_modem_data.gpio_cp_reset = modem_gpios[GPIO_CP_RST].gpio;
	umts_modem_data.gpio_pda_active = modem_gpios[GPIO_PDA_ACTIVE].gpio;
	umts_modem_data.gpio_phone_active = modem_gpios[GPIO_PHONE_ACTIVE].gpio;
	umts_modem_data.gpio_cp_dump_int = modem_gpios[GPIO_CP_DUMP_INT].gpio;
	umts_modem_data.gpio_ap_dump_int = modem_gpios[GPIO_AP_DUMP_INT].gpio;

	mif_debug("umts_modem_cfg_gpio done\n");
	return 0;
}

/* if use more than one modem device, then set id num */
static struct platform_device umts_modem = {
	.name = "mif_sipc4",
	.id = -1,
	.dev = {
		.platform_data = &umts_modem_data,
	},
};

/* late_initcall */
int __init sec_santos103g_modems_init(void)
{
	int err;

	err = umts_modem_cfg_gpio();
	if (unlikely(err))
		return err;

	platform_device_register(&umts_modem);

	mif_info("board init_modem done\n");
	return 0;
}

void __init sec_santos10wifi_no_modems_init_rest(void)
{
	struct intel_muxtbl *muxtbl;
	int i;

	for (i = 0; i < ARRAY_SIZE(modem_gpios); i++) {
		muxtbl = intel_muxtbl_find_by_net_name(modem_gpios[i].label);
		if (unlikely(!muxtbl))
			continue;
		muxtbl->mux.alt_func = LNW_GPIO;
		muxtbl->mux.pull = DOWN_20K;
		muxtbl->mux.pin_direction = INPUT_ONLY;
		muxtbl->mux.open_drain = OD_ENABLE;
		muxtbl->gpio.label = kasprintf(GFP_KERNEL, "%s.nc",
				muxtbl->gpio.label);
	}
}
