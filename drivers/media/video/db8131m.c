/* drivers/media/video/db8131m.c
 *
 * Copyright (c) 2010, Samsung Electronics. All rights reserved
 * Author: dongseong.lim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
#include "db8131m.h"

#define MAX_FMTS 1

struct device *cam_dev_front;

#define DB8131M_CAPTURE_WIDTH_MAX 1280
#define DB8131M_CAPTURE_HEIGHT_MAX 960


static int db8131m_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);

static const struct db8131m_fps db8131m_framerates[] = {
	{ I_FPS_7,	FRAME_RATE_7 },
	{ I_FPS_15,	FRAME_RATE_15 },
};

static const struct db8131m_framesize db8131m_preview_frmsizes[] = {
	{ DB8131M_PREVIEW_QCIF,	176,  144 },
	{ DB8131M_PREVIEW_QVGA,	320,  240 },
	{ DB8131M_PREVIEW_CIF,		352,  288 },
	{ DB8131M_PREVIEW_VGA,		640,  480 },
};

static const struct db8131m_framesize db8131m_capture_frmsizes[] = {
	{ DB8131M_CAPTURE_VGA,		640,  480 },
	{ DB8131M_CAPTURE_0_7MP,		960,  720 },
	{ DB8131M_CAPTURE_1_3MP,		1280, 960 },
};

static struct db8131m_control db8131m_ctrls[] = {
	DB8131M_INIT_CONTROL(V4L2_CID_CAMERA_BRIGHTNESS, \
					EV_DEFAULT),
};

static const struct db8131m_regs reg_datas = {
	.ev = {
#ifdef CONFIG_LOAD_FILE
		[EV_MINUS_4] = {.name = "db8131m_bright_m4"},
		[EV_MINUS_3] = {.name = "db8131m_bright_m3"},
		[EV_MINUS_2] = {.name = "db8131m_bright_m2"},
		[EV_MINUS_1] = {.name = "db8131m_bright_m1"},
		[EV_DEFAULT] = {.name = "db8131m_bright_default"},
		[EV_PLUS_1] = {.name = "db8131m_bright_p1"},
		[EV_PLUS_2] = {.name = "db8131m_bright_p2"},
		[EV_PLUS_3] = {.name = "db8131m_bright_p3"},
		[EV_PLUS_4] = {.name = "db8131m_bright_p4"},
#else
		[EV_MINUS_4] = {.reg = db8131m_bright_m4,
			.array_size = ARRAY_SIZE(db8131m_bright_m4)},
		[EV_MINUS_3] = {.reg = db8131m_bright_m3,
			.array_size = ARRAY_SIZE(db8131m_bright_m3)},
		[EV_MINUS_2] = {.reg = db8131m_bright_m2,
			.array_size = ARRAY_SIZE(db8131m_bright_m2)},
		[EV_MINUS_1] = {.reg = db8131m_bright_m1,
			.array_size = ARRAY_SIZE(db8131m_bright_m1)},
		[EV_DEFAULT] = {.reg = db8131m_bright_default,
			.array_size = ARRAY_SIZE(db8131m_bright_default)},
		[EV_PLUS_1] = {.reg = db8131m_bright_p1,
			.array_size = ARRAY_SIZE(db8131m_bright_p1)},
		[EV_PLUS_2] = {.reg = db8131m_bright_p2,
			.array_size = ARRAY_SIZE(db8131m_bright_p2)},
		[EV_PLUS_3] = {.reg = db8131m_bright_p3,
			.array_size = ARRAY_SIZE(db8131m_bright_p3)},
		[EV_PLUS_4] = {.reg = db8131m_bright_p4,
			.array_size = ARRAY_SIZE(db8131m_bright_p4)},
#endif
	},
	.preview_size = {
#ifdef CONFIG_LOAD_FILE
		[DB8131M_PREVIEW_QCIF] = {.name = "db8131m_size_qcif"},
		[DB8131M_PREVIEW_QVGA] = {.name = "db8131m_size_qvga"},
		[DB8131M_PREVIEW_CIF] = {.name = "db8131m_size_cif"},
		[DB8131M_PREVIEW_VGA] = {.name = "db8131m_size_vga"},
#else
		[DB8131M_PREVIEW_QCIF] = {.reg = db8131m_size_qcif,
			.array_size = ARRAY_SIZE(db8131m_size_qcif)},
		[DB8131M_PREVIEW_QVGA] = {.reg = db8131m_size_qvga,
			.array_size = ARRAY_SIZE(db8131m_size_qvga)},
		[DB8131M_PREVIEW_CIF] = {.reg = db8131m_size_cif,
			.array_size = ARRAY_SIZE(db8131m_size_cif)},
		[DB8131M_PREVIEW_VGA] = {.reg = db8131m_size_vga,
			.array_size = ARRAY_SIZE(db8131m_size_vga)},
#endif
	},
#ifdef CONFIG_LOAD_FILE
	.capture_start = {
		[DB8131M_CAPTURE_VGA] = {.name = "db8131m_vga_capture"},
		[DB8131M_CAPTURE_0_7MP] = {.name = "db8131m_capture_960x720"},
		[DB8131M_CAPTURE_1_3MP] = {.name = "db8131m_capture_1280x960"},
	},
	.fps = {
		[I_FPS_7] = {.name = "db8131m_vt_7fps"},
		[I_FPS_15] = {.name = "db8131m_vt_15fps"},
	},
	.init_reg = {.name = "db8131m_common"},
	.vt_init_reg = {.name = "db8131m_vt_common"},
	.record = {.name = "db8131m_recording_50Hz_common"},
	.capture_on = {.name = "db8131m_capture"},
	.preview_on = {.name = "db8131m_preview"},
	.stream_off = {.name = "db8131m_stream_stop"},
#else
	.capture_start = {
		[DB8131M_CAPTURE_VGA] = {.reg = db8131m_vga_capture,
			.array_size = ARRAY_SIZE(db8131m_vga_capture)},
		[DB8131M_CAPTURE_0_7MP] = {.reg = db8131m_capture_960x720,
			.array_size = ARRAY_SIZE(db8131m_capture_960x720)},
		[DB8131M_CAPTURE_1_3MP] = {.reg = db8131m_capture_1280x960,
			.array_size = ARRAY_SIZE(db8131m_capture_1280x960)},
	},
	.fps = {
		[I_FPS_7] = {.reg = db8131m_vt_7fps,
			.array_size = ARRAY_SIZE(db8131m_vt_7fps)},
		[I_FPS_15] = {.reg = db8131m_vt_15fps,
			.array_size = ARRAY_SIZE(db8131m_vt_15fps)},
	},
	.init_reg = {.reg = db8131m_common,
			.array_size = ARRAY_SIZE(db8131m_common)},
	.vt_init_reg = {.reg = db8131m_vt_common,
			.array_size = ARRAY_SIZE(db8131m_vt_common)},
	.record = {.reg = db8131m_recording_50Hz_common,
			.array_size =
				ARRAY_SIZE(db8131m_recording_50Hz_common)},
	.capture_on = {.reg = db8131m_capture,
			.array_size = ARRAY_SIZE(db8131m_capture)},
	.preview_on = {.reg = db8131m_preview,
			.array_size = ARRAY_SIZE(db8131m_preview)},
	.stream_off = {.reg = db8131m_stream_stop,
			.array_size = ARRAY_SIZE(db8131m_stream_stop)},
#endif
	.effect = {
#ifdef CONFIG_LOAD_FILE
		[V4L2_COLORFX_NONE] = {.name = "db8131m_Effect_Normal"},
		[V4L2_COLORFX_BW] = {.name = "db8131m_Effect_Black_White"},
		[V4L2_COLORFX_SEPIA] = {.name = "db8131m_Effect_Sepia"},
		[V4L2_COLORFX_NEGATIVE] = {.name = "db8131m_Effect_Negative"},
#else
		[V4L2_COLORFX_NONE] = {.reg = db8131m_Effect_Normal,
					.array_size =
					ARRAY_SIZE(db8131m_Effect_Normal)},
		[V4L2_COLORFX_BW] = {.reg = db8131m_Effect_Black_White,
					.array_size =
					ARRAY_SIZE(db8131m_Effect_Black_White)},
		[V4L2_COLORFX_SEPIA] = {.reg = db8131m_Effect_Sepia,
					.array_size =
					ARRAY_SIZE(db8131m_Effect_Sepia)},
		[V4L2_COLORFX_NEGATIVE] = {.reg = db8131m_Effect_Negative,
					.array_size =
					ARRAY_SIZE(db8131m_Effect_Negative)},
#endif
	},
	.wb = {
#ifdef CONFIG_LOAD_FILE
		[V4L2_WHITE_BALANCE_AUTO] = {.name = "db8131m_WB_Auto"},
		[V4L2_WHITE_BALANCE_DAYLIGHT] = {.name = "db8131m_WB_Daylight"},
		[V4L2_WHITE_BALANCE_CLOUDY] = {.name = "db8131m_WB_Cloudy"},
		[V4L2_WHITE_BALANCE_INCANDESCENT] = {.name =
			"db8131m_WB_Incandescent"},
		[V4L2_WHITE_BALANCE_FLUORESCENT] = {.name =
			"db8131m_WB_Fluorescent"},
#else
		[V4L2_WHITE_BALANCE_AUTO] = {.reg = db8131m_WB_Auto,
					.array_size =
					ARRAY_SIZE(db8131m_WB_Auto)},
		[V4L2_WHITE_BALANCE_DAYLIGHT] = {.reg = db8131m_WB_Daylight,
					.array_size =
					ARRAY_SIZE(db8131m_WB_Daylight)},
		[V4L2_WHITE_BALANCE_CLOUDY] = {.reg = db8131m_WB_Cloudy,
					.array_size =
					ARRAY_SIZE(db8131m_WB_Cloudy)},
		[V4L2_WHITE_BALANCE_INCANDESCENT] = {
					.reg =
					db8131m_WB_Incandescent,
					.array_size =
					ARRAY_SIZE(db8131m_WB_Incandescent)},
		[V4L2_WHITE_BALANCE_FLUORESCENT] = {
					.reg =
					db8131m_WB_Fluorescent,
					.array_size =
					ARRAY_SIZE(db8131m_WB_Fluorescent)},
#endif
	},
	.antibanding = {
#ifdef CONFIG_LOAD_FILE
		[V4L2_CID_POWER_LINE_FREQUENCY_50HZ] = {.name = "db8131m_50Hz"},
		[V4L2_CID_POWER_LINE_FREQUENCY_60HZ] = {.name = "db8131m_60Hz"},
#else
		[V4L2_CID_POWER_LINE_FREQUENCY_50HZ] = {.reg = db8131m_50Hz,
					.array_size =
					ARRAY_SIZE(db8131m_50Hz)},
		[V4L2_CID_POWER_LINE_FREQUENCY_60HZ] = {.reg = db8131m_60Hz,
					.array_size =
					ARRAY_SIZE(db8131m_60Hz)},
#endif
	},
};

static const struct v4l2_mbus_framefmt capture_fmts[] = {
	{
		.code		= V4L2_MBUS_FMT_FIXED,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
};

#ifdef CONFIG_LOAD_FILE
static int db8131m_regs_file_size;
static int loadFile(void)
{
	struct file *fp = NULL;
	struct test *nextBuf = NULL;

	u8 *nBuf = NULL;
	size_t file_size = 0, max_size = 0, testBuf_size = 0;
	ssize_t nread = 0;
	s32 check = 0, starCheck = 0;
	s32 tmp_large_file = 0;
	s32 i = 0;
	int ret = 0;
	loff_t pos;

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	BUG_ON(testBuf);

	fp = filp_open(TUNING_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		cam_err("file open error\n");
		return PTR_ERR(fp);
	}

	file_size = (size_t) fp->f_path.dentry->d_inode->i_size;
	max_size = file_size;
	db8131m_regs_file_size = file_size;

	cam_dbg("file_size = %d\n", file_size);

	nBuf = kmalloc(file_size, GFP_ATOMIC);
	if (nBuf == NULL) {
		cam_dbg("Fail to 1st get memory\n");
		nBuf = vmalloc(file_size);
		if (nBuf == NULL) {
			cam_err("ERR: nBuf Out of Memory\n");
			ret = -ENOMEM;
			goto error_out;
		}
		tmp_large_file = 1;
	}

	testBuf_size = sizeof(struct test) * file_size;
	if (tmp_large_file) {
		testBuf = (struct test *)vmalloc(testBuf_size);
		large_file = 1;
	} else {
		testBuf = kmalloc(testBuf_size, GFP_ATOMIC);
		if (testBuf == NULL) {
			cam_dbg("Fail to get mem(%d bytes)\n", testBuf_size);
			testBuf = (struct test *)vmalloc(testBuf_size);
			large_file = 1;
		}
	}
	if (testBuf == NULL) {
		cam_err("ERR: Out of Memory\n");
		ret = -ENOMEM;
		goto error_out;
	}

	pos = 0;
	memset(nBuf, 0, file_size);
	memset(testBuf, 0, file_size * sizeof(struct test));

	nread = vfs_read(fp, (char __user *)nBuf, file_size, &pos);
	if (nread != file_size) {
		cam_err("failed to read file ret = %d\n", nread);
		ret = -1;
		goto error_out;
	}

	set_fs(fs);

	i = max_size;

	printk("i = %d\n", i);

	while (i) {
		testBuf[max_size - i].data = *nBuf;
		if (i != 1) {
			testBuf[max_size - i].nextBuf =
			    &testBuf[max_size - i + 1];
		} else {
			testBuf[max_size - i].nextBuf = NULL;
			break;
		}
		i--;
		nBuf++;
	}

	i = max_size;
	nextBuf = &testBuf[0];

#if 1
	while (i - 1) {
		if (!check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size - i].nextBuf != NULL) {
					if (testBuf[max_size - i].nextBuf->data
					    == '/') {
						check = 1; /* when find '//' */
						i--;
					} else if (testBuf[max_size - i].
						   nextBuf->data == '*') {
						starCheck = 1; /* when find '/ *' */
						i--;
					}
				} else
					break;
			}
			if (!check && !starCheck) {
				/* ignore '\t' */
				if (testBuf[max_size - i].data != '\t') {
					nextBuf->nextBuf =
					    &testBuf[max_size - i];
					nextBuf = &testBuf[max_size - i];
				}
			}
		} else if (check && !starCheck) {
			if (testBuf[max_size - i].data == '/') {
				if (testBuf[max_size - i].nextBuf != NULL) {
					if (testBuf[max_size - i].nextBuf->
					    data == '*') {
						starCheck = 1; /* when find '/ *' */
						check = 0;
						i--;
					}
				} else
					break;
			}

			/* when find '\n' */
			if (testBuf[max_size - i].data == '\n' && check) {
				check = 0;
				nextBuf->nextBuf = &testBuf[max_size - i];
				nextBuf = &testBuf[max_size - i];
			}

		} else if (!check && starCheck) {
			if (testBuf[max_size - i].data == '*') {
				if (testBuf[max_size - i].nextBuf != NULL) {
					if (testBuf[max_size - i].nextBuf->
					    data == '/') {
						starCheck = 0; /* when find '* /' */
						i--;
					}
				} else
					break;
			}
		}

		i--;

		if (i < 2) {
			nextBuf = NULL;
			break;
		}

		if (testBuf[max_size - i].nextBuf == NULL) {
			nextBuf = NULL;
			break;
		}
	}
