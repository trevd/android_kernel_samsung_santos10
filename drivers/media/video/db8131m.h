/* drivers/media/video/db8131m.h
 *
 * Driver for db8131m (1.3MP Camera) from Dongbu
 *
 * Copyright (C) 2010, SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __DB8131M_H__
#define __DB8131M_H__
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/db8131m_platform.h>
#include <linux/atomisp.h>
#include <linux/videodev2_sec_camera.h>
#include <linux/workqueue.h>
#include <linux/atomisp_platform.h>

#define DB8131M_DRIVER_NAME	"db8131m"

#define DB8131M_DELAY		0xE700

#define CAM_MAJOR       119
extern struct class *camera_class;

/************************************
 * FEATURE DEFINITIONS
 ************************************/
#define FEATURE_YUV_CAPTURE
/* #define CONFIG_LOAD_FILE */ /* for tuning */

/** Debuging Feature **/
#define CONFIG_CAM_DEBUG
/* #define CONFIG_CAM_TRACE*/ /* Enable it with CONFIG_CAM_DEBUG */
/* #define CONFIG_CAM_AF_DEBUG *//* Enable it with CONFIG_CAM_DEBUG */
/* #define DEBUG_WRITE_REGS */
/***********************************/

#ifdef CONFIG_VIDEO_DB8131M_DEBUG
enum {
	DB8131M_DEBUG_I2C		= 1U << 0,
	DB8131M_DEBUG_I2C_BURSTS	= 1U << 1,
};
static uint32_t db8131m_debug_mask = DB8131M_DEBUG_I2C_BURSTS;
module_param_named(debug_mask, db8131m_debug_mask, uint, S_IWUSR | S_IRUGO);

#define db8131m_debug(mask, x...) \
	do { \
		if (db8131m_debug_mask & mask) \
			pr_info(x);	\
	} while (0)
#else
#define db8131m_debug(mask, x...)
#endif

