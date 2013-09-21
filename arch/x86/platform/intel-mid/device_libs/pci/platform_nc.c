/*
 * platform_nc.c: Intel MID CTP NC platform initialization file
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Author: Zhang Dongsheng <dongsheng.zhang@intel.com>
 *
 */

#include <linux/pci.h>

#define NC_VNN_VID_PORT 0x81
#define NC_VNN_VID_REG 0x0d

#define NC_VNN_VID_MASK 0x7f

#define PCI_DEVICE_ID_CTP_NC 0x08c0

/*
 * Customer requested to log the Voltage Identifier for the CTP's Vnn.
 * Add the pci quirks to print it.
 */
static void __devinit nc_pci_early_quirk(struct pci_dev *pci_dev)
{
	int ret;
	u32 cmd, val;
	int port, reg;

	port = NC_VNN_VID_PORT;
	reg = NC_VNN_VID_REG;

	cmd = (0x10 << 24) | (port << 16) | (reg << 8) | (0xf0);
	ret = pci_write_config_dword(pci_dev, 0xd0, cmd);
	if (ret) {
		pr_err("%s write pci config 0xd0 with 0x%x failed\n", __func__,
			cmd);
		goto exit;
	}

	val = 0;
	ret = pci_read_config_dword(pci_dev, 0xd4, &val);
	if (ret) {
		pr_err("%s read pci config 0xd4 failed\n", __func__);
		goto exit;
	}

	pr_info("Port:0x%x Reg:0x%x value:0x%x", port, reg, val);
	pr_info("VNN VID: [0x%x]\n", val & NC_VNN_VID_MASK);

	return;

exit:
	pr_err("VNN VID: read failed");

}

DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_CTP_NC,
	nc_pci_early_quirk);