#endif

#if 0
	printk("i = %d\n", i);
	nextBuf = &testBuf[0];
	while (1) {
		printk("sdfdsf\n");
		if (nextBuf->nextBuf == NULL)
			break;
		printk("%c", nextBuf->data);
		nextBuf = nextBuf->nextBuf;
	}
#endif

 error_out:

	if (nBuf)
		tmp_large_file ? vfree(nBuf) : kfree(nBuf);
	if (fp)
		filp_close(fp, current->files);
	return ret;
}
#endif

/**
 * db8131m_i2c_read: Read 2 bytes from sensor
 */
static int db8131m_i2c_read(struct i2c_client *client,
				  u8 subaddr, u8 *data)
{
	int err;
	u8 buf[1];
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = (u8 *)&subaddr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	err = i2c_transfer(client->adapter, msg, 2);
	CHECK_ERR_COND_MSG(err != 2, -EIO, "fail to read register\n");

	*data = buf[0];

	return 0;
}

/**
 * db8131m_i2c_write: Write (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 *
 * Returns 0 on success, <0 on error
 */
static int db8131m_i2c_write(struct i2c_client *client,
					 u8 addr, u8 w_data)
{
	int retry_count = 5;
	int ret = 0;
	u8 buf[2] = {0,};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 2,
		.buf	= buf,
	};

	buf[0] = addr;
	buf[1] = w_data & 0xff;

	db8131m_debug(DB8131M_DEBUG_I2C, "%s : W(0x%02X%02X)\n",
		__func__, buf[0], buf[1]);

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep(POLL_TIME_MS);
		cam_err("%s: ERROR(%d), write (%02X, %02X), retry %d.\n",
				__func__, ret, addr, w_data, retry_count);
	} while (retry_count-- > 0);

	CHECK_ERR_COND_MSG(ret != 1, -EIO, "I2C does not working.\n\n");

	return 0;
}

