/*
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

#ifndef __MODEM_LINK_DEVICE_MIPI_H__
#define __MODEM_LINK_DEVICE_MIPI_H__

#define HSI_SEMAPHORE_COUNT	0

#define HSI_CHANNEL_TX_STATE_UNAVAIL	(1 << 0)
#define HSI_CHANNEL_TX_STATE_WRITING	(1 << 1)
#define HSI_CHANNEL_RX_STATE_UNAVAIL	(1 << 0)
#define HSI_CHANNEL_RX_STATE_READING	(1 << 1)

#define HSI_WRITE_DONE_TIMEOUT	(HZ)
#define HSI_READ_DONE_TIMEOUT	(HZ)
#define HSI_ACK_DONE_TIMEOUT	(HZ / 2)
#define HSI_CLOSE_CONN_DONE_TIMEOUT	(HZ / 2)
#define HSI_ACWAKE_DOWN_TIMEOUT	(HZ)

#define HSI_TX_MODE_SYNC	0
#define HSI_TX_MODE_ASYNC	1

#define HSI_CONTROL_CHANNEL	0
#define HSI_FLASHLESS_CHANNEL	0
#define HSI_CP_RAMDUMP_CHANNEL	0
#define HSI_FMT_CHANNEL	1
#define HSI_RAW_CHANNEL	2
#define HSI_RFS_CHANNEL	3
#define HSI_CMD_CHANNEL	4
#define HSI_NUM_OF_USE_CHANNELS	5

#define HSI_RX_0_CHAN_BUF_SIZE	(32 * 1024)
#define HSI_RX_1_CHAN_BUF_SIZE	(8 * 1024)
#define HSI_RX_2_CHAN_BUF_SIZE	(256 * 1024)
#define HSI_RX_3_CHAN_BUF_SIZE	(256 * 1024)
#define HSI_RX_4_CHAN_BUF_SIZE	(8 * 1024)

#define DUMP_PACKET_SIZE	4097 /* (16K + 4 ) / 4 length, word unit */
#define DUMP_ERR_INFO_SIZE	39 /* 150 bytes + 4 length , word unit */

#define HSI_LL_INVALID_CHANNEL	0xFF
#define HSI_LL_MSG_NOP_CHECK_PATTERN	0xE7E7EC
#define MIPI_BULK_TX_SIZE	(8 * 1024)

#define create_hi_prio_wq(name) \
	alloc_workqueue("hsi_%s_h_wq", \
			WQ_HIGHPRI | WQ_UNBOUND | WQ_RESCUER, 1, (name))
