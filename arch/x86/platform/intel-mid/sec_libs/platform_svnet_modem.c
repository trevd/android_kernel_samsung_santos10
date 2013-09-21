/* arch/x86/platform/intel-mid/sec_libs/platform_svnet_modem.c
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

#include <linux/hsi/hsi.h>
#include <linux/hsi/intel_mid_hsi.h>
#include <asm/intel-mid.h>
#include "platform_svnet_modem.h"


static struct hsi_board_info hsi_info[SVNET_HSI_CLIENT_CNT] = {
	[0] = {
		.name = "svnet_hsi_link_3g",
		.hsi_id = 0,
		.port = 0,
		.archdata = NULL,
		.tx_cfg.speed = 100000,	/* tx clock, kHz */
		.tx_cfg.channels = 8,
		.tx_cfg.mode = HSI_MODE_FRAME,
		.tx_cfg.arb_mode = HSI_ARB_RR,
		.rx_cfg.flow = HSI_FLOW_SYNC,
		.rx_cfg.mode = HSI_MODE_FRAME,
		.rx_cfg.channels = 8
	},
	[1] = {
		.name = "svnet_hsi_link_lte",
		.hsi_id = 0,
		.port = 0,
		.archdata = NULL,
		.tx_cfg.speed = 200000,	/* tx clock, kHz */
		.tx_cfg.channels = 8,
		.tx_cfg.mode = HSI_MODE_FRAME,
		.tx_cfg.arb_mode = HSI_ARB_RR,
		.rx_cfg.flow = HSI_FLOW_PIPE,
		.rx_cfg.mode = HSI_MODE_FRAME,
		.rx_cfg.channels = 8
	},
};

static struct hsi_mid_platform_data mid_info_3g = {
	.tx_dma_channels[0] = -1,
	.tx_dma_channels[1] = 0,
	.tx_dma_channels[2] = 1,
	.tx_dma_channels[3] = 2,
	.tx_dma_channels[4] = 3,
	.tx_dma_channels[5] = -1,
	.tx_dma_channels[6] = -1,
	.tx_dma_channels[7] = -1,
	.tx_sg_entries[0] = 1,
	.tx_sg_entries[1] = 1,
	.tx_sg_entries[2] = 1,
	.tx_sg_entries[3] = 1,
	.tx_sg_entries[4] = 1,
	.tx_sg_entries[5] = 1,
	.tx_sg_entries[6] = 1,
	.tx_sg_entries[7] = 1,
	.tx_fifo_sizes[0] = 128,
	.tx_fifo_sizes[1] = 256,
	.tx_fifo_sizes[2] = 256,
	.tx_fifo_sizes[3] = 256,
	.tx_fifo_sizes[4] = 128,
	.tx_fifo_sizes[5] = -1,
	.tx_fifo_sizes[6] = -1,
	.tx_fifo_sizes[7] = -1,
	.rx_dma_channels[0] = -1,
	.rx_dma_channels[1] = 4,
	.rx_dma_channels[2] = 5,
	.rx_dma_channels[3] = 6,
	.rx_dma_channels[4] = 7,
	.rx_dma_channels[5] = -1,
	.rx_dma_channels[6] = -1,
	.rx_dma_channels[7] = -1,
	.rx_sg_entries[0] = 1,
	.rx_sg_entries[1] = 1,
	.rx_sg_entries[2] = 1,
	.rx_sg_entries[3] = 1,
	.rx_sg_entries[4] = 1,
	.rx_sg_entries[5] = 1,
	.rx_sg_entries[6] = 1,
	.rx_sg_entries[7] = 1,
	.rx_fifo_sizes[0] = 128,
	.rx_fifo_sizes[1] = 256,
	.rx_fifo_sizes[2] = 256,
	.rx_fifo_sizes[3] = 256,
	.rx_fifo_sizes[4] = 128,
	.rx_fifo_sizes[5] = -1,
	.rx_fifo_sizes[6] = -1,
	.rx_fifo_sizes[7] = -1,
};

static struct hsi_mid_platform_data mid_info_lte = {
	.tx_dma_channels[0] = -1,
	.tx_dma_channels[1] = 0,
	.tx_dma_channels[2] = 1,
	.tx_dma_channels[3] = 2,
	.tx_dma_channels[4] = 3,
	.tx_dma_channels[5] = -1,
	.tx_dma_channels[6] = -1,
	.tx_dma_channels[7] = -1,
	.tx_sg_entries[0] = 1,
	.tx_sg_entries[1] = 64,
	.tx_sg_entries[2] = 64,
	.tx_sg_entries[3] = 64,
	.tx_sg_entries[4] = 64,
	.tx_sg_entries[5] = 64,
	.tx_sg_entries[6] = 1,
	.tx_sg_entries[7] = 1,
	.tx_fifo_sizes[0] = 64,
	.tx_fifo_sizes[1] = 256,
	.tx_fifo_sizes[2] = 256,
	.tx_fifo_sizes[3] = 256,
	.tx_fifo_sizes[4] = 128,
	.tx_fifo_sizes[5] = 64,
	.tx_fifo_sizes[6] = -1,
	.tx_fifo_sizes[7] = -1,
	.rx_dma_channels[0] = -1,
	.rx_dma_channels[1] = 4,
	.rx_dma_channels[2] = 5,
	.rx_dma_channels[3] = 6,
	.rx_dma_channels[4] = -1,
	.rx_dma_channels[5] = -1,
	.rx_dma_channels[6] = 7,
	.rx_dma_channels[7] = -1,
	.rx_sg_entries[0] = 1,
	.rx_sg_entries[1] = 1,
	.rx_sg_entries[2] = 1,
	.rx_sg_entries[3] = 1,
	.rx_sg_entries[4] = 1,
	.rx_sg_entries[5] = 1,
	.rx_sg_entries[6] = 1,
	.rx_sg_entries[7] = 1,
	.rx_fifo_sizes[0] = 64,
	.rx_fifo_sizes[1] = 256,
	.rx_fifo_sizes[2] = 256,
	.rx_fifo_sizes[3] = 128,
	.rx_fifo_sizes[4] = 128,
	.rx_fifo_sizes[5] = 64,
	.rx_fifo_sizes[6] = 64,
	.rx_fifo_sizes[7] = 64,
};

void *svnet_modem_platform_data(void *data)
{
	hsi_info[0].platform_data = (void *)&mid_info_3g;
	hsi_info[1].platform_data = (void *)&mid_info_lte;

	return &hsi_info[0];
}
