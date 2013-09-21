/* /linux/drivers/modem_if_v2/modem_link_device_mipi.c
 *
 * Copyright (C) 2012 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/if_arp.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/hsi/hsi.h>

#include <linux/platform_data/modem_v2.h>
#include "modem_prj.h"
#include "modem_link_device_mipi.h"
#include "modem_utils.h"


/** mipi hsi link configuration
 *
 * change inside this table, do not fix if_hsi_init() directly
 *
 */
static struct hsi_channel_info ch_info[HSI_NUM_OF_USE_CHANNELS] = {
	{ "cmd", WQ_HIGHPRI, HSI_RX_0_CHAN_BUF_SIZE, 0, 0, 0 },
	{ "ipc", 0, HSI_RX_1_CHAN_BUF_SIZE, 0, 0, 1 },
	{ "m_raw", WQ_HIGHPRI, HSI_RX_2_CHAN_BUF_SIZE, 0, 0, 1 },
	{ "rfs", 0, HSI_RX_3_CHAN_BUF_SIZE, 0, 0, 1 },
	{ "extra", 0, HSI_RX_4_CHAN_BUF_SIZE, 0, 0, 1 },
};

static int mipi_hsi_init_communication(struct link_device *ld,
			struct io_device *iod)
{
	struct mipi_link_device *mipi_ld = to_mipi_link_device(ld);

	switch (iod->format) {
	case IPC_FMT:
		if (iod->id == 0x1)
			return if_hsi_init_handshake(mipi_ld,
						HSI_INIT_MODE_NORMAL);

	case IPC_BOOT:
	case IPC_RAMDUMP:
	case IPC_RFS:
	case IPC_RAW:
	default:
		return 0;
	}
}

static void mipi_hsi_terminate_communication(
			struct link_device *ld, struct io_device *iod)
{
	struct mipi_link_device *mipi_ld = to_mipi_link_device(ld);

	switch (iod->format) {
	case IPC_FMT:
	case IPC_RFS:
	case IPC_RAW:
	case IPC_RAMDUMP:
		if (wake_lock_active(&mipi_ld->wlock)) {
			wake_unlock(&mipi_ld->wlock);
			mipi_debug("wake_unlock\n");
		}
		break;

	case IPC_BOOT:
	default:
		break;
	}
}

static int mipi_hsi_ioctl(struct link_device *ld, struct io_device *iod,
		unsigned int cmd, unsigned long arg)
{
	struct mipi_link_device *mipi_ld = to_mipi_link_device(ld);
	int err = 0;
	int speed;

	switch (cmd) {
	case IOCTL_BOOT_LINK_SPEED_CHG:
		if (copy_from_user(&speed, (void __user *)arg, sizeof(int)))
			return -EFAULT;

		if (speed == 0)
			return if_hsi_init_handshake(mipi_ld,
					HSI_INIT_MODE_FLASHLESS_BOOT);
		else {
			if (iod->format == IPC_RAMDUMP)
				return if_hsi_init_handshake(mipi_ld,
					HSI_INIT_MODE_CP_RAMDUMP);
			else
				return if_hsi_init_handshake(mipi_ld,
					HSI_INIT_MODE_FLASHLESS_BOOT_EBL);
		}

	default:
		mif_err("%s: ERR! invalid cmd 0x%08X\n", ld->name, cmd);
		err = -EINVAL;
		break;
	}

	return err;
}

static int mipi_hsi_send(struct link_device *ld, struct io_device *iod,
			struct sk_buff *skb)
{
	int ret;
	struct mipi_link_device *mipi_ld = to_mipi_link_device(ld);
	size_t tx_size = skb->len;
	unsigned int ch_id;

	/* save io device into cb area */
	*((struct io_device **)skb->cb) = iod;

	switch (iod->format) {
	/* handle asynchronous write */
	case IPC_RAW:
	case IPC_MULTI_RAW:
		ch_id = HSI_RAW_CHANNEL;
		break;
	case IPC_FMT:
		ch_id = HSI_FMT_CHANNEL;
		break;
	case IPC_RFS:
		ch_id = HSI_RFS_CHANNEL;
		break;

	/* handle synchronous write */
	case IPC_BOOT:
		ret = if_hsi_write(mipi_ld, HSI_FLASHLESS_CHANNEL,
				(u32 *)skb->data, skb->len, HSI_TX_MODE_SYNC);
		if (ret < 0) {
			mipi_err("write fail : %d\n", ret);
			dev_kfree_skb_any(skb);
			return ret;
		} else
			mipi_debug("write Done\n");
		dev_kfree_skb_any(skb);
		return tx_size;

	case IPC_RAMDUMP:
		ret = if_hsi_write(mipi_ld, HSI_CP_RAMDUMP_CHANNEL,
				(u32 *)skb->data, skb->len, HSI_TX_MODE_SYNC);
		if (ret < 0) {
			mipi_err("write fail : %d\n", ret);
			dev_kfree_skb_any(skb);
			return ret;
		} else
			mipi_debug("write Done\n");
		dev_kfree_skb_any(skb);
		return ret;

	default:
		mipi_err("does not support %d type\n", iod->format);
		dev_kfree_skb_any(skb);
		return -ENOTSUPP;
	}

	/* set wake_lock to prevent to sleep before tx_work thread run */
	if (!wake_lock_active(&mipi_ld->wlock)) {
		wake_lock_timeout(&mipi_ld->wlock, HZ / 2);
		mipi_debug("wake_lock\n");
	}

	/* en queue skb data */
	skb_queue_tail(&mipi_ld->hsi_channels[ch_id].tx_q, skb);

	mipi_debug("ch %d: len:%d, %p:%p:%p\n", ch_id, skb->len, skb->prev,
				skb, skb->next);
	queue_work(mipi_ld->hsi_channels[ch_id].tx_wq,
				&mipi_ld->hsi_channels[ch_id].tx_w);

	return tx_size;
}

