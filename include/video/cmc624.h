/* include/video/cmc624.h
 *
 * Copyright (C) 2012 Samsung Electronics Co, Ltd.
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

#ifndef CMC624_HEADER
#define CMC624_HEADER

/* SFR Bank selection */
#define CMC624_REG_SELBANK		0x00

/* A stage configuration */
#define CMC624_REG_DNRHDTROVE		0x01
#define CMC624_REG_DITHEROFF		0x06
#define CMC624_REG_CLKCONT		0x10
#define CMC624_REG_CLKGATINGOFF		0x0a
#define CMC624_REG_INPUTIFCON		0x24
#define CMC624_REG_CLKMONCONT		0x11  /* Clock Monitor Ctrl Register */
#define CMC624_REG_HDRTCEOFF		0x3a
#define CMC624_REG_I2C			0x0d
#define CMC624_REG_BSTAGE		0x0e
#define CMC624_REG_CABCCTRL		0x7c
#define CMC624_REG_PWMCTRL		0xb4
#define CMC624_REG_OVEMAX		0x54

/* A stage image size */
#define CMC624_REG_1280			0x22
#define CMC624_REG_800			0x23

/* B stage image size */
#define CMC624_REG_SCALERINPH		0x09
#define CMC624_REG_SCALERINPV		0x0a
#define CMC624_REG_SCALEROUTH		0x0b
#define CMC624_REG_SCALEROUTV		0x0c

/* EDRAM configuration */
#define CMC624_REG_EDRBFOUT40		0x01
#define CMC624_REG_EDRAUTOREF		0x06
#define CMC624_REG_EDRACPARAMTIM	0x07

/* Vsync Calibartion */
#define CMC624_REG_CALVAL10		0x65

/* tcon output polarity */
#define CMC624_REG_TCONOUTPOL		0x68

/* tcon RGB configuration */
#define CMC624_REG_TCONRGB1		0x6c
#define CMC624_REG_TCONRGB2		0x6d
#define CMC624_REG_TCONRGB3		0x6e

/* Reg update */
#define CMC624_REG_REGMASK		0x28
#define CMC624_REG_SWRESET		0x09
#define CMC624_REG_RGBIFEN		0x26

#define SLEEPMSEC			0xFFFF

/*
 * TUNING DATA LIST ID
 * Each field has own skip field (1-bit), which is used by MENU_SKIP.
 * For example, if 23-bit SKIP field is "1", "SPEC" field should be ignored.
 *
 * -----------------------------------------------------------------------------
 * FIELD |SKIP ACCESSIBILITY(2) | SKIP SPEC(4) | SKIP CABC(1) | SKIP NEGATIVE(1)
 * ------|----------------------|-----------------------------|-----------------
 * BIT	 |27   26-25            | 24   23-20   | 19   18      | 17   16
 * -----------------------------------------------------------------------------
 *
 * ------------------------------------------------------------------
 * FILED | SKIP BYPASS(1) | SKIP APP(5) | SKIP  MODE(3) | SKIP CMD(3)
 * ------------------------------------------------------------------
 * BIT   | 15   14        | 13   12-8   | 7     6-4     | 3    2-0
 * ------------------------------------------------------------------
 */

#define BITPOS_CMD		(0)
#define BITPOS_MODE		(4)
#define BITPOS_APP		(8)
#define BITPOS_BYPASS		(14)
#define BITPOS_NEGATIVE		(16)
#define BITPOS_CABC		(18)
#define BITPOS_SPEC		(20)
#define BITPOS_ACCESSIBILITY	(25)

#define BITMASK_CMD		(0x0F << BITPOS_CMD)
#define BITMASK_MODE		(0x0F << BITPOS_MODE)
#define BITMASK_APP		(0x3F << BITPOS_APP)
#define BITMASK_BYPASS		(0x03 << BITPOS_BYPASS)
#define BITMASK_NEGATIVE	(0x03 << BITPOS_NEGATIVE)
#define BITMASK_CABC		(0x03 << BITPOS_CABC)
#define BITMASK_SPEC		(0x1F << BITPOS_SPEC)
#define BITMASK_ACCESSIBILITY	(0x07 << BITPOS_ACCESSIBILITY)

#define BITMASK_MAIN_TUNE	(BITMASK_CMD | BITMASK_MODE | BITMASK_APP | \
				 BITMASK_CABC)

#define TUNE_DATA_ID(_id, _cmd, _mode, _app, _bypass, \
					_negative, _cabc, _spec, _acc) { \
	(_id) = (((_cmd) << BITPOS_CMD & BITMASK_CMD)	|	\
		((_mode) << BITPOS_MODE & BITMASK_MODE)	|	\
		((_app) << BITPOS_APP & BITMASK_APP)	|	\
		((_bypass) << BITPOS_BYPASS & BITMASK_BYPASS)	|	\
		((_negative) << BITPOS_NEGATIVE & BITMASK_NEGATIVE) | \
		((_cabc) << BITPOS_CABC & BITMASK_CABC)	|	\
		((_spec) << BITPOS_SPEC & BITMASK_SPEC) |	\
		((_acc) << BITPOS_ACCESSIBILITY & BITMASK_ACCESSIBILITY)); \
	pr_debug("TUNE: id=%d (%d, %d, %d, %d, %d, %d, %d, %d)\n",	\
			(_id), (_cmd), (_mode), (_app),		\
			(_bypass), (_negative), (_cabc), (_spec), (_acc)); \
}