#define create_norm_prio_wq(name) \
	alloc_workqueue("hsi_%s_wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1, (name))

enum {
	HSI_LL_MSG_BREAK, /* 0x0 */
	HSI_LL_MSG_ECHO,
	HSI_LL_MSG_INFO_REQ,
	HSI_LL_MSG_INFO,
	HSI_LL_MSG_NOP,
	HSI_LL_MSG_ALLOCATE_CH,
	HSI_LL_MSG_RELEASE_CH,
	HSI_LL_MSG_OPEN_CONN,
	HSI_LL_MSG_CONN_READY,
	HSI_LL_MSG_CONN_CLOSED, /* 0x9 */
	HSI_LL_MSG_CANCEL_CONN,
	HSI_LL_MSG_ACK, /* 0xB */
	HSI_LL_MSG_NAK, /* 0xC */
	HSI_LL_MSG_CONF_RATE,
	HSI_LL_MSG_OPEN_CONN_OCTET, /* 0xE */
	HSI_LL_MSG_INVALID = 0xFF,
};

enum {
	STEP_UNDEF,
	STEP_CLOSED,
	STEP_NOT_READY,
	STEP_IDLE,
	STEP_ERROR,
	STEP_SEND_OPEN_CONN,
	STEP_SEND_ACK,
	STEP_WAIT_FOR_ACK,
	STEP_TO_ACK,
	STEP_SEND_NACK,
	STEP_GET_NACK,
	STEP_SEND_CONN_READY,
	STEP_WAIT_FOR_CONN_READY,
	STEP_SEND_CONF_RATE,
	STEP_WAIT_FOR_CONF_ACK,
	STEP_TX,
	STEP_RX,
	STEP_SEND_CONN_CLOSED,
	STEP_WAIT_FOR_CONN_CLOSED,
	STEP_SEND_BREAK,
	STEP_SEND_TO_CONN_CLOSED,
};

struct hsi_channel_info {
	char *name;
	int wq_priority;
	int rx_pdu_sz;
	int tx_pdu_sz;
	int use_credits;
	int is_dma_ch;
};

struct if_hsi_channel {
	unsigned int channel_id;
	unsigned int use_credits;
	atomic_t credits;
	int is_dma_ch;

	void *tx_data;
	int tx_pdu_size;
	unsigned int tx_count;
	void *rx_data;
	int rx_pdu_size;
	unsigned int rx_count;
	unsigned int packet_size;

	unsigned int send_step;
	unsigned int recv_step;

	unsigned int got_nack;

	struct semaphore write_done_sem;
	struct semaphore open_conn_done_sem;
	struct semaphore close_conn_done_sem;

	atomic_t opened;

	struct workqueue_struct *tx_wq;
	struct sk_buff_head tx_q;
	struct work_struct tx_w;

	/* reference to link device */
	struct link_device *ld;
};

struct mipi_link_device {
	struct link_device ld;

	/* mipi specific link data */
	struct if_hsi_channel hsi_channels[HSI_MAX_CHANNELS];

	struct delayed_work start_work;

	struct wake_lock wlock;
	struct timer_list hsi_acwake_down_timer;

	/* maybe -list of io devices for the link device to use
	 * to find where to send incoming packets to */
	struct list_head list_of_io_devices;

	void *bulk_tx_buf;
	struct sk_buff_head bulk_txq;

	/* hsi control data */
	unsigned int is_dma_capable;
	struct hsi_client *client;
	struct device *controller;

	atomic_t acwake;
	spinlock_t acwake_lock;
};
/* converts from struct link_device* to struct xxx_link_device* */
#define to_mipi_link_device(linkdev) \
			container_of(linkdev, struct mipi_link_device, ld)


enum {
	HSI_INIT_MODE_NORMAL,
	HSI_INIT_MODE_FLASHLESS_BOOT,
	HSI_INIT_MODE_CP_RAMDUMP,
	HSI_INIT_MODE_FLASHLESS_BOOT_EBL,
};

/*
 * HSI transfer complete callback prototype
 */
typedef void (*xfer_complete_cb) (struct hsi_msg *pdu);

static int if_hsi_set_wakeline(struct mipi_link_device *mipi_ld,
			unsigned int state);
static int if_hsi_init_handshake(struct mipi_link_device *mipi_ld, int mode);
static u32 if_hsi_create_cmd(u32 cmd_type, int ch, void *arg);
static int if_hsi_protocol_send(struct mipi_link_device *mipi_ld, int ch,
			u32 *data, unsigned int len);
static int if_hsi_write(struct mipi_link_device *mipi_ld, unsigned int ch_num,
			u32 *data, unsigned int size, int tx_mode);
static void if_hsi_read_done(struct hsi_msg *msg);
static int if_hsi_read_async(struct mipi_link_device *mipi_ld,
			unsigned int ch_num, unsigned int size);
static void if_hsi_port_event(struct hsi_client *cl, unsigned long event);

#define MIPI_LOG_TAG "mipi_link: "

#define mipi_err(fmt, ...) \
	pr_err(MIPI_LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mipi_debug(fmt, ...) \
	pr_debug(MIPI_LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mipi_info(fmt, ...) \
	pr_info(MIPI_LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)

#endif