static void mipi_hsi_send_work(struct work_struct *work)
{
	struct if_hsi_channel *channel =
				container_of(work, struct if_hsi_channel, tx_w);
	struct mipi_link_device *mipi_ld = to_mipi_link_device(channel->ld);
	struct sk_buff *skb;
	struct sk_buff_head *tx_q = &channel->tx_q;
	void *buff;
	int ret;
	unsigned pk_size, bulk_size;

check_online:
	/* check it every tx transcation */
	if ((channel->ld->com_state != COM_ONLINE) ||
		(mipi_ld->ld.mc->phone_state != STATE_ONLINE)) {
		mipi_debug("CP not ready\n");
		return;
	}

	if (skb_queue_len(tx_q)) {
		mipi_debug("ch%d: tx qlen : %d\n", channel->channel_id,
						skb_queue_len(tx_q));

		skb = skb_dequeue(tx_q);
		if (!skb)
			return;

		if (channel->channel_id == HSI_RAW_CHANNEL) {
			bulk_size = 0;
			while (skb) {
				if (bulk_size + skb->len < MIPI_BULK_TX_SIZE) {
					memcpy(mipi_ld->bulk_tx_buf + bulk_size,
							skb->data, skb->len);
					bulk_size += skb->len;
					skb_queue_head(&mipi_ld->bulk_txq, skb);
				} else {
					skb_queue_head(tx_q, skb);
					break;
				}
				skb = skb_dequeue(tx_q);
			}
			buff = mipi_ld->bulk_tx_buf;
			pk_size = bulk_size;
		} else {
			buff = skb->data;
			pk_size = skb->len;
		}

		ret = if_hsi_protocol_send(mipi_ld, channel->channel_id,
					buff, pk_size);
		if (ret < 0) {
			if (channel->channel_id == HSI_RAW_CHANNEL) {
				skb = skb_dequeue(&mipi_ld->bulk_txq);
				while (skb) {
					skb_queue_head(tx_q, skb);
					skb = skb_dequeue(&mipi_ld->bulk_txq);
				}
			} else
				skb_queue_head(tx_q, skb);
		} else {
			if ((channel->channel_id == HSI_FMT_CHANNEL) ||
				(channel->channel_id == HSI_RFS_CHANNEL))
				print_hex_dump(KERN_DEBUG,
					channel->channel_id == HSI_FMT_CHANNEL ?
					"IPC-TX: " : "RFS-TX: ",
					DUMP_PREFIX_NONE,
					1, 1,
					(void *)skb->data,
					skb->len <= 16 ?
					(size_t)skb->len :
					(size_t)16, false);

			if (channel->channel_id == HSI_RAW_CHANNEL)
				skb_queue_purge(&mipi_ld->bulk_txq);
			else
				dev_kfree_skb_any(skb);
		}

		goto check_online;
	}
}

static int __devinit if_hsi_probe(struct device *dev);
static void if_hsi_shutdown(struct device *dev);
static struct hsi_client_driver if_hsi_driver = {
	.driver = {
		   .name = "svnet_hsi_link_3g",
		   .owner = THIS_MODULE,
		   .probe = if_hsi_probe,
		   .shutdown = if_hsi_shutdown,
		   },
};

static int if_hsi_set_wakeline(struct mipi_link_device *mipi_ld,
			unsigned int state)
{
	unsigned long int flags;
	u32 nop_command;

	spin_lock_irqsave(&mipi_ld->acwake_lock, flags);
	if (atomic_read(&mipi_ld->acwake) == state) {
		spin_unlock_irqrestore(&mipi_ld->acwake_lock, flags);
		return 0;
	}
	atomic_set(&mipi_ld->acwake, state);
	spin_unlock_irqrestore(&mipi_ld->acwake_lock, flags);

	if (state)
		hsi_start_tx(mipi_ld->client);
	else {
		if ((mipi_ld->ld.com_state == COM_ONLINE) &&
			(mipi_ld->ld.mc->phone_state == STATE_ONLINE)) {
			/* Send the NOP command to avoid tailing bits issue */
			nop_command = if_hsi_create_cmd(HSI_LL_MSG_NOP,
						HSI_CONTROL_CHANNEL, NULL);
			if_hsi_write(mipi_ld, HSI_CONTROL_CHANNEL, &nop_command,
						4, HSI_TX_MODE_ASYNC);
			mipi_debug("SEND CMD : %08x\n", nop_command);
		}

		hsi_stop_tx(mipi_ld->client);
	}

	mipi_info("ACWAKE(%d)\n", state);
	return 0;
}

static void if_hsi_acwake_down_func(unsigned long data)
{
	int i;
	struct if_hsi_channel *channel;
	struct mipi_link_device *mipi_ld = (struct mipi_link_device *)data;

	for (i = 0; i < HSI_NUM_OF_USE_CHANNELS; i++) {
		channel = &mipi_ld->hsi_channels[i];

		if ((channel->send_step != STEP_IDLE) ||
			(channel->recv_step != STEP_IDLE)) {
			mipi_debug("%s: ch %d is using, fail to acwake down\n",
						__func__, i);
			mod_timer(&mipi_ld->hsi_acwake_down_timer,
					jiffies + HSI_ACWAKE_DOWN_TIMEOUT);
			return;
		}
	}
	if_hsi_set_wakeline(mipi_ld, 0);
}

static int if_hsi_open_channel(struct mipi_link_device *mipi_ld,
			unsigned int ch_num)
{
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[ch_num];

	channel->send_step = STEP_IDLE;
	channel->recv_step = STEP_IDLE;

	mipi_debug("hsi_open Done : %d\n", channel->channel_id);
	return 0;
}

static int if_hsi_close_channel(struct mipi_link_device *mipi_ld,
			unsigned int ch_num)
{
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[ch_num];

	channel->send_step = STEP_CLOSED;
	channel->recv_step = STEP_CLOSED;

	mipi_debug("hsi_close Done : %d\n", channel->channel_id);
	return 0;
}

static void if_hsi_start_work(struct work_struct *work)
{
	int ret;
	u32 start_cmd = 0xC2;
	struct mipi_link_device *mipi_ld =
			container_of(work, struct mipi_link_device,
						start_work.work);

	mipi_ld->ld.com_state = COM_HANDSHAKE;
	ret = if_hsi_protocol_send(mipi_ld, HSI_CMD_CHANNEL, &start_cmd, 1);
	if (ret < 0) {
		mipi_err("Send HandShake 0xC2 fail: %d\n", ret);
		schedule_delayed_work(&mipi_ld->start_work, 3 * HZ);
		return;
	}

	mipi_ld->ld.com_state = COM_ONLINE;
	mipi_info("Send HandShake 0xC2 Done\n");
}