#ifdef CONFIG_LOAD_FILE
static int db8131m_write_regs_from_sd(struct v4l2_subdev *sd,
						const u8 s_name[])
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct test *tempData = NULL;

	int ret = -EAGAIN;
	regs_short_t temp;
	u16 delay = 0;
	u8 addr[6];
	u8 value[6];
	s32 searched = 0, pair_cnt = 0, brace_cnt = 0;
	size_t size = strlen(s_name);
	s32 i;


	cam_trace("E size = %d, string = %s\n", size, s_name);
	printk("[JHS] E size = %d, string = %s\n", size, s_name);
	tempData = &testBuf[0];
	while (!searched) {
		searched = 1;
		for (i = 0; i < size; i++) {
			if (tempData->data != s_name[i]) {
				searched = 0;
				break;
			}
			tempData = tempData->nextBuf;
		}
		tempData = tempData->nextBuf;
	}
	/* structure is get.. */
	while (1) {
		if (tempData->data == '{') {
			dbg_setfile("%s: found big_brace start\n", __func__);
			tempData = tempData->nextBuf;
			break;
		} else
			tempData = tempData->nextBuf;
	}

	while(1) {

		searched = 0;
		pair_cnt = 0;
		while (1) {
			if (tempData->data == 'x') {
				/* get 10 strings. */
				addr[0] = '0';
				for (i = 1; i < 4; i++) {
					addr[i] = tempData->data;
					tempData = tempData->nextBuf;
				}
				addr[i] = '\0';

				value[0] = '0';
				value[1] = 'x';
				for (i = 2; i < 4; i++) {
					value[i] = tempData->data;
					tempData = tempData->nextBuf;
				}
				value[i] = '\0';
				/* dbg_setfile("read HEX: %s\n", data); */
				temp.subaddr =
					simple_strtoul(addr, NULL, 16);
				temp.value =
					simple_strtoul(value, NULL, 16);
				break;
			} else if (tempData->data == '}') {
				/* dbg_setfile("%s: found big_brace end\n", __func__); */
				tempData = tempData->nextBuf;
				searched = 1; 
				break;
			} else
				tempData = tempData->nextBuf;

			if (tempData->nextBuf == NULL)
				return -1;
		}

		if (searched)
			break;

		if ((temp.subaddr & 0xFF) == 0xE7) {
			delay = temp.value & 0x00FF;
			debug_msleep(sd, delay);
			continue;
		}

		/* cam_err("Write: 0x%02X, 0x%02X\n",
		   (u8)(temp.subaddr), (u8)(temp.value)); */
		ret = db8131m_i2c_write(client, temp.subaddr, temp.value);

		/* In error circumstances */
		/* Give second shot */
		if (unlikely(ret)) {
			dev_info(&client->dev,
					"db8131m i2c retry one more time\n");
			ret = db8131m_i2c_write(client, temp.subaddr,
					temp.value);

			/* Give it one more shot */
			if (unlikely(ret)) {
				dev_info(&client->dev,
						"db8131m i2c retry twice\n");
				ret = db8131m_i2c_write(client, temp.subaddr,
						temp.value);
			}
		}
	}

	return ret;

}
#endif