#define MENU_SKIP	0xFFFF

#define NUM_PWRLUT_REG	8

/*
 * enum
 */
enum tune_menu_cmd {
	MENU_CMD_TUNE = 0,
	MENU_CMD_INIT,
	MENU_CMD_INIT_LDI,
};

enum tune_menu_mode {
	MENU_MODE_DYNAMIC = 0,
	MENU_MODE_STANDARD,
	MENU_MODE_MOVIE,
	MENU_MODE_AUTO,
	MAX_BACKGROUND_MODE,
};

enum tune_menu_app {
	MENU_APP_UI = 0,
	MENU_APP_VIDEO,
	MENU_APP_VIDEO_WARM,
	MENU_APP_VIDEO_COLD,
	MENU_APP_CAMERA,
	MENU_APP_NAVI,
	MENU_APP_GALLERY,
	MENU_APP_VT,
	MENU_APP_BROWSER,
	MENU_APP_EBOOK,
	MAX_mDNIe_MODE,
	MENU_DMB_NORMAL = 20,
	MAX_DMB_MODE
};

enum tune_menu_cabc {
	MENU_CABC_OFF = 0,
	MENU_CABC_ON,
	MAX_CABC_MODE,
};

enum tune_menu_siop {
	MENU_SIOP_OFF = 0,
	MENU_SIOP_ON,
	MAX_SIOP_MODE,
};

enum tune_menu_negative {
	MENU_NEGATIVE_OFF = 0,
	MENU_NEGATIVE_ON,
	MAX_NEGATIVE_MODE,
};

enum tune_menu_accessibility {
	MENU_ACC_OFF = 0,
	MENU_ACC_NEGATIVE,
	MENU_ACC_COLOR_BLIND,
	MAX_ACC_MODE
};


enum tune_menu_bypass {
	MENU_BYPASS_OFF = 0,
	MENU_BYPASS_ON,
	MAX_BYPASS_MODE,
};

enum auto_brt_val {
	AUTO_BRIGHTNESS_MANUAL = 0,
	AUTO_BRIGHTNESS_VAL1,
	AUTO_BRIGHTNESS_VAL2,
	AUTO_BRIGHTNESS_VAL3,
	AUTO_BRIGHTNESS_VAL4,
	AUTO_BRIGHTNESS_VAL5,
	MAX_AUTO_BRIGHTNESS,
};

enum power_lut_mode {
	PWRLUT_MODE_UI = 0,
	PWRLUT_MODE_VIDEO,
	MAX_PWRLUT_MODE,
};

enum power_lut_level {
	PWRLUT_LEV_INDOOR = 0,
	PWRLUT_LEV_OUTDOOR1,
	PWRLUT_LEV_OUTDOOR2,
	MAX_PWRLUT_LEV,
};



/*
 * structures
 */
struct cmc624_register_set {
	unsigned int reg_addr;
	unsigned int data;
};

struct cabcoff_pwm_cnt_tbl {
	u16 max;
	u16 def;
	u16 min;
};

struct cabcon_pwr_lut_tbl {
	u16 max[MAX_PWRLUT_LEV][MAX_PWRLUT_MODE][NUM_PWRLUT_REG];
	u16 def[MAX_PWRLUT_LEV][MAX_PWRLUT_MODE][NUM_PWRLUT_REG];
	u16 min[MAX_PWRLUT_LEV][MAX_PWRLUT_MODE][NUM_PWRLUT_REG];
};

struct cmc624_panel_data {
	int gpio_ima_sleep;
	int gpio_ima_nrst;
	int gpio_ima_cmc_en;
	bool skip_init;
	bool skip_ldi_cmd;
	u8 *lcd_name;
	u8 lcd_panel_id;

	int (*init_tune_list)(void);
	void (*power_lcd)(bool enable);
	int (*power_vima_1_1V)(int on);
	int (*power_vima_1_8V)(int on);
	int (*init_power)(void);

	/* PWM control */
	struct cabcoff_pwm_cnt_tbl *pwm_tbl;
	struct cabcon_pwr_lut_tbl *pwr_luts;
};

struct tune_data {
	struct list_head list;
	u32 id;
	struct cmc624_register_set *value;
	u32 size;
};

/*
 * extern functions
 */
extern int cmc624_register_tune_data(u32 id,
		struct cmc624_register_set *cmc_set, u32 tune_size);
extern int cmc624_set_pwm(int intensity);
extern void cmc624_set_auto_brightness(enum auto_brt_val auto_brt);

extern void cmc624_power(int on);

#endif /*CMC624_HEADER */