static int if_hsi_init_handshake(struct mipi_link_device *mipi_ld, int mode)
{
	int ret;
	int i;

	if (!mipi_ld->client) {
		mipi_err("no hsi client data\n");
		return -ENODEV;
	}

	switch (mode) {
	case HSI_INIT_MODE_NORMAL:
		if (timer_pending(&mipi_ld->hsi_acwake_down_timer))
			del_timer(&mipi_ld->hsi_acwake_down_timer);

		if (mipi_ld->ld.mc->phone_state != STATE_ONLINE) {
			mipi_err("CP not ready\n");
			return -ENODEV;
		}

		for (i = 0; i < HSI_NUM_OF_USE_CHANNELS; i++) {
			if_hsi_close_channel(mipi_ld, i);
			if_hsi_open_channel(mipi_ld, i);
		}

		if (!hsi_port_claimed(mipi_ld->client)) {
			mipi_err("hsi_port is not ready\n");
			return -EACCES;
		}

		ret = hsi_flush(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_flush failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		mipi_ld->client->tx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->tx_cfg.channels = 8;
		mipi_ld->client->tx_cfg.speed = 100000; /* Speed: 100MHz */
		mipi_ld->client->tx_cfg.arb_mode = HSI_ARB_RR;
		mipi_ld->client->rx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->rx_cfg.channels = 8;
		mipi_ld->client->rx_cfg.flow = HSI_FLOW_SYNC;

		/* Setup the HSI controller */
		ret = hsi_setup(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_setup failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		if (mipi_ld->ld.com_state != COM_ONLINE)
			mipi_ld->ld.com_state = COM_HANDSHAKE;

		ret = if_hsi_read_async(mipi_ld, HSI_CONTROL_CHANNEL, 4);
		if (ret)
			mipi_err("if_hsi_read_async fail : %d\n", ret);

		if (mipi_ld->ld.com_state != COM_ONLINE)
			schedule_delayed_work(&mipi_ld->start_work, 3 * HZ);

		mipi_debug("hsi_init_handshake Done : MODE_NORMAL\n");
		return 0;

	case HSI_INIT_MODE_FLASHLESS_BOOT:
		mipi_ld->ld.com_state = COM_BOOT;

		if (timer_pending(&mipi_ld->hsi_acwake_down_timer))
			del_timer(&mipi_ld->hsi_acwake_down_timer);

		if_hsi_close_channel(mipi_ld, HSI_FLASHLESS_CHANNEL);
		if_hsi_open_channel(mipi_ld, HSI_FLASHLESS_CHANNEL);

		if (hsi_port_claimed(mipi_ld->client)) {
			if_hsi_set_wakeline(mipi_ld, 0);
			mipi_debug("Releasing the HSI controller\n");
			hsi_release_port(mipi_ld->client);
		}

		/* Claim the HSI port */
		ret = hsi_claim_port(mipi_ld->client, 1);
		if (unlikely(ret)) {
			mipi_err("hsi_claim_port failed (%d)\n", ret);
			return ret;
		}

		mipi_ld->client->tx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->tx_cfg.channels = 1;
		mipi_ld->client->tx_cfg.speed = 25000; /* Speed: 25MHz */
		mipi_ld->client->tx_cfg.arb_mode = HSI_ARB_RR;
		mipi_ld->client->rx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->rx_cfg.channels = 1;
		mipi_ld->client->rx_cfg.flow = HSI_FLOW_SYNC;

		/* Setup the HSI controller */
		ret = hsi_setup(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_setup failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		/* Restore the events callback*/
		hsi_register_port_event(mipi_ld->client, if_hsi_port_event);

		if_hsi_set_wakeline(mipi_ld, 1);

		ret = if_hsi_read_async(mipi_ld, HSI_FLASHLESS_CHANNEL, 4);
		if (ret)
			mipi_err("if_hsi_read_async fail : %d\n", ret);

		mipi_debug("hsi_init_handshake Done : FLASHLESS_BOOT\n");
		return 0;

	case HSI_INIT_MODE_FLASHLESS_BOOT_EBL:
		mipi_ld->ld.com_state = COM_BOOT_EBL;

		if (!hsi_port_claimed(mipi_ld->client)) {
			mipi_err("hsi_port is not ready\n");
			return -EACCES;
		}

		ret = hsi_flush(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_flush failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		mipi_ld->client->tx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->tx_cfg.channels = 1;
		mipi_ld->client->tx_cfg.speed = 100000; /* Speed: 100MHz */
		mipi_ld->client->tx_cfg.arb_mode = HSI_ARB_RR;
		mipi_ld->client->rx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->rx_cfg.channels = 1;
		mipi_ld->client->rx_cfg.flow = HSI_FLOW_SYNC;

		/* Setup the HSI controller */
		ret = hsi_setup(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_setup failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		ret = if_hsi_read_async(mipi_ld, HSI_FLASHLESS_CHANNEL, 4);
		if (ret)
			mipi_err("if_hsi_read_async fail : %d\n", ret);

		mipi_debug("hsi_init_handshake Done : FLASHLESS_BOOT_EBL\n");
		return 0;

	case HSI_INIT_MODE_CP_RAMDUMP:
		mipi_ld->ld.com_state = COM_CRASH;

		if (!hsi_port_claimed(mipi_ld->client)) {
			mipi_err("hsi_port is not ready\n");
			return -EACCES;
		}

		ret = hsi_flush(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_flush failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		mipi_ld->client->tx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->tx_cfg.channels = 1;
		mipi_ld->client->tx_cfg.speed = 100000; /* Speed: 100MHz */
		mipi_ld->client->tx_cfg.arb_mode = HSI_ARB_RR;
		mipi_ld->client->rx_cfg.mode = HSI_MODE_FRAME;
		mipi_ld->client->rx_cfg.channels = 1;
		mipi_ld->client->rx_cfg.flow = HSI_FLOW_SYNC;

		/* Setup the HSI controller */
		ret = hsi_setup(mipi_ld->client);
		if (unlikely(ret)) {
			mipi_err("hsi_setup failed (%d)\n", ret);
			hsi_release_port(mipi_ld->client);
			return ret;
		}

		ret = if_hsi_read_async(mipi_ld, HSI_CP_RAMDUMP_CHANNEL,
					DUMP_ERR_INFO_SIZE * 4);
		if (ret)
			mipi_err("if_hsi_read_async fail : %d\n", ret);

		mipi_debug("hsi_init_handshake Done : RAMDUMP\n");
		return 0;

	default:
		return -EINVAL;
	}
}

static void if_hsi_conn_err_recovery(struct mipi_link_device *mipi_ld)
{
	int i;
	int ret;
	unsigned long int flags;

	if (timer_pending(&mipi_ld->hsi_acwake_down_timer))
		del_timer(&mipi_ld->hsi_acwake_down_timer);

	for (i = 0; i < HSI_NUM_OF_USE_CHANNELS; i++) {
		if_hsi_close_channel(mipi_ld, i);
		if_hsi_open_channel(mipi_ld, i);
	}

	ret = if_hsi_read_async(mipi_ld, HSI_CONTROL_CHANNEL, 4);
	if (ret)
		mipi_err("if_hsi_read_async fail : %d\n", ret);

	mipi_info("if_hsi_conn_err_recovery Done\n");
}

static u32 if_hsi_create_cmd(u32 cmd_type, int ch, void *arg)
{
	u32 cmd = 0;
	unsigned int size = 0;

	switch (cmd_type) {
	case HSI_LL_MSG_BREAK:
		return 0;

	case HSI_LL_MSG_CONN_CLOSED:
		cmd =  ((HSI_LL_MSG_CONN_CLOSED & 0x0000000F) << 28)
					|((ch & 0x000000FF) << 24);
		return cmd;

	case HSI_LL_MSG_ACK:
		size = *(unsigned int *)arg;

		cmd = ((HSI_LL_MSG_ACK & 0x0000000F) << 28)
			|((ch & 0x000000FF) << 24) | ((size & 0x00FFFFFF));
		return cmd;

	case HSI_LL_MSG_NAK:
		cmd = ((HSI_LL_MSG_NAK & 0x0000000F) << 28)
					|((ch & 0x000000FF) << 24);
		return cmd;

	case HSI_LL_MSG_OPEN_CONN_OCTET:
		size = *(unsigned int *)arg;

		cmd = ((HSI_LL_MSG_OPEN_CONN_OCTET & 0x0000000F)
				<< 28) | ((ch & 0x000000FF) << 24)
					| ((size & 0x00FFFFFF));
		return cmd;

	case HSI_LL_MSG_NOP:
		cmd = HSI_LL_MSG_NOP_CHECK_PATTERN;
		cmd |= ((HSI_LL_MSG_NOP & 0x0000000F) << 28);
		return cmd;

	case HSI_LL_MSG_OPEN_CONN:
	case HSI_LL_MSG_CONF_RATE:
	case HSI_LL_MSG_CANCEL_CONN:
	case HSI_LL_MSG_CONN_READY:
	case HSI_LL_MSG_ECHO:
	case HSI_LL_MSG_INFO_REQ:
	case HSI_LL_MSG_INFO:
	case HSI_LL_MSG_ALLOCATE_CH:
	case HSI_LL_MSG_RELEASE_CH:
	case HSI_LL_MSG_INVALID:
	default:
		mipi_err("ERROR... CMD Not supported : %08x\n",
					cmd_type);
		return -EINVAL;
	}
}

static int if_hsi_send_command(struct mipi_link_device *mipi_ld,
			u32 cmd_type, int ch, u32 param)
{
	unsigned long int flags;
	struct if_hsi_channel *channel =
				&mipi_ld->hsi_channels[HSI_CONTROL_CHANNEL];
	u32 command;
	int ret;

	command = if_hsi_create_cmd(cmd_type, ch, &param);
	mipi_debug("made command : %08x\n", command);

	mod_timer(&mipi_ld->hsi_acwake_down_timer,
			jiffies + HSI_ACWAKE_DOWN_TIMEOUT);
	if_hsi_set_wakeline(mipi_ld, 1);

	ret = if_hsi_write(mipi_ld, channel->channel_id, &command, 4,
				HSI_TX_MODE_ASYNC);
	if (ret < 0) {
		mipi_err("write command fail : %d\n", ret);
		return ret;
	}

	return 0;
}

static int if_hsi_decode_cmd(struct mipi_link_device *mipi_ld,
			u32 *cmd_data, u32 *cmd, u32 *ch, u32 *param)
{
	u32 data = *cmd_data;
	u8 lrc_cal, lrc_act;
	u8 val1, val2, val3;

	*cmd = ((data & 0xF0000000) >> 28);
	switch (*cmd) {
	case HSI_LL_MSG_BREAK:
		return 0;

	case HSI_LL_MSG_OPEN_CONN:
		*ch = ((data & 0x0F000000) >> 24);
		*param = ((data & 0x00FFFF00) >> 8);
		val1 = ((data & 0xFF000000) >> 24);
		val2 = ((data & 0x00FF0000) >> 16);
		val3 = ((data & 0x0000FF00) >>  8);
		lrc_act = (data & 0x000000FF);
		lrc_cal = val1 ^ val2 ^ val3;

		if (lrc_cal != lrc_act) {
			mipi_err("CAL is broken\n");
			return -1;
		}
		return 0;

	case HSI_LL_MSG_CONN_READY:
	case HSI_LL_MSG_CONN_CLOSED:
	case HSI_LL_MSG_CANCEL_CONN:
	case HSI_LL_MSG_NAK:
		*ch = ((data & 0x0F000000) >> 24);
		return 0;

	case HSI_LL_MSG_ACK:
		*ch = ((data & 0x0F000000) >> 24);
		*param = (data & 0x00FFFFFF);
		return 0;

	case HSI_LL_MSG_CONF_RATE:
		*ch = ((data & 0x0F000000) >> 24);
		*param = ((data & 0x0F000000) >> 24);
		return 0;

	case HSI_LL_MSG_OPEN_CONN_OCTET:
		*ch = ((data & 0x0F000000) >> 24);
		*param = (data & 0x00FFFFFF);
		return 0;

	case HSI_LL_MSG_NOP:
		*ch = 0;
		if ((data & 0x00FFFFFF) != HSI_LL_MSG_NOP_CHECK_PATTERN) {
			mipi_err("NOP pattern not match:%x\n", data);
			return -1;
		}
		return 0;

	case HSI_LL_MSG_ECHO:
	case HSI_LL_MSG_INFO_REQ:
	case HSI_LL_MSG_INFO:
	case HSI_LL_MSG_ALLOCATE_CH:
	case HSI_LL_MSG_RELEASE_CH:
	case HSI_LL_MSG_INVALID:
	default:
		mipi_err("Invalid command received : %08x\n", *cmd);
					*cmd = HSI_LL_MSG_INVALID;
					*ch  = HSI_LL_INVALID_CHANNEL;
		return -1;
	}
	return 0;
}

static int if_hsi_rx_cmd_handle(struct mipi_link_device *mipi_ld, u32 cmd,
			u32 ch, u32 param)
{
	int ret;
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[ch];

	mipi_debug("if_hsi_rx_cmd_handle cmd=0x%x, ch=%d, param=%d\n",
				cmd, ch, param);

	switch (cmd) {
	case HSI_LL_MSG_BREAK:
		mipi_ld->ld.com_state = COM_HANDSHAKE;
		mipi_err("Command MSG_BREAK Received\n");

		/* if_hsi_conn_err_recovery(mipi_ld); */

		if_hsi_send_command(mipi_ld, HSI_LL_MSG_BREAK,
					HSI_CONTROL_CHANNEL, 0);
		mipi_err("Send MSG BREAK TO CP\n");

		/* Send restart command */
		schedule_delayed_work(&mipi_ld->start_work, 0);
		break;

	case HSI_LL_MSG_OPEN_CONN_OCTET:
		switch (channel->recv_step) {
		case STEP_IDLE:
			channel->recv_step = STEP_TO_ACK;

			ret = if_hsi_send_command(mipi_ld, HSI_LL_MSG_ACK, ch,
						param);
			if (ret) {
				mipi_err("if_hsi_send_command fail=%d\n", ret);
				return ret;
			}

			channel->packet_size = param;
			channel->recv_step = STEP_RX;

			if (param % 4)
				param += (4 - (param % 4));
			ret = if_hsi_read_async(mipi_ld, channel->channel_id,
						param);
			if (ret) {
				mipi_err("if_hsi_read_async fail : %d\n", ret);
				return ret;
			}
			break;

		default:
			mipi_err("got open in wrong step : %d\n",
						channel->recv_step);

			/* mipi_err("send NACK\n");
			ret = if_hsi_send_command(mipi_ld, HSI_LL_MSG_NAK, ch,
						param);
			if (ret) {
				mipi_err("if_hsi_send_command fail=%d\n", ret);
				return ret;
			} */
			break;
		}
		break;

	case HSI_LL_MSG_ACK:
	case HSI_LL_MSG_NAK:
		switch (channel->send_step) {
		case STEP_WAIT_FOR_ACK:
		case STEP_SEND_OPEN_CONN:
			if (cmd == HSI_LL_MSG_ACK) {
				channel->got_nack = 0;
				mipi_debug("got ack\n");
			} else {
				channel->got_nack = 1;
				mipi_debug("got nack\n");
			}

			up(&channel->open_conn_done_sem);
			break;

		default:
			mipi_err("wrong state : %d, %08x(%d)\n",
				channel->send_step, cmd, channel->channel_id);
			return -1;
		}
		break;

	case HSI_LL_MSG_CONN_CLOSED:
		switch (channel->send_step) {
		case STEP_TX:
		case STEP_WAIT_FOR_CONN_CLOSED:
			mipi_debug("got close\n");
			up(&channel->close_conn_done_sem);
			break;

		default:
			mipi_err("wrong state : %d, %08x(%d)\n",
				channel->send_step, cmd, channel->channel_id);
			return -1;
		}
		break;

	case HSI_LL_MSG_CANCEL_CONN:
		mipi_err("HSI_LL_MSG_CANCEL_CONN\n");

		ret = if_hsi_send_command(mipi_ld, HSI_LL_MSG_ACK,
				HSI_CONTROL_CHANNEL, 0);
		if (ret) {
			mipi_err("if_hsi_send_command fail : %d\n", ret);
			return ret;
		}
		mipi_err("RESET MIPI, SEND ACK\n");
		return -1;

	case HSI_LL_MSG_NOP:
		mipi_info("Receive NOP\n");
		break;

	case HSI_LL_MSG_OPEN_CONN:
	case HSI_LL_MSG_ECHO:
	case HSI_LL_MSG_CONF_RATE:
	default:
		mipi_err("ERROR... CMD Not supported : %08x\n", cmd);
		return -EINVAL;
	}
	return 0;
}

static void if_hsi_handle_control(struct mipi_link_device *mipi_ld,
						struct if_hsi_channel *channel)
{
	struct io_device *iod;
	int ret;
	u32 cmd = 0, ch = 0, param = 0;

	switch (mipi_ld->ld.com_state) {
	case COM_HANDSHAKE:
	case COM_ONLINE:
		mipi_debug("RECV CMD : %08x\n", *(u32 *)channel->rx_data);

		if (channel->rx_count != 4) {
			mipi_err("wrong command len : %d\n", channel->rx_count);
			return;
		}

		ret = if_hsi_decode_cmd(mipi_ld, (u32 *)channel->rx_data,
					&cmd, &ch, &param);
		if (ret)
			mipi_err("decode_cmd fail=%d, cmd=%x\n", ret, cmd);
		else {
			mipi_debug("decode_cmd : %08x\n", cmd);
			ret = if_hsi_rx_cmd_handle(mipi_ld, cmd, ch, param);
			if (ret)
				mipi_debug("handle cmd cmd=%x\n", cmd);
		}

		ret = if_hsi_read_async(mipi_ld, channel->channel_id, 4);
		if (ret)
			mipi_err("hsi_read fail : %d\n", ret);
		return;

	case COM_BOOT:
	case COM_BOOT_EBL:
		mipi_debug("receive data : 0x%x(%d)\n",
				*(u32 *)channel->rx_data, channel->rx_count);

		iod = link_get_iod_with_format(&mipi_ld->ld, IPC_BOOT);
		if (iod) {
			ret = iod->recv(iod, &mipi_ld->ld,
					(char *)channel->rx_data,
					channel->rx_count);
			if (ret < 0)
				mipi_err("recv call fail : %d\n", ret);
		}

		ret = if_hsi_read_async(mipi_ld, channel->channel_id, 4);
		if (ret)
			mipi_err("hsi_read fail : %d\n", ret);
		return;

	case COM_CRASH:
		mipi_debug("receive data : 0x%x(%d)\n",
				*(u32 *)channel->rx_data, channel->rx_count);

		iod = link_get_iod_with_format(&mipi_ld->ld, IPC_RAMDUMP);
		if (iod) {
			channel->packet_size = *(u32 *)channel->rx_data;
			mipi_debug("ramdump packet size : %d\n",
						channel->packet_size);

			ret = iod->recv(iod, &mipi_ld->ld,
					(char *)channel->rx_data + 4,
					channel->packet_size);
			if (ret < 0)
				mipi_err("recv call fail : %d\n", ret);
		}

		ret = if_hsi_read_async(mipi_ld, channel->channel_id,
					DUMP_PACKET_SIZE * 4);
		if (ret)
			mipi_err("hsi_read fail : %d\n", ret);
		return;

	case COM_NONE:
	default:
		mipi_err("receive data in wrong state : 0x%x(%d)\n",
				*(u32 *)channel->rx_data, channel->rx_count);
		return;
	}
}

static int if_hsi_protocol_send(struct mipi_link_device *mipi_ld, int ch,
			u32 *data, unsigned int len)
{
	int ret;
	int retry_count = 0;
	int ack_timeout_cnt = 0;
	struct io_device *iod;
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[ch];

	if (channel->send_step != STEP_IDLE) {
		mipi_err("send step is not IDLE : %d\n",
					channel->send_step);
		return -EBUSY;
	}
	channel->send_step = STEP_SEND_OPEN_CONN;

retry_send:

	ret = if_hsi_send_command(mipi_ld, HSI_LL_MSG_OPEN_CONN_OCTET, ch,
				len);
	if (ret) {
		mipi_err("if_hsi_send_command fail : %d\n", ret);
		channel->send_step = STEP_IDLE;
		return -1;
	}

	channel->send_step = STEP_WAIT_FOR_ACK;

	if (down_timeout(&channel->open_conn_done_sem,
				HSI_ACK_DONE_TIMEOUT) < 0) {
		mipi_err("ch=%d, ack_done timeout\n", channel->channel_id);

		iod = link_get_iod_with_format(&mipi_ld->ld, IPC_FMT);
		if (iod && iod->mc->phone_state == STATE_ONLINE &&
			((mipi_ld->ld.com_state == COM_ONLINE) ||
			(mipi_ld->ld.com_state == COM_HANDSHAKE))) {
			channel->send_step = STEP_SEND_OPEN_CONN;
			ack_timeout_cnt++;
			if (ack_timeout_cnt < 5) {
				mipi_err("check ack again. cnt:%d\n",
						ack_timeout_cnt);
				msleep(20);
				if (down_trylock(
					&channel->open_conn_done_sem)) {
					mipi_err("retry send open\n");
					sema_init(&channel->open_conn_done_sem,
							HSI_SEMAPHORE_COUNT);
					goto retry_send;
				} else {
					mipi_err("got ack after sw-reset\n");
					goto check_nack;
				}
			}

			/* cp force crash to get cp ramdump */
			if (iod->mc->gpio_ap_dump_int)
				iod->mc->ops.modem_force_crash_exit(
							iod->mc);
			else if (iod->mc->bootd) /* cp force reset */
				iod->mc->bootd->modem_state_changed(
					iod->mc->bootd, STATE_CRASH_RESET);
		}

		channel->send_step = STEP_IDLE;
		return -ETIMEDOUT;
	}

check_nack:

	mipi_debug("ch=%d, got ack_done=%d\n", channel->channel_id,
				channel->got_nack);

	if (channel->got_nack && (retry_count < 10)) {
		mipi_info("ch=%d, got nack=%d retry=%d\n", channel->channel_id,
					channel->got_nack, retry_count);
		retry_count++;
		msleep(20);
		goto retry_send;
	}
	retry_count = 0;

	channel->send_step = STEP_TX;

	mod_timer(&mipi_ld->hsi_acwake_down_timer,
			jiffies + HSI_ACWAKE_DOWN_TIMEOUT);
	if_hsi_set_wakeline(mipi_ld, 1);

	ret = if_hsi_write(mipi_ld, channel->channel_id, data, len,
				HSI_TX_MODE_SYNC);
	if (ret < 0) {
		mipi_err("if_hsi_write fail : %d\n", ret);
		channel->send_step = STEP_IDLE;
		return ret;
	}
	mipi_debug("SEND DATA : %08x(%d)\n", *data, len);

	channel->send_step = STEP_WAIT_FOR_CONN_CLOSED;
	if (down_timeout(&channel->close_conn_done_sem,
				HSI_CLOSE_CONN_DONE_TIMEOUT) < 0) {
		mipi_err("ch=%d, close conn timeout\n", channel->channel_id);

		channel->send_step = STEP_IDLE;
		return 0;
	}
	mipi_debug("ch=%d, got close_conn_done\n",
				channel->channel_id);

	channel->send_step = STEP_IDLE;

	mipi_debug("write protocol Done : %d\n", channel->tx_count);
	return channel->tx_count;
}

static void *if_hsi_buffer_alloc(struct mipi_link_device *mipi_ld,
			struct hsi_msg *msg, unsigned int size)
{
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[msg->channel];
	int flags = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;
	void *buff = NULL;

	if (channel->is_dma_ch)
		buff = dma_alloc_coherent(mipi_ld->controller, size,
				&sg_dma_address(msg->sgt.sgl), flags);
	else {
		if ((size >= PAGE_SIZE) && (size & (PAGE_SIZE - 1)))
			buff = (void *)__get_free_pages(flags,
						get_order(size));
		else
			buff = kmalloc(size, flags);
	}

	return buff;
}

static void if_hsi_buffer_free(struct mipi_link_device *mipi_ld,
			struct hsi_msg *msg)
{
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[msg->channel];

	if (channel->is_dma_ch)
		dma_free_coherent(mipi_ld->controller,
				msg->sgt.sgl->length,
				sg_virt(msg->sgt.sgl),
				sg_dma_address(msg->sgt.sgl));
	else {
		if ((msg->sgt.sgl->length >= PAGE_SIZE) &&
			(msg->sgt.sgl->length & (PAGE_SIZE - 1)))
			free_pages((unsigned int)sg_virt(msg->sgt.sgl),
				get_order(msg->sgt.sgl->length));
		else
			kfree(sg_virt(msg->sgt.sgl));
	}
}

static void if_hsi_msg_destruct(struct hsi_msg *msg)
{
	struct mipi_link_device *mipi_ld =
		(struct mipi_link_device *)hsi_client_drvdata(msg->cl);

	if (msg->ttype == HSI_MSG_WRITE)
		/* Free the buff */
		if_hsi_buffer_free(mipi_ld, msg);

	/* Free the msg */
	hsi_free_msg(msg);
}

static void if_hsi_write_done(struct hsi_msg *msg)
{
	unsigned long int flags;
	struct mipi_link_device *mipi_ld =
			(struct mipi_link_device *)hsi_client_drvdata(msg->cl);
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[msg->channel];

	mipi_debug("got write data=0x%x(%d)\n", *(u32 *)channel->tx_data,
				msg->actual_len);

	if ((mipi_ld->ld.com_state == COM_ONLINE) &&
			(channel->channel_id == HSI_CONTROL_CHANNEL))
		mipi_debug("SEND CMD : %08x\n", *(u32 *)sg_virt(msg->sgt.sgl));

	channel->tx_count = msg->actual_len;
	up(&channel->write_done_sem);

	/* Free the TX buff & msg */
	if_hsi_buffer_free(mipi_ld, msg);
	hsi_free_msg(msg);
}

static int if_hsi_write(struct mipi_link_device *mipi_ld, unsigned int ch_num,
			u32 *data, unsigned int size, int tx_mode)
{
	int ret;
	unsigned long int flags = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[ch_num];
	struct hsi_msg *tx_msg = NULL;

	mipi_debug("submit write data : 0x%x(%d)\n", *data, size);

	/* Allocate a new TX msg */
	tx_msg = hsi_alloc_msg(1, flags);
	if (!tx_msg) {
		mipi_err("Out of memory (tx_msg)\n");
		return -ENOMEM;
	}
	mipi_debug("hsi_alloc_msg done\n");

	tx_msg->cl = mipi_ld->client;
	tx_msg->channel = ch_num;
	tx_msg->ttype = HSI_MSG_WRITE;
	tx_msg->context = NULL;
	tx_msg->complete = if_hsi_write_done;
	tx_msg->destructor = if_hsi_msg_destruct;

	/* Allocate data buffer */
	channel->tx_count = ALIGN(size, 4);
	channel->tx_data = if_hsi_buffer_alloc(mipi_ld, tx_msg,
				channel->tx_count);
	if (!channel->tx_data) {
		mipi_err("Out of memory (hsi_msg buffer => size: %d)\n",
				channel->tx_count);
		hsi_free_msg(tx_msg);
		return -ENOMEM;
	}
	sg_set_buf(tx_msg->sgt.sgl, channel->tx_data, channel->tx_count);
	mipi_debug("allocate tx buffer done\n");

	/* Copy Data */
	memcpy(sg_virt(tx_msg->sgt.sgl), data, size);
	mipi_debug("copy tx buffer done\n");

	/* Send the TX HSI msg */
	ret = hsi_async(mipi_ld->client, tx_msg);
	if (ret) {
		mipi_err("ch=%d, hsi_async(write) fail=%d\n",
					channel->channel_id, ret);

		/* Free the TX buff & msg */
		/* if_hsi_buffer_free(mipi_ld, tx_msg);
		hsi_free_msg(tx_msg); */
		return ret;
	}
	mipi_debug("hsi_async done\n");

	if (tx_mode != HSI_TX_MODE_SYNC)
		return channel->tx_count;

	if (down_timeout(&channel->write_done_sem,
				HSI_WRITE_DONE_TIMEOUT) < 0) {
		mipi_err("ch=%d, hsi_write_done timeout : %d\n",
					channel->channel_id, size);

		/* Free the TX buff & msg */
		/* if_hsi_buffer_free(mipi_ld, tx_msg);
		hsi_free_msg(tx_msg); */
		return -ETIMEDOUT;
	}

	return channel->tx_count;
}

static void if_hsi_read_done(struct hsi_msg *msg)
{
	int ret;
	unsigned long int flags;
	struct mipi_link_device *mipi_ld =
			(struct mipi_link_device *)hsi_client_drvdata(msg->cl);
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[msg->channel];
	struct io_device *iod;
	enum dev_format format_type = 0;

	mipi_debug("got read data=0x%x(%d)\n", *(u32 *)channel->rx_data,
				msg->actual_len);

	channel->rx_count = msg->actual_len;
	hsi_free_msg(msg);

	switch (channel->channel_id) {
	case HSI_CONTROL_CHANNEL:
		if_hsi_handle_control(mipi_ld, channel);
		return;
	case HSI_FMT_CHANNEL:
		mipi_debug("iodevice format : IPC_FMT\n");
		format_type = IPC_FMT;
		break;
	case HSI_RAW_CHANNEL:
		mipi_debug("iodevice format : IPC_MULTI_RAW\n");
		format_type = IPC_MULTI_RAW;
		break;
	case HSI_RFS_CHANNEL:
		mipi_debug("iodevice format : IPC_RFS\n");
		format_type = IPC_RFS;
		break;

	case HSI_CMD_CHANNEL:
		mipi_debug("receive command data : 0x%x\n",
					*(u32 *)channel->rx_data);

		ret = if_hsi_send_command(mipi_ld, HSI_LL_MSG_CONN_CLOSED,
					channel->channel_id, 0);
		if (ret)
			mipi_err("send_cmd fail=%d\n", ret);
		return;

	default:
		mipi_err("received nouse channel: %d\n",
					channel->channel_id);
		return;
	}

	iod = link_get_iod_with_format(&mipi_ld->ld, format_type);
	if (iod) {
		mipi_debug("RECV DATA : %08x(%d)-%d\n",
				*(u32 *)channel->rx_data, channel->packet_size,
				iod->format);

		ret = iod->recv(iod, &mipi_ld->ld, (char *)channel->rx_data,
					channel->packet_size);
		if (ret < 0) {
			mipi_err("recv call fail : %d\n", ret);
			mipi_err("discard data: channel=%d, packet_size=%d\n",
				channel->channel_id, channel->packet_size);
		}

		if ((iod->format == IPC_FMT) ||
					(iod->format == IPC_RFS))
			print_hex_dump(KERN_DEBUG,
					iod->format == IPC_FMT ?
					"IPC-RX: " : "RFS-RX: ",
					DUMP_PREFIX_NONE,
					1, 1,
					channel->rx_data,
					channel->packet_size <= 16 ?
					(size_t)channel->packet_size :
					(size_t)16, false);

		channel->packet_size = 0;
		channel->recv_step = STEP_IDLE;

		ret = if_hsi_send_command(mipi_ld, HSI_LL_MSG_CONN_CLOSED,
					channel->channel_id, 0);
		if (ret)
			mipi_err("send_cmd fail=%d\n", ret);
	}
}

static int if_hsi_read_async(struct mipi_link_device *mipi_ld,
			unsigned int ch_num, unsigned int size)
{
	int ret;
	unsigned long int flags = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;
	struct if_hsi_channel *channel = &mipi_ld->hsi_channels[ch_num];
	struct hsi_msg *rx_msg = NULL;

	mipi_debug("submit read data : (%d)\n", size);

	/* Allocate a new RX msg */
	rx_msg = hsi_alloc_msg(1, flags);
	if (!rx_msg) {
		mipi_err("Out of memory (rx_msg)\n");
		return -ENOMEM;
	}
	mipi_debug("hsi_alloc_msg done\n");

	rx_msg->cl = mipi_ld->client;
	rx_msg->channel = ch_num;
	rx_msg->ttype = HSI_MSG_READ;
	rx_msg->context = NULL;
	rx_msg->complete = if_hsi_read_done;
	rx_msg->destructor = if_hsi_msg_destruct;

	sg_set_buf(rx_msg->sgt.sgl, channel->rx_data, size);
	mipi_debug("allocate rx buffer done\n");

	/* Send the RX HSI msg */
	ret = hsi_async(mipi_ld->client, rx_msg);
	if (ret) {
		mipi_err("ch=%d, hsi_async(read) fail=%d\n",
					channel->channel_id, ret);
		/* Free the RX msg */
		hsi_free_msg(rx_msg);
		return ret;
	}
	mipi_debug("hsi_async done\n");

	return 0;
}

static void if_hsi_port_event(struct hsi_client *cl, unsigned long event)
{
	struct mipi_link_device *mipi_ld =
			(struct mipi_link_device *)hsi_client_drvdata(cl);

	switch (event) {
	case HSI_EVENT_START_RX:
		mipi_info("CAWAKE(1)\n");
		return;

	case HSI_EVENT_STOP_RX:
		mipi_info("CAWAKE(0)\n");
		return;

	default:
		mipi_err("Unknown Event : %ld\n", event);
		return;
	}
}

static int __devinit if_hsi_probe(struct device *dev)
{
	struct mipi_link_device *mipi_ld =
			(struct mipi_link_device *)if_hsi_driver.priv_data;

	/* The parent of our device is the HSI port,
	 * the parent of the HSI port is the HSI controller device */
	struct device *controller = dev->parent->parent;
	struct hsi_client *client = to_hsi_client(dev);

	/* Save the controller & client */
	mipi_ld->controller = controller;
	mipi_ld->client = client;
	mipi_ld->is_dma_capable = is_device_dma_capable(controller);

	/* Warn if no DMA capability */
	if (!mipi_ld->is_dma_capable)
		mipi_info("HSI device is not DMA capable !\n");

	/* Save private Data */
	hsi_client_set_drvdata(client, (void *)mipi_ld);

	mipi_debug("if_hsi_probe() Done\n");
	return 0;
}

static void if_hsi_shutdown(struct device *dev)
{
	struct mipi_link_device *mipi_ld =
			(struct mipi_link_device *)if_hsi_driver.priv_data;

	if (hsi_port_claimed(mipi_ld->client)) {
		mipi_info("Releasing the HSI controller\n");
		hsi_release_port(mipi_ld->client);
	}

	mipi_info("if_hsi_shutdown() Done\n");
}

static inline void if_hsi_free_buffer(struct mipi_link_device *mipi_ld)
{
	int i;
	struct if_hsi_channel *channel;

	for (i = 0; i < HSI_NUM_OF_USE_CHANNELS; i++) {
		channel = &mipi_ld->hsi_channels[i];
		if (channel->rx_data != NULL) {
			kfree(channel->rx_data);
			channel->rx_data = NULL;
		}
	}
}

static inline void if_hsi_destroy_workqueue(struct mipi_link_device *mipi_ld)
{
	int i;
	struct if_hsi_channel *channel;

	for (i = 0; i < HSI_NUM_OF_USE_CHANNELS; i++) {
		channel = &mipi_ld->hsi_channels[i];
		if (channel->tx_wq) {
			destroy_workqueue(channel->tx_wq);
			channel->tx_wq = NULL;
		}
	}
}

static int if_hsi_init(struct link_device *ld)
{
	struct mipi_link_device *mipi_ld = to_mipi_link_device(ld);
	struct if_hsi_channel *channel;
	int i, ret;

	for (i = 0; i < HSI_NUM_OF_USE_CHANNELS; i++) {
		mipi_debug("make ch %d\n", i);
		channel = &mipi_ld->hsi_channels[i];
		channel->channel_id = i;

		channel->use_credits = ch_info[i].use_credits;
		atomic_set(&channel->credits, 0);
		channel->is_dma_ch = ch_info[i].is_dma_ch;

		/* create tx workqueue */
		if (ch_info[i].wq_priority == WQ_HIGHPRI)
			channel->tx_wq = create_hi_prio_wq(ch_info[i].name);
		else
			channel->tx_wq = create_norm_prio_wq(ch_info[i].name);
		if (!channel->tx_wq) {
			ret = -ENOMEM;
			mipi_err("fail to make wq\n");
			goto destory_wq;
		}
		mipi_debug("tx_wq : %p\n", channel->tx_wq);

		/* allocate rx / tx data buffer */
		channel->tx_pdu_size = ch_info[i].tx_pdu_sz;
		channel->rx_pdu_size = ch_info[i].rx_pdu_sz;
		if (!channel->rx_pdu_size)
			goto variable_init;

		channel->rx_data = kmalloc(channel->rx_pdu_size,
							GFP_DMA | GFP_ATOMIC);
		if (!channel->rx_data) {
			mipi_err("alloc ch %d rx_data fail\n", i);
			ret = -ENOMEM;
			goto free_buffer;
		}

variable_init:
		mipi_debug("rx_buf = %p\n", channel->rx_data);

		atomic_set(&channel->opened, 0);
		channel->send_step = STEP_UNDEF;
		channel->recv_step = STEP_UNDEF;
		sema_init(&channel->write_done_sem, HSI_SEMAPHORE_COUNT);
		sema_init(&channel->open_conn_done_sem, HSI_SEMAPHORE_COUNT);
		sema_init(&channel->close_conn_done_sem, HSI_SEMAPHORE_COUNT);

		skb_queue_head_init(&channel->tx_q);

		if (i != HSI_CONTROL_CHANNEL)
			/* raw dma tx composition can be varied with fmt */
			INIT_WORK(&channel->tx_w, mipi_hsi_send_work);

		/* save referece to link device */
		channel->ld = ld;
	}

	/* init delayed work for IPC handshaking */
	INIT_DELAYED_WORK(&mipi_ld->start_work, if_hsi_start_work);

	/* init timer to stop tx if there's no more transit */
	setup_timer(&mipi_ld->hsi_acwake_down_timer, if_hsi_acwake_down_func,
				(unsigned long)mipi_ld);
	atomic_set(&mipi_ld->acwake, 0);
	spin_lock_init(&mipi_ld->acwake_lock);

	/* Register HSI Client driver */
	if_hsi_driver.priv_data = (void *)mipi_ld;
	ret = hsi_register_client_driver(&if_hsi_driver);
	if (ret) {
		mipi_err("hsi_register_client_driver() fail : %d\n", ret);
		goto free_buffer;
	}
	mipi_debug("hsi_register_client_driver() Done: %d\n", ret);

	/* To make Bulk TX for RAW Data */
	mipi_ld->bulk_tx_buf = kmalloc(MIPI_BULK_TX_SIZE, GFP_DMA | GFP_ATOMIC);
	if (!mipi_ld->bulk_tx_buf) {
		mipi_err("alloc bulk tx buffer fail\n");
		ret = -ENOMEM;
		goto free_buffer;
	}
	skb_queue_head_init(&mipi_ld->bulk_txq);

	return 0;

free_buffer:
	if_hsi_free_buffer(mipi_ld);
destory_wq:
	if_hsi_destroy_workqueue(mipi_ld);
	return ret;
}

struct link_device *mipi_create_link_device(struct platform_device *pdev)
{
	int ret;
	struct mipi_link_device *mipi_ld;
	struct link_device *ld;

	mipi_ld = kzalloc(sizeof(struct mipi_link_device), GFP_KERNEL);
	if (!mipi_ld)
		return NULL;

	wake_lock_init(&mipi_ld->wlock, WAKE_LOCK_SUSPEND, "mipi_link");

	ld = &mipi_ld->ld;

	ld->name = "mipi_hsi";
	ld->init_comm = mipi_hsi_init_communication;
	ld->terminate_comm = mipi_hsi_terminate_communication;
	ld->ioctl = mipi_hsi_ioctl;
	ld->send = mipi_hsi_send;
	ld->com_state = COM_NONE;

	ret = if_hsi_init(ld);
	if (ret)
		return NULL;

	return ld;
}