/* Write register
 * If success, return value: 0
 * If fail, return value: -EIO
 */
static int db8131m_write_regs(struct v4l2_subdev *sd, const u16 regs[],
			     int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u16 delay = 0;
	int i, err = 0;

	for (i = 0; i < size; i++) {
		if ((regs[i] & 0xFF00) == DB8131M_DELAY) {
			delay = regs[i] & 0x00FF;
			debug_msleep(sd, delay);
			continue;
		}

		err = db8131m_i2c_write(client,
			(regs[i] >> 8), regs[i]);
		CHECK_ERR_MSG(err, "write registers\n")
	}

	return 0;
}

/* PX: */
static int db8131m_set_from_table(struct v4l2_subdev *sd,
				const char *setting_name,
				const struct db8131m_regset_table *table,
				u32 table_size, s32 index)
{
	int err = 0;

	 cam_dbg("%s: set %s index %d\n",
		__func__, setting_name, index);
	CHECK_ERR_COND_MSG(((index < 0) || (index >= table_size)),
		-EINVAL, "index(%d) out of range[0:%d] for table for %s\n",
		index, table_size, setting_name);

	table += index;

#ifdef CONFIG_LOAD_FILE
	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
	return db8131m_write_regs_from_sd(sd, table->name);

#else /* CONFIG_LOAD_FILE */

# ifdef DEBUG_WRITE_REGS
	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
# endif /* DEBUG_WRITE_REGS */
	
	CHECK_ERR_COND_MSG(!table->reg, -EFAULT, \
		"table=%s, index=%d, reg = NULL\n", setting_name, index);

	err = db8131m_write_regs(sd, table->reg, table->array_size);
	CHECK_ERR_MSG(err, "write regs(%s), err=%d\n", setting_name, err);

	return 0;
#endif /* CONFIG_LOAD_FILE */
}

/* PX: */
static inline int db8131m_save_ctrl(struct v4l2_subdev *sd,
					struct v4l2_control *ctrl)
{
	int ctrl_cnt = ARRAY_SIZE(db8131m_ctrls);
	int i;

	for (i = 0; i < ctrl_cnt; i++) {
		if (ctrl->id == db8131m_ctrls[i].id) {
			db8131m_ctrls[i].value = ctrl->value;
			break;
		}
	}

	if (unlikely(i >= ctrl_cnt))
		cam_trace("WARNING, not saved ctrl-ID=0x%X\n", ctrl->id);

	return 0;
}

/* PX: Set brightness */
static int db8131m_set_exposure(struct v4l2_subdev *sd, s32 val)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	val = val + EV_DEFAULT;

	if ((val < EV_MINUS_4) || (val > EV_PLUS_4)) {
		cam_err("%s: ERROR, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}
	
	err = db8131m_set_from_table(sd, "brightness", state->regs->ev,
		ARRAY_SIZE(state->regs->ev), val);

	return err;
}

/* Set whitebalance */
static int db8131m_set_wb(struct v4l2_subdev *sd, s32 val)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	err = db8131m_set_from_table(sd, "wb", state->regs->wb,
		ARRAY_SIZE(state->regs->wb), val);

	return err;
}

/* Set whitebalance */
static int db8131m_set_effect(struct v4l2_subdev *sd, s32 val)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	err = db8131m_set_from_table(sd, "effect", state->regs->effect,
		ARRAY_SIZE(state->regs->effect), val);

	return err;
}

static int db8131m_set_capture_size(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);

	/* Don't forget the below codes.
	 * We set here state->preview to NULL after reconfiguring
	 * capure config if capture ratio does't match with preview ratio.
	 */
	state->preview = NULL;

	return 0;
}

/* PX: Set framerate */
static int db8131m_set_frame_rate(struct v4l2_subdev *sd, s32 fps)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EIO;
	int i = 0, fps_index = -1;

	cam_info("set frame rate %d\n", fps);

	for (i = 0; i < ARRAY_SIZE(db8131m_framerates); i++) {
		if (fps == db8131m_framerates[i].fps) {
			fps_index = db8131m_framerates[i].index;
			state->fps = fps;
			state->req_fps = -1;
			break;
		}
	}

	if (unlikely(fps_index < 0)) {
		cam_err("%s: WARNING, Not supported FPS(%d)\n", __func__, fps);
		return 0;
	}

	err = db8131m_set_from_table(sd, "fps", state->regs->fps,
			ARRAY_SIZE(state->regs->fps), fps_index);
	CHECK_ERR_MSG(err, "fail to set framerate\n")

	return 0;
}


static int db8131m_init_regs(struct v4l2_subdev *sd)
{
	/*struct i2c_client *client = v4l2_get_subdevdata(sd);*/
	struct db8131m_state *state = to_state(sd);
	/*u16 read_value = 0;*/
	int i;

	/* we'd prefer to do this in probe, but the framework hasn't
	 * turned on the camera yet so our i2c operations would fail
	 * if we tried to do it in probe, so we have to do it here
	 * and keep track if we succeeded or not.
	 */

	state->preview = state->capture = NULL;
	state->sensor_mode = SENSOR_CAMERA;
	state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	state->fps = 0;
	state->req_fps = -1;
	state->vt_mode = 0;
	state->antibanding_mode = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;

	for (i = 0; i < ARRAY_SIZE(db8131m_ctrls); i++)
		db8131m_ctrls[i].value = db8131m_ctrls[i].default_value;

	/* enter read mode */

	state->regs = &reg_datas;

	return 0;
}