#define TAG_NAME	"["DB8131M_DRIVER_NAME"]"" "
#define cam_err(fmt, ...)	\
	printk(KERN_ERR TAG_NAME fmt, ##__VA_ARGS__)
#define cam_warn(fmt, ...)	\
	printk(KERN_WARNING TAG_NAME fmt, ##__VA_ARGS__)
#define cam_info(fmt, ...)	\
	printk(KERN_INFO TAG_NAME fmt, ##__VA_ARGS__)

#if defined(CONFIG_CAM_DEBUG)
#define cam_dbg(fmt, ...)	\
	printk(KERN_DEBUG TAG_NAME fmt, ##__VA_ARGS__)
#else
#define cam_dbg(fmt, ...)	\
	do { \
		if (*to_state(sd)->dbg_level & CAMDBG_LEVEL_DEBUG) \
			printk(KERN_DEBUG TAG_NAME fmt, ##__VA_ARGS__); \
	} while (0)
#endif

#if defined(CONFIG_CAM_DEBUG) && defined(CONFIG_CAM_TRACE)
#define cam_trace(fmt, ...)	cam_dbg("%s: " fmt, __func__, ##__VA_ARGS__);
#else
#define cam_trace(fmt, ...)	\
	do { \
		if (*to_state(sd)->dbg_level & CAMDBG_LEVEL_TRACE) \
			printk(KERN_DEBUG TAG_NAME "%s: " fmt, \
				__func__, ##__VA_ARGS__); \
	} while (0)
#endif

#if defined(CONFIG_CAM_DEBUG) && defined(CONFIG_CAM_AF_DEBUG)
#define af_dbg(fmt, ...)	cam_dbg(fmt, ##__VA_ARGS__);
#else
#define af_dbg(fmt, ...)
#endif

#define CHECK_ERR_COND(condition, ret)	\
	do { if (unlikely(condition)) return ret; } while (0)
#define CHECK_ERR_COND_MSG(condition, ret, fmt, ...) \
	{if (unlikely(condition)) { \
		cam_err("%s: ERROR, " fmt, __func__, ##__VA_ARGS__); \
		return ret; \
	} }

#define CHECK_ERR(x)	CHECK_ERR_COND(((x) < 0), (x))
#define CHECK_ERR_MSG(x, fmt, ...) \
	CHECK_ERR_COND_MSG(((x) < 0), (x), fmt, ##__VA_ARGS__)


#ifdef CONFIG_LOAD_FILE
#define DB8131M_BURST_WRITE_REGS(sd, A) ({ \
	int ret; \
		cam_info("BURST_WRITE_REGS: reg_name=%s from setfile\n", #A); \
		ret = db8131m_write_regs_from_sd(sd, #A); \
		ret; \
	})
#else
#define DB8131M_BURST_WRITE_REGS(sd, A) \
	db8131m_burst_write_regs(sd, A, (sizeof(A) / sizeof(A[0])), #A)
#endif

enum db8131m_oprmode {
	DB8131M_OPRMODE_VIDEO = 0,
	DB8131M_OPRMODE_IMAGE = 1,
};

enum stream_cmd {
	STREAM_STOP,
	STREAM_START,
};

enum wide_req_cmd {
	WIDE_REQ_NONE,
	WIDE_REQ_CHANGE,
	WIDE_REQ_RESTORE,
};

enum db8131m_preview_frame_size {
	DB8131M_PREVIEW_QCIF = 0,	/* 176x144 */
	DB8131M_PREVIEW_QVGA,	/* 320x240 */
	DB8131M_PREVIEW_CIF,		/* 352x288 */
	DB8131M_PREVIEW_VGA,		/* 640x480 */

	DB8131M_PREVIEW_MAX,
};

/* Capture Size List: Capture size is defined as below.
 *
 *	DB8131M_CAPTURE_VGA:		640x480
 *	DB8131M_CAPTURE_1_3MP:		1280x960
 */

enum db8131m_capture_frame_size {
	DB8131M_CAPTURE_VGA = 0,	/* 640x480 */
	DB8131M_CAPTURE_0_7MP, 		/* 960x720 */
	DB8131M_CAPTURE_1_3MP,			/* 1280x960 */

	DB8131M_CAPTURE_MAX,
};

enum db8131m_fps_index {
	I_FPS_0,
	I_FPS_7,
	I_FPS_15,
	I_FPS_MAX,
};

#define PREVIEW_WIDE_SIZE	DB8131M_PREVIEW_1024x576

struct db8131m_control {
	u32 id;
	s32 value;
	s32 default_value;
};

#define DB8131M_INIT_CONTROL(ctrl_id, default_val) \
	{					\
		.id = ctrl_id,			\
		.value = default_val,		\
		.default_value = default_val,	\
	}

struct db8131m_framesize {
	s32 index;
	u32 width;
	u32 height;
};

#define FRM_RATIO(framesize) \
	(((framesize)->width) * 10 / ((framesize)->height))

struct db8131m_interval {
	struct timeval curr_time;
	struct timeval before_time;
};

#define GET_ELAPSED_TIME(cur, before) \
		(((cur).tv_sec - (before).tv_sec) * USEC_PER_SEC \
		+ ((cur).tv_usec - (before).tv_usec))

struct db8131m_fps {
	u32 index;
	u32 fps;
};

struct db8131m_version {
	u32 major;
	u32 minor;
};

enum db8131m_runmode {
	DB8131M_RUNMODE_NOTREADY,
	DB8131M_RUNMODE_INIT,
	/*DB8131M_RUNMODE_IDLE,*/
	DB8131M_RUNMODE_RUNNING, /* previewing */
	DB8131M_RUNMODE_RUNNING_STOP,
	DB8131M_RUNMODE_CAPTURING,
	DB8131M_RUNMODE_CAPTURE_STOP,
};

struct db8131m_firmware {
	u32 addr;
	u32 size;
};

struct gps_info_common {
	u32 direction;
	u32 dgree;
	u32 minute;
	u32 second;
};

struct db8131m_gps_info {
	u8 gps_buf[8];
	u8 altitude_buf[4];
	s32 gps_timeStamp;
};

struct db8131m_exif {
	u16 exp_time_den;
	u16 iso;
	u16 flash;
};

struct db8131m_regset {
	u32 size;
	u8 *data;
};

#ifdef CONFIG_LOAD_FILE
#if 0
#define dbg_setfile(fmt, ...)	\
	printk(KERN_ERR TAG_NAME fmt, ##__VA_ARGS__)
#else
#define dbg_setfile(fmt, ...)
#endif				/* 0 */

typedef struct regs_array_type {
	u16 subaddr;
	u16 value;
} regs_short_t;

struct db8131m_regset_table {
	const regs_short_t *reg;
	int array_size;
	char *name;
};
/*#define DEBUG_WRITE_REGS*/
#if 0
struct db8131m_regset_table {
	const char	*const name;
};
#endif

#define DB8131M_REGSET(x, y)		\
	([(x)] = {			\
		.name		= #y,	\
	})

#define DB8131M_REGSET_TABLE(y)	\
	{				\
		.name		= #y,	\
}
#else
struct db8131m_regset_table {
	const u16	*const reg;
	const u32	array_size;
#ifdef DEBUG_WRITE_REGS
	const char	*const name;
#endif
};

#ifdef DEBUG_WRITE_REGS
#define DB8131M_REGSET(x, y)		\
	([(x)] = {					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
	})

#define DB8131M_REGSET_TABLE(y)		\
	{					\
		.reg		= (y),			\
		.array_size	= ARRAY_SIZE((y)),	\
		.name		= #y,			\
}
#else
#define DB8131M_REGSET(x, y)
#define DB8131M_REGSET_TABLE(y)
#endif /* DEBUG_WRITE_REGS */
#endif /* CONFIG_LOAD_FILE */

struct db8131m_regs {
	struct db8131m_regset_table ev[EV_MAX_V4L2];
	struct db8131m_regset_table preview_size[DB8131M_PREVIEW_MAX];
	struct db8131m_regset_table capture_start[DB8131M_CAPTURE_MAX];
	struct db8131m_regset_table fps[I_FPS_MAX];
	struct db8131m_regset_table init_reg;
	struct db8131m_regset_table vt_init_reg;
	struct db8131m_regset_table record;
	struct db8131m_regset_table capture_on;
	struct db8131m_regset_table preview_on;
	struct db8131m_regset_table stream_off;
	struct db8131m_regset_table effect[IMAGE_EFFECT_MAX];
	struct db8131m_regset_table wb[WHITE_BALANCE_MAX];
	struct db8131m_regset_table antibanding[V4L2_CID_POWER_LINE_FREQUENCY_AUTO+1];
};

struct db8131m_state {
	struct db8131m_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format req_fmt;
	struct v4l2_mbus_framefmt format;
	struct media_pad pad;
	struct db8131m_framesize *preview;
	struct db8131m_framesize *capture;
	struct db8131m_exif exif;
	struct db8131m_interval stream_time;
	const struct db8131m_regs *regs;
	struct mutex ctrl_lock;
	struct workqueue_struct *workqueue;
	enum db8131m_runmode runmode;
	enum v4l2_sensor_mode sensor_mode;
	enum stream_mode_t stream_mode;
	enum v4l2_pix_format_mode format_mode;
	enum v4l2_flash_mode flash_mode;
	enum v4l2_scene_mode scene_mode;
	enum v4l2_wb_mode wb_mode;
	enum v4l2_power_line_frequency antibanding_mode;

	s32 vt_mode;
	s32 req_fps;
	s32 fps;
	s32 freq;		/* MCLK in Hz */
	u32 one_frame_delay_ms;
	u8 *dbg_level;
	s32 streamoff_delay;	/* ms, type is signed */

	u32 recording:1;
	u32 need_update_frmsize:1;
	u32 initialized:1;
};

static inline struct  db8131m_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct db8131m_state, sd);
}

static inline void debug_msleep(struct v4l2_subdev *sd, u32 msecs)
{
	cam_dbg("delay for %dms\n", msecs);
	msleep(msecs);
}

/*********** Sensor specific ************/
/*#define DB8131M_CHIP_ID	0x05CC
#define DB8131M_CHIP_REV	0x0001*/

#define FORMAT_FLAGS_COMPRESSED		0x3

#define POLL_TIME_MS		10
#define CAPTURE_POLL_TIME_MS    1000

/* maximum time for one frame in norma light */
#define ONE_FRAME_DELAY_MS_NORMAL		66
/* maximum time for one frame in low light: minimum 10fps. */
#define ONE_FRAME_DELAY_MS_LOW			100
/* maximum time for one frame in night mode: 6fps */
#define ONE_FRAME_DELAY_MS_NIGHTMODE		166

/* The Path of Setfile */
#ifdef CONFIG_LOAD_FILE
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct test {
	u8 data;
	struct test *nextBuf;
};
static struct test *testBuf;
static s32 large_file;

#define TEST_INIT	\
{			\
	.data = 0;	\
	.nextBuf = NULL;	\
}

#define TUNING_FILE_PATH "/mnt/sdcard/db8131m_reg.h"
#endif /* CONFIG_LOAD_FILE*/

#include "db8131m_reg.h"

#endif /* __DB8131M_H__ */
