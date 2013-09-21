
/* drivers/staging/mrst/drv/panel-tc35876x-vid.h
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

/* MSIC PWM duty cycle goes up to 0x63 = 99% */
#define BACKLIGHT_DUTY_FACTOR   0x63
#define PWM0DUTYCYCLE           0x67

/* panel info */
#define TC35876X_PANEL_WIDTH    216
#define TC35876X_PANEL_HEIGHT   135

/* Panel CABC registers */
#define PANEL_PWM_CONTROL       0x90
#define PANEL_FREQ_DIVIDER_HI   0x91
#define PANEL_FREQ_DIVIDER_LO   0x92
#define PANEL_DUTY_CONTROL      0x93
#define PANEL_MODIFY_RGB        0x94
#define PANEL_FRAMERATE_CONTROL 0x96
#define PANEL_PWM_MIN           0x97
#define PANEL_PWM_REF           0x98
#define PANEL_PWM_MAX           0x99
#define PANEL_ALLOW_DISTORT     0x9A
#define PANEL_BYPASS_PWMI       0x9B

/* Panel color management registers */
#define PANEL_CM_ENABLE         0x700
#define PANEL_CM_HUE            0x701
#define PANEL_CM_SATURATION     0x702
#define PANEL_CM_INTENSITY      0x703
#define PANEL_CM_BRIGHTNESS     0x704
#define PANEL_CM_CE_ENABLE      0x705
#define PANEL_CM_PEAK_EN        0x710
#define PANEL_CM_GAIN           0x711
#define PANEL_CM_HUETABLE_START 0x730
#define PANEL_CM_HUETABLE_END   0x747 /* inclusive */
#define I2C_PCI_DEVICE_ID       0x082E

/* bridge registers */
/* DSI D-PHY Layer Registers */
#define D0W_DPHYCONTTX          0x0004
#define CLW_DPHYCONTRX          0x0020
#define D0W_DPHYCONTRX          0x0024
#define D1W_DPHYCONTRX          0x0028
#define D2W_DPHYCONTRX          0x002C
#define D3W_DPHYCONTRX          0x0030
#define COM_DPHYCONTRX          0x0038
#define CLW_CNTRL               0x0040
#define D0W_CNTRL               0x0044
#define D1W_CNTRL               0x0048
#define D2W_CNTRL               0x004C
#define D3W_CNTRL               0x0050
#define DFTMODE_CNTRL           0x0054



/* DSI PPI Layer Registers */
#define PPI_STARTPPI            0x0104
#define PPI_BUSYPPI             0x0108
#define PPI_LINEINITCNT         0x0110
#define PPI_LPTXTIMECNT         0x0114
#define PPI_LANEENABLE          0x0134
#define PPI_TX_RX_TA            0x013C
#define PPI_CLS_ATMR            0x0140
#define PPI_D0S_ATMR            0x0144
#define PPI_D1S_ATMR            0x0148
#define PPI_D2S_ATMR            0x014C
#define PPI_D3S_ATMR            0x0150
#define PPI_D0S_CLRSIPOCOUNT    0x0164
#define PPI_D1S_CLRSIPOCOUNT    0x0168
#define PPI_D2S_CLRSIPOCOUNT    0x016C
#define PPI_D3S_CLRSIPOCOUNT    0x0170
#define CLS_PRE                 0x0180
#define D0S_PRE                 0x0184
#define D1S_PRE                 0x0188
#define D2S_PRE                 0x018C
#define D3S_PRE                 0x0190
#define CLS_PREP                0x01A0
#define D0S_PREP                0x01A4
#define D1S_PREP                0x01A8
#define D2S_PREP                0x01AC
#define D3S_PREP                0x01B0
#define CLS_ZERO                0x01C0
#define D0S_ZERO                0x01C4
#define D1S_ZERO                0x01C8
#define D2S_ZERO                0x01CC
#define D3S_ZERO                0x01D0
#define PPI_CLRFLG              0x01E0
#define PPI_CLRSIPO             0x01E4
#define HSTIMEOUT               0x01F0
#define HSTIMEOUTENABLE         0x01F4



/* DSI Protocol Layer Registers */
#define DSI_STARTDSI            0x0204
#define DSI_BUSYDSI             0x0208
#define DSI_LANEENABLE          0x0210
#define DSI_LANESTATUS0         0x0214
#define DSI_LANESTATUS1         0x0218
#define DSI_INTSTATUS           0x0220
#define DSI_INTMASK             0x0224
#define DSI_INTCLR              0x0228
#define DSI_LPTXTO              0x0230

/* DSI General Registers */
#define DSIERRCNT               0x0300

/* DSI Application Layer Registers */
#define APLCTRL                 0x0400
#define RDPKTLN                 0x0404

/* Video Path Registers */
#define VPCTRL                  0x0450
#define HTIM1                   0x0454
#define HTIM2                   0x0458
#define VTIM1                   0x045C
#define VTIM2                   0x0460
#define VFUEN                   0x0464



/* LVDS Registers */
#define LVMX0003                0x0480
#define LVMX0407                0x0484
#define LVMX0811                0x0488
#define LVMX1215                0x048C
#define LVMX1619                0x0490
#define LVMX2023                0x0494
#define LVMX2427                0x0498
#define LVCFG                   0x049C
#define LVPHY0                  0x04A0
#define LVPHY1                  0x04A4

/* System Registers */
#define SYSSTAT                 0x0500
#define SYSRST                  0x0504

/* GPIO Registers */
#define GPIOC                   0x0520
#define GPIOO                   0x0524
#define GPIOI                   0x0528

/* I2C Registers */
#define I2CTIMCTRL              0x0540
#define I2CMADDR                0x0544
#define WDATAQ                  0x0548
#define RDATAQ                  0x054C

/* Chip/Rev Registers */
#define IDREG                   0x0580

enum {
	INPUT_R0,	/* 0 */
	INPUT_R1,
	INPUT_R2,
	INPUT_R3,
	INPUT_R4,
	INPUT_R5,
	INPUT_R6,
	INPUT_R7,
	INPUT_G0,	/* 8 */
	INPUT_G1,
	INPUT_G2,
	INPUT_G3,
	INPUT_G4,
	INPUT_G5,
	INPUT_G6,
	INPUT_G7,
	INPUT_B0,	/* 16 */
	INPUT_B1,
	INPUT_B2,
	INPUT_B3,
	INPUT_B4,
	INPUT_B5,
	INPUT_B6,
	INPUT_B7,
	INPUT_HSYNC,	/* 24 */
	INPUT_VSYNC,
	INPUT_DE,
	LOGIC_0,
	/* 28...31 undefined */
};

#define FLD_MASK(start, end)    (((1 << ((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))
#define INPUT_MUX(lvmx03, lvmx02, lvmx01, lvmx00)		\
	(FLD_VAL(lvmx03, 29, 24) | FLD_VAL(lvmx02, 20, 16) |	\
	FLD_VAL(lvmx01, 12, 8) | FLD_VAL(lvmx00, 4, 0))