static const struct db8131m_framesize *db8131m_get_framesize
	(const struct db8131m_framesize *frmsizes,
	u32 frmsize_count, u32 index)
{
	int i = 0;

	for (i = 0; i < frmsize_count; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

/* This function is called from the g_ctrl api
 *
 * This function should be called only after the s_fmt call,
 * which sets the required width/height value.
 *
 * It checks a list of available frame sizes and sets the
 * most appropriate frame size.
 *
 * The list is stored in an increasing order (as far as possible).
 * Hence the first entry (searching from the beginning) where both the
 * width and height is more than the required value is returned.
 * In case of no perfect match, we set the last entry (which is supposed
 * to be the largest resolution supported.)
 */
static void db8131m_set_framesize(struct v4l2_subdev *sd,
				const struct db8131m_framesize *frmsizes,
				u32 num_frmsize, bool preview)
{
	struct db8131m_state *state = to_state(sd);
	const struct db8131m_framesize **found_frmsize = NULL;
	u32 width = state->req_fmt.width;
	u32 height = state->req_fmt.height;
	int i = 0;
	int found = 0;

	cam_dbg("%s: Requested Res %dx%d\n", __func__,
			width, height);

	found_frmsize = (const struct db8131m_framesize **)
			(preview ? &state->preview : &state->capture);

	for (i = 0; i < num_frmsize; i++) {
		if ((frmsizes[i].width == width) &&
			(frmsizes[i].height == height)) {
			*found_frmsize = &frmsizes[i];
			found = 1;
			break;
		}
	}

	if (*found_frmsize == NULL || found == 0) {
		cam_err("%s: ERROR, invalid frame size %dx%d\n", __func__,
						width, height);
		*found_frmsize = preview ?
			db8131m_get_framesize(frmsizes, num_frmsize,
					DB8131M_PREVIEW_VGA) :
			db8131m_get_framesize(frmsizes, num_frmsize,
					DB8131M_CAPTURE_1_3MP);
		BUG_ON(!(*found_frmsize));
	}

	if (preview)
		cam_info("Preview Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
	else
		cam_info("Capture Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
}

static int db8131m_control_stream(struct v4l2_subdev *sd, u32 cmd)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	if (unlikely(cmd != STREAM_STOP))
		return 0;

	cam_info("STREAM STOP!!\n");
	err = db8131m_set_from_table(sd, "stream_off",
			&state->regs->stream_off, 1, 0);

	/*debug_msleep(sd, state->pdata->streamoff_delay);*/

	if (state->runmode == DB8131M_RUNMODE_CAPTURING) {
		state->runmode = DB8131M_RUNMODE_CAPTURE_STOP;
		cam_dbg("Capture Stop!\n");
	}

	CHECK_ERR_MSG(err, "failed to stop stream\n");
	return 0;
}

/* PX: */
static int db8131m_get_exif(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char lsb, msb;
	unsigned short coarse_time, line_length, shutter_speed, exposureValue, iso;

	db8131m_i2c_write(client, 0xFF, 0xD0);
	db8131m_i2c_read(client, 0x06, &msb);
	db8131m_i2c_read(client, 0x07, &lsb);
	coarse_time = (msb << 8) + lsb ;

	db8131m_i2c_read(client, 0x0A, &msb);
	db8131m_i2c_read(client, 0x0B, &lsb);
	line_length = (msb << 8) + lsb ;

	shutter_speed = 24000000 / (coarse_time * line_length);
	exposureValue = (coarse_time * line_length) / 2400 ;

	if (exposureValue >= 1000)
		iso = 400;
	else if (exposureValue >= 500)
		iso = 200;
	else if (exposureValue >= 333)
		iso = 100;
	else
		iso = 50;

	state->exif.exp_time_den = exposureValue;
	state->exif.iso = iso;

	cam_dbg("EXIF: ex_time_den=%d, iso=%d\n",
		state->exif.exp_time_den, state->exif.iso);

	return 0;
}

static int db8131m_set_preview_start(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;
	/* bool set_size = true; */

	cam_dbg("Camera Preview start, runmode = %d\n", state->runmode);
	cam_dbg("preview_start, vt_mode = %d\n", state->vt_mode);

	if ((state->runmode == DB8131M_RUNMODE_NOTREADY) ||
	    (state->runmode == DB8131M_RUNMODE_CAPTURING)) {
		cam_err("%s: ERROR - Invalid runmode\n", __func__);
		return -EPERM;
	}

	if ((state->req_fmt.width == 176 && state->req_fmt.height == 144) ||
		(state->req_fmt.width == 352 && state->req_fmt.height == 288))
		db8131m_set_frame_rate(sd, 15);

	err = db8131m_set_from_table(sd, "preview_size",
			state->regs->preview_size,
			ARRAY_SIZE(state->regs->preview_size),
			state->preview->index);

	if(state->antibanding_mode == V4L2_CID_POWER_LINE_FREQUENCY_50HZ ||
			state->antibanding_mode == V4L2_CID_POWER_LINE_FREQUENCY_60HZ) {
		err = db8131m_set_from_table(sd, "antibanding",
				state->regs->antibanding,
				ARRAY_SIZE(state->regs->antibanding),
				state->antibanding_mode);
	}

	err |= db8131m_set_from_table(sd, "preview_on",
				&state->regs->preview_on, 1, 0);
	if (err < 0) {
		printk(KERN_ERR "%s, start preview fail!!!!", __func__);
		return -EIO;
	}

	state->runmode = DB8131M_RUNMODE_RUNNING;

	return 0;
}

static int db8131m_set_video_preview(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("Video Preview start, runmode = %d\n", state->runmode);

	if ((state->runmode == DB8131M_RUNMODE_NOTREADY) ||
	    (state->runmode == DB8131M_RUNMODE_CAPTURING)) {
		cam_err("%s: ERROR - Invalid runmode\n", __func__);
		return -EPERM;
	}

	if ((state->req_fmt.width == 176 && state->req_fmt.height == 144) ||
		(state->req_fmt.width == 352 && state->req_fmt.height == 288))
		db8131m_set_frame_rate(sd, 15);

	err = db8131m_set_from_table(sd, "record",
			&state->regs->record, 1, 0);
	CHECK_ERR_MSG(err, "failed to set record\n");

	err = db8131m_set_from_table(sd, "preview_size",
			state->regs->preview_size,
			ARRAY_SIZE(state->regs->preview_size),
			state->preview->index);
	CHECK_ERR_MSG(err, "failed to set preview size\n");
	
	if(state->antibanding_mode == V4L2_CID_POWER_LINE_FREQUENCY_50HZ ||
			state->antibanding_mode == V4L2_CID_POWER_LINE_FREQUENCY_60HZ) {
		err = db8131m_set_from_table(sd, "antibanding",
				state->regs->antibanding,
				ARRAY_SIZE(state->regs->antibanding),
				state->antibanding_mode);
	}

	err = db8131m_set_from_table(sd, "preview_on",
			&state->regs->preview_on, 1, 0);
	CHECK_ERR_MSG(err, "failed to set preview\n");

	cam_dbg("runmode now RUNNING\n");
	state->runmode = DB8131M_RUNMODE_RUNNING;

	return 0;
}

/* PX: Start capture */
static int db8131m_set_capture_start(struct v4l2_subdev *sd)
{
	struct db8131m_state *state = to_state(sd);
	int err = -ENODEV;

	/* Set capture size */
	err = db8131m_set_capture_size(sd);

	/* Send capture start command. */
	cam_dbg("Send Capture_Start cmd\n");
	
	err = db8131m_set_from_table(sd, "capture_start",
			state->regs->capture_start,
			ARRAY_SIZE(state->regs->capture_start),
			state->capture->index);
	CHECK_ERR_MSG(err, "fail to capture_start (%d)\n", err);

	state->runmode = DB8131M_RUNMODE_CAPTURING;

	db8131m_get_exif(sd);

	return 0;
}


static int db8131m_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct db8131m_state *state = to_state(sd);

/*	if (!fmt)
		return -EINVAL;*/

	cam_dbg("%s: pixelformat = 0x%x, colorspace = 0x%x, width = %d, height = %d\n",
		__func__, fmt->code, fmt->colorspace, fmt->width, fmt->height);
/*
	if ((fmt->width > S5k5CCGX_CAPTURE_WIDTH_MAX) ||
			(fmt->height > S5k5CCGX_CAPTURE_HEIGHT_MAX)) {
		fmt->width = S5k5CCGX_CAPTURE_WIDTH_MAX;
		fmt->height = S5k5CCGX_CAPTURE_HEIGHT_MAX;
	}*/

	v4l2_fill_pix_format(&state->req_fmt, fmt);

	return -EINVAL/*0*/;
}

static int db8131m_s_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct db8131m_state *state = to_state(sd);
	s32 previous_index = 0;

	cam_dbg("%s: pixelformat = 0x%x, colorspace = 0x%x, width = %d, height = %d\n",
		__func__, fmt->code, fmt->colorspace, fmt->width, fmt->height);

	db8131m_try_mbus_fmt(sd, fmt);

	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		previous_index = state->preview ? state->preview->index : -1;

		db8131m_set_framesize(sd, db8131m_preview_frmsizes,
				ARRAY_SIZE(db8131m_preview_frmsizes),
				true);

		if (state->preview == NULL)
			return -EINVAL;

		cam_info("init: mode state->sensor_mode %d, state->preview->index %d\n",
			state->sensor_mode, state->preview->index);
	} else {
		/*
		 * In case of image capture mode,
		 * if the given image resolution is not supported,
		 * use the next higher image resolution. */
		db8131m_set_framesize(sd, db8131m_capture_frmsizes,
				ARRAY_SIZE(db8131m_capture_frmsizes),
				false);
	}

	return 0;
}

static int db8131m_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
					enum v4l2_mbus_pixelcode *code)
{
	cam_dbg("%s: index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(capture_fmts))
		return -EINVAL;

	*code = capture_fmts[index].code;

	return 0;
}

static int db8131m_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	struct db8131m_state *state = to_state(sd);

	/*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		if (unlikely(state->preview == NULL)) {
			cam_err("%s: ERROR\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview->width;
		fsize->discrete.height = state->preview->height;
	} else {
		if (unlikely(state->capture == NULL)) {
			cam_err("%s: ERROR\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture->width;
		fsize->discrete.height = state->capture->height;
	}


	return 0;
}

static int db8131m_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	return 0;
}

static int db8131m_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct db8131m_state *state = to_state(sd);
	u32 run_mode;

	run_mode = param->parm.capture.capturemode;

	cam_dbg("%s: run_mode = %d\n", __func__, run_mode);

	switch (run_mode) {
	case CI_MODE_STILL_CAPTURE:
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;
		state->sensor_mode = SENSOR_CAMERA;
		break;
	case CI_MODE_VIDEO:
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
		state->sensor_mode = SENSOR_MOVIE;
		break;
	case CI_MODE_PREVIEW:
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
		state->sensor_mode = SENSOR_CAMERA;
		break;
	default:
		cam_err("%s: ERROR, Not support.(%d)\n", __func__, run_mode);
		break;
	}

	return 0;
}

static int db8131m_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct db8131m_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized) {
		cam_err("%s: WARNING, camera not initialized\n", __func__);
		return 0;
	}

	mutex_lock(&state->ctrl_lock);
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE_ABSOLUTE:
		ctrl->value = state->exif.exp_time_den;
		break;
	case V4L2_CID_ISO_SENSITIVITY:
		ctrl->value = state->exif.iso;
		break;
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
	case V4L2_CID_FOCAL_ABSOLUTE:
	case V4L2_CID_FNUMBER_ABSOLUTE:
	case V4L2_CID_FNUMBER_RANGE:
	case V4L2_CID_POWER_LINE_FREQUENCY:
	default:
		break;
	}
	mutex_unlock(&state->ctrl_lock);

	return err;
}


static int db8131m_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct db8131m_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized) {
		cam_err("%s: WARNING, camera not initialized\n", __func__);
		return 0;
	}

	cam_dbg("%s: ctrl->id %d\n",
		__func__, ctrl->id);

	mutex_lock(&state->ctrl_lock);
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		err = db8131m_set_exposure(sd, ctrl->value);
		break;
	case V4L2_CID_SCENE_MODE:
	case V4L2_CID_ISO_SENSITIVITY:
	case V4L2_CID_COLORFX:
		err = db8131m_set_effect(sd, ctrl->value);
		break;
	case V4L2_CID_EXPOSURE_METERING:
	case V4L2_CID_CAMERA_EFFECT:
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		err = db8131m_set_wb(sd, ctrl->value);
		break;
	case V4L2_CID_CAMERA_VT_MODE:
		state->vt_mode = ctrl->value;
		cam_dbg("db8131m_s_ctrl, vt_mode = %d\n", state->vt_mode);
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		state->antibanding_mode = ctrl->value;

		break;
	//case V4L2_CID_CAMERA_WHITE_BALANCE:
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
	case V4L2_CID_FOCAL_ABSOLUTE:
	case V4L2_CID_FNUMBER_ABSOLUTE:
	case V4L2_CID_FNUMBER_RANGE:
	default:
		break;
	}
	mutex_unlock(&state->ctrl_lock);

	return err;
}

static int db8131m_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct db8131m_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d\n", enable);
	BUG_ON(!state->initialized);

	if (enable) {
		switch (state->sensor_mode) {
		case SENSOR_CAMERA:
			if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
				err = db8131m_set_capture_start(sd);
			else
				err = db8131m_set_preview_start(sd);
			break;
		case SENSOR_MOVIE:
			err = db8131m_set_video_preview(sd);
			break;
		default:
			break;
		}
	} else {
		if (state->pdata->is_mipi)
			err = db8131m_control_stream(sd, STREAM_STOP);
	}

	return 0;
}

#define to_db8131m_sensor(sd) container_of(sd, struct db8131m_state, sd)

static int db8131m_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_fh *fh,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= MAX_FMTS)
		return -EINVAL;
	code->code = V4L2_MBUS_FMT_SGRBG10_1X10;

	return 0;
}

static int db8131m_enum_frame_size(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh,
	struct v4l2_subdev_frame_size_enum *fse)
{
	unsigned int index = fse->index;

	if (index >= DB8131M_PREVIEW_MAX)
		return -EINVAL;

	fse->min_width = db8131m_preview_frmsizes[index].width;
	fse->min_height = db8131m_preview_frmsizes[index].height;
	fse->max_width = db8131m_preview_frmsizes[index].width;
	fse->max_height = db8131m_preview_frmsizes[index].height;

	return 0;
}


static struct v4l2_mbus_framefmt *
__db8131m_get_pad_format(struct db8131m_state *sensor,
			 struct v4l2_subdev_fh *fh, unsigned int pad,
			 enum v4l2_subdev_format_whence which)
{
	struct i2c_client *client = v4l2_get_subdevdata(&sensor->sd);

	if (pad != 0) {
		dev_err(&client->dev,  "%s err. pad %x\n", __func__, pad);
		return NULL;
	}

	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_format(fh, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &sensor->format;
	default:
		return NULL;
	}
}

static int
db8131m_get_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct db8131m_state *snr = to_db8131m_sensor(sd);
	struct v4l2_mbus_framefmt *format =
		__db8131m_get_pad_format(snr, fh, fmt->pad, fmt->which);

	if (format == NULL)
		return -EINVAL;
	fmt->format = *format;

	return 0;
}

static int
db8131m_set_pad_format(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
		       struct v4l2_subdev_format *fmt)
{
	struct db8131m_state *snr = to_db8131m_sensor(sd);
	struct v4l2_mbus_framefmt *format =
		__db8131m_get_pad_format(snr, fh, fmt->pad, fmt->which);

	if (format == NULL)
		return -EINVAL;

	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		snr->format = fmt->format;

	return 0;
}


static int db8131m_init(struct v4l2_subdev *sd, u32 val)
{
	struct db8131m_state *state = to_state(sd);
	int err = -EINVAL;

	cam_dbg("%s: start\n", __func__);

	err = db8131m_init_regs(sd);
	CHECK_ERR_MSG(err, "failed to indentify sensor chip\n");

	err = db8131m_set_from_table(sd, "init_reg",
		&state->regs->init_reg, 1, 0);
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");
/*
	err = db8131m_set_from_table(sd, "stream_off",
		&state->regs->stream_off, 1, 0);
	CHECK_ERR_MSG(err, "failed to stream off\n");
*/

	state->runmode = DB8131M_RUNMODE_INIT;

	state->initialized = 1;

	return 0;
}

static int db8131m_s_power(struct v4l2_subdev *sd, int on)
{
	struct db8131m_state *state = to_state(sd);
	int ret = 0;

	pr_info("db8131m power value %d\n", on);

	if (on == 0)
		return state->pdata->power_ctrl(sd, on);
	else {
		if (state->pdata->power_ctrl(sd, on))
			return -EINVAL;
		return db8131m_init(sd, on);
	}

	return ret;
}


/*
 * s_config subdev ops
 * With camera device, we need to re-initialize
 * every single opening time therefor,
 * it is not necessary to be initialized on probe time.
 * except for version checking
 * NOTE: version checking is optional
 */
static int db8131m_s_config(struct v4l2_subdev *sd,
			int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;
	struct db8131m_state *state = to_state(sd);
#ifdef CONFIG_LOAD_FILE
	int err = 0;
#endif

	if (!platform_data) {
		cam_err("%s: ERROR, no platform data\n", __func__);
		return -ENODEV;
	}
	state->pdata = platform_data;
	state->dbg_level = &state->pdata->dbg_level;

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	state->req_fmt.width = state->pdata->default_width;

	state->req_fmt.height = state->pdata->default_height;


	if (!state->pdata->pixelformat)
		state->req_fmt.pixelformat = DEFAULT_PIX_FMT;
	else
		state->req_fmt.pixelformat = state->pdata->pixelformat;

	if (!state->pdata->freq)
		state->freq = DEFAULT_MCLK;	/* 24MHz default */
	else
		state->freq = state->pdata->freq;

	if (state->pdata->platform_init) {
		ret = state->pdata->platform_init(client);
		if (ret) {
			v4l2_err(client, "db8131m platform init err\n");
			return ret;
		}
	}

	ret = state->pdata->csi_cfg(sd, 1);
	if (ret)
		return ret;

#ifdef CONFIG_LOAD_FILE
	err = loadFile();
	if (unlikely(err < 0)) {
		cam_err("failed to load file ERR=%d\n", err);
		return err;
	}
#endif

	return 0;
}

static int db8131m_enum_frameintervals(struct v4l2_subdev *sd,
				       struct v4l2_frmivalenum *fival)
{
	struct db8131m_state *state = to_state(sd);

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = state->req_fps;

	return 0;
}

static int db8131m_get_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct db8131m_state *state = to_state(sd);

	if (!fmt)
		return -EINVAL;

	fmt->width = state->req_fmt.width;
	fmt->height = state->req_fmt.height;
	fmt->code = V4L2_MBUS_FMT_SGRBG10_1X10;

	return 0;
}

static int db8131m_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
	*frames = 0;

	return 0;
}

static long db8131m_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	switch (cmd) {
	case ATOMISP_IOC_S_EXPOSURE:
		return 0;
	case ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA:
		return 0;
	default:
		return -EINVAL;
	}
	return 0;
}


static const struct v4l2_subdev_core_ops db8131m_core_ops = {
	.g_ctrl = db8131m_g_ctrl,
	.s_ctrl = db8131m_s_ctrl,
	.s_power = db8131m_s_power,
	.ioctl = db8131m_ioctl,
};

/* REVISIT: Do we need pad operations? */
static const struct v4l2_subdev_pad_ops db8131m_pad_ops = {
	.enum_mbus_code = db8131m_enum_mbus_code,
	.enum_frame_size = db8131m_enum_frame_size,
};

static const struct v4l2_subdev_video_ops db8131m_video_ops = {
	.try_mbus_fmt = db8131m_try_mbus_fmt,
	.s_mbus_fmt = db8131m_s_mbus_fmt,
	.g_mbus_fmt = db8131m_get_mbus_fmt,
	.g_parm = db8131m_g_parm,
	.s_parm = db8131m_s_parm,
	.s_stream = db8131m_s_stream,
	.enum_framesizes = db8131m_enum_framesizes,
	.enum_frameintervals = db8131m_enum_frameintervals,
	.enum_mbus_fmt = db8131m_enum_mbus_fmt,
};

static struct v4l2_subdev_sensor_ops db8131m_sensor_ops = {
	.g_skip_frames	= db8131m_g_skip_frames,
};

static const struct v4l2_subdev_ops db8131m_ops = {
	.core = &db8131m_core_ops,
	.video = &db8131m_video_ops,
	.pad = &db8131m_pad_ops,
	.sensor = &db8131m_sensor_ops,
};

static const struct media_entity_operations db8131m_entity_ops = {
	.link_setup = NULL,
};

static int db8131m_remove(struct i2c_client *client);
/*
 * db8131m_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static ssize_t front_camera_fw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "DB8131A_DB8131A\n");
}

static ssize_t front_camera_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char cam_type[] = "DB_DB8131A_NONE\n";

	return sprintf(buf, "%s", cam_type);
}

static struct device_attribute dev_attr_camtype_front =
__ATTR(front_camtype, S_IRUGO, front_camera_type_show, NULL);

static struct device_attribute dev_attr_camfw_front =
__ATTR(front_camfw, S_IRUGO, front_camera_fw_show, NULL);

static int __devinit db8131m_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct db8131m_state *state;
	int err = -EINVAL;

	state = kzalloc(sizeof(struct db8131m_state), GFP_KERNEL);
	if (unlikely(!state)) {
		dev_err(&client->dev, "probe, fail to get memory\n");
		goto err_out;
	}

	mutex_init(&state->ctrl_lock);

	state->runmode = DB8131M_RUNMODE_NOTREADY;
	sd = &state->sd;
	strcpy(sd->name, DB8131M_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &db8131m_ops);

	cam_dev_front =  device_create(camera_class, NULL,
			MKDEV(CAM_MAJOR, 1), NULL, "front");

	if (IS_ERR(cam_dev_front)) {
		dev_err(&client->dev, "Failed to create deivce (cam_dev_front)\n");
		goto SKIP_SYSFS;
	}

	if (device_create_file(cam_dev_front, &dev_attr_camtype_front) < 0) {
		dev_err(&client->dev, "Failed to create device file: %s\n",
				dev_attr_camtype_front.attr.name);
		goto SKIP_SYSFS;
	}

	if (device_create_file(cam_dev_front, &dev_attr_camfw_front) < 0) {
		dev_err(&client->dev, "Failed to create device file: %s\n",
				dev_attr_camfw_front.attr.name);
		goto SKIP_SYSFS;
	}

SKIP_SYSFS:

	err = db8131m_s_config(sd, 0, client->dev.platform_data);
	CHECK_ERR_MSG(err, "fail to s_config\n");

	printk(KERN_DEBUG "%s %s: driver probed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));


	/*TODO add format code here*/
	state->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	state->pad.flags = MEDIA_PAD_FL_SOURCE;

	/* REVISIT: Do we need media controller? */
	err = media_entity_init(&state->sd.entity, 1, &state->pad, 0);
	if (err)
		return db8131m_remove(client);

	return 0;

err_out:
	kfree(state);
	return -ENOMEM;
}

static int db8131m_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct db8131m_state *state = to_state(sd);

	destroy_workqueue(state->workqueue);

	if (state->pdata->platform_deinit)
		state->pdata->platform_deinit();
	state->pdata->csi_cfg(sd, 0);

	v4l2_device_unregister_subdev(sd);
	mutex_destroy(&state->ctrl_lock);
	kfree(state);

#ifdef CONFIG_LOAD_FILE
	large_file ? vfree(testBuf) : kfree(testBuf);
	large_file = 0;
	testBuf = NULL;
#endif

	printk(KERN_DEBUG "%s %s: driver removed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));
	return 0;
}

static const struct i2c_device_id db8131m_id[] = {
	{ DB8131M_DRIVER_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, db8131m_id);

static struct i2c_driver v4l2_i2c_driver = {
	.driver.name	= DB8131M_DRIVER_NAME,
	.probe		= db8131m_probe,
	.remove		= db8131m_remove,
	.id_table	= db8131m_id,
};

static int __init v4l2_i2c_drv_init(void)
{
	pr_info("%s: %s called\n", __func__, DB8131M_DRIVER_NAME);
	return i2c_add_driver(&v4l2_i2c_driver);
}

static void __exit v4l2_i2c_drv_cleanup(void)
{
	pr_info("%s: %s called\n", __func__, DB8131M_DRIVER_NAME);
	i2c_del_driver(&v4l2_i2c_driver);
}

module_init(v4l2_i2c_drv_init);
module_exit(v4l2_i2c_drv_cleanup);

MODULE_DESCRIPTION("Dongbu DB8131M 1.3MP SOC camera driver");
MODULE_AUTHOR("Dong-Seong Lim <dongseong.lim@samsung.com>");
MODULE_LICENSE("GPL");
