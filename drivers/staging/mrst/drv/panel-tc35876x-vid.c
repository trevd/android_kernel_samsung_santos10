/* drivers/staging/mrst/drv/panel-tc35876x-vid.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/gpio.h>
#include <asm/intel_scu_ipc.h>
#include <linux/lnw_gpio.h>
#include <linux/delay.h>

#include <linux/i2c/tc35876x-sec.h>

#include "mdfld_dsi_dbi.h"
#include "mdfld_dsi_pkg_sender.h"
#include "mdfld_dsi_esd.h"
#include "panel-tc35876x-vid.h"

#define TC358764_PANEL_NAME	"tc358764"

static struct i2c_client *tc35876x_client;

static int tc35876x_write_reg(struct i2c_client *client, u16 reg, u32 value)
{
	int ret;
	u8 tx_data[6];
	struct i2c_msg msg[] = {
		{client->addr, 0, 6, tx_data }
	};

	/* NOTE: Register address big-endian, data little-endian. */
	tx_data[0] = (reg >> 8) & 0xff;
	tx_data[1] = reg & 0xff;
	tx_data[2] = value & 0xff;
	tx_data[3] = (value >> 8) & 0xff;
	tx_data[4] = (value >> 16) & 0xff;
	tx_data[5] = (value >> 24) & 0xff;

	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (unlikely(ret < 0)) {
		pr_err("%s: i2c write failed reg 0x%04x val 0x%08x error %d\n",
				__func__, reg, value, ret);
		return ret;
	}
	if (unlikely(ret < ARRAY_SIZE(msg))) {
		pr_err("%s: reg 0x%04x val 0x%08x msgs %d\n",
				__func__, reg, value, ret);
		return -EAGAIN;
	}
	return ret;
}

static int tc35876x_i2c_read_reg(struct i2c_client *client, u16 reg)
{
	u8 tx_data[2], rx_data[4];
	int ret, val;
	struct i2c_msg msg[2] = {
		/* first write slave position to i2c devices */
		{ client->addr, 0, ARRAY_SIZE(tx_data), tx_data },
		/* Second read data from position */
		{ client->addr, I2C_M_RD, ARRAY_SIZE(rx_data), rx_data}
	};

	tx_data[0] = (reg >> 8) & 0xff;
	tx_data[1] = reg & 0xff;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (unlikely(ret < 0)) {
		pr_err(" [%s] i2c read failed  on reg 0x%04x error %d\n",
				__func__,  reg, ret);
		return ret;
	}
	if (unlikely(ret < ARRAY_SIZE(msg))) {
		pr_err("%s: reg 0x%04x msgs %d\n" ,
				__func__, reg, ret);
		return -EAGAIN;
	}
	val = (int)rx_data[0] << 24 | ((int)(rx_data[1]) << 16) |
		((int)(rx_data[2]) << 8) | ((int)(rx_data[3]));

	return val;
}

void tc35876x_configure_lvds_bridge(void)
{
	struct i2c_client *i2c = tc35876x_client;
	u32 id = 0;
	id = tc35876x_i2c_read_reg(i2c, IDREG);

	pr_info("tc35876x ID 0x%08x\n", id);

	tc35876x_write_reg(i2c, PPI_TX_RX_TA, FLD_VAL(2, 26, 16) |
				FLD_VAL(3, 10, 0));
	tc35876x_write_reg(i2c, PPI_LPTXTIMECNT,
				FLD_VAL(2, 10, 0));

	tc35876x_write_reg(i2c, PPI_D0S_CLRSIPOCOUNT, FLD_VAL(4, 5, 0));
	tc35876x_write_reg(i2c, PPI_D1S_CLRSIPOCOUNT, FLD_VAL(4, 5, 0));
	tc35876x_write_reg(i2c, PPI_D2S_CLRSIPOCOUNT, FLD_VAL(4, 5, 0));
	tc35876x_write_reg(i2c, PPI_D3S_CLRSIPOCOUNT, FLD_VAL(4, 5, 0));

	/* Enabling MIPI & PPI lanes, Enable 4 lanes */
	tc35876x_write_reg(i2c, PPI_LANEENABLE,
		BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
	tc35876x_write_reg(i2c, DSI_LANEENABLE,
		BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
	tc35876x_write_reg(i2c, PPI_STARTPPI, BIT0);
	tc35876x_write_reg(i2c, DSI_STARTDSI, BIT0);

	/* Setting video panel control register,0x00000120 VTGen=ON ?!?!? */
	tc35876x_write_reg(i2c, VPCTRL, 0x03F00120);

	/* Horizontal back porch and horizontal pulse width. 0x00280028 */
	tc35876x_write_reg(i2c, HTIM1, FLD_VAL(50, 24, 16) | FLD_VAL(28, 8, 0));

	/* Horizontal front porch and horizontal active video size. 0x00500500*/
	tc35876x_write_reg(i2c, HTIM2, FLD_VAL(50, 24, 16) |
							FLD_VAL(1280, 10, 0));

	/* Vertical back porch and vertical sync pulse width. 0x000e000a */
	tc35876x_write_reg(i2c, VTIM1, FLD_VAL(14, 23, 16) | FLD_VAL(10, 7, 0));

	/* Vertical front porch and vertical display size. 0x000e0320 */
	tc35876x_write_reg(i2c, VTIM2, FLD_VAL(14, 23, 16) |
							FLD_VAL(800, 10, 0));

	/* Set above HTIM1, HTIM2, VTIM1, and VTIM2 at next VSYNC. */
	tc35876x_write_reg(i2c, VFUEN, BIT0);

	/* Setting LVDS output frequency */
	tc35876x_write_reg(i2c, LVPHY0, 0x00448406);
	udelay(120);
	tc35876x_write_reg(i2c, LVPHY0, 0x00048406);

	/* Soft reset LCD controller. */
	tc35876x_write_reg(i2c, SYSRST, BIT2);

	/*LVDS INPUT MUXING */

	tc35876x_write_reg(i2c, LVMX0003,
			INPUT_MUX(INPUT_R3, INPUT_R2, INPUT_R1, INPUT_R0));
	tc35876x_write_reg(i2c, LVMX0407,
			INPUT_MUX(INPUT_G0, INPUT_R5, INPUT_R7, INPUT_R4));
	tc35876x_write_reg(i2c, LVMX0811,
			INPUT_MUX(INPUT_G7, INPUT_G6, INPUT_G2, INPUT_G1));
	tc35876x_write_reg(i2c, LVMX1215,
			INPUT_MUX(INPUT_B0, INPUT_G5, INPUT_G4, INPUT_G3));
	tc35876x_write_reg(i2c, LVMX1619,
			INPUT_MUX(INPUT_B2, INPUT_B1, INPUT_B7, INPUT_B6));
	tc35876x_write_reg(i2c, LVMX2023,
			INPUT_MUX(LOGIC_0,  INPUT_B5, INPUT_B4, INPUT_B3));
	tc35876x_write_reg(i2c, LVMX2427,
		INPUT_MUX(INPUT_R6, INPUT_DE, INPUT_VSYNC, INPUT_HSYNC));
	/* Enable LVDS transmitter. */
	tc35876x_write_reg(i2c, LVCFG, BIT0);

	/* Clear notifications. Don't write reserved bits. Was write 0xffffffff
	* to 0x0288, must be in error?! */
	tc35876x_write_reg(i2c, DSI_INTCLR, FLD_MASK(31, 30) | FLD_MASK(22, 0));

}

static int tc35876x_vid_power_on(struct mdfld_dsi_config *dsi_config)
{
	struct mdfld_dsi_pkg_sender *sender =
					mdfld_dsi_get_pkg_sender(dsi_config);
	struct tc35876x_platform_data *pdata =
					(struct tc35876x_platform_data *)
					tc35876x_client->dev.platform_data;
	int err;

	/* DISPLAY_3.3V */
	if (pdata->panel_power)
		pdata->panel_power(true);

	tc35876x_configure_lvds_bridge();

	/*send TURN_ON packet*/
	err = mdfld_dsi_send_dpi_spk_pkg_hs(sender,
					MDFLD_DSI_DPI_SPK_TURN_ON);

	if (pdata->backlight_power) {
		msleep(200);
		pdata->backlight_power(true);
	}

	if (unlikely(err)) {
		pr_err("Faild to send turn on packet\n");
		return err;

	}
	return 0;
}

static int tc35876x_vid_power_off(struct mdfld_dsi_config *dsi_config)
{
	struct tc35876x_platform_data *pdata =
					(struct tc35876x_platform_data *)
					tc35876x_client->dev.platform_data;

	if (pdata->backlight_power) {
		pdata->backlight_power(false);
		msleep(200);
	}

	if (pdata->power_rest)
		pdata->power_rest(0);

	pdata->power(0);
	/* It may not need to contorl reset pin during poweroff.. */
	gpio_set_value(pdata->gpio_lvds_reset, 0);

	/* DISPLAY_3.3V */
	if (pdata->panel_power)
		pdata->panel_power(false);

	return 0;
}

static int tc35876x_panel_reset(struct mdfld_dsi_config *dsi_config)
{
	struct tc35876x_platform_data *pdata =
					(struct tc35876x_platform_data *)
					tc35876x_client->dev.platform_data;

	pdata->power(1);
	usleep_range(100, 110);

	gpio_set_value(pdata->gpio_lvds_reset, 0);
	/* FIXME:
	 * per spec, the min period of reset signal is 50 nano secs,
	 * but no detailed description. Here wait 0.5~1ms for safe.
	 */
	usleep_range(500, 1000);
	gpio_set_value(pdata->gpio_lvds_reset, 1);

	if (pdata->power_rest)
		pdata->power_rest(1);

	return 0;
}

static int tc35876x_vid_detect(struct mdfld_dsi_config *dsi_config)
{
	int status;
	struct drm_device *dev = dsi_config->dev;
	struct mdfld_dsi_hw_registers *regs = &dsi_config->regs;
	int pipe = dsi_config->pipe;
	u32 dpll_val, device_ready_val;

	u32 dspcntr_reg = dsi_config->regs.dspcntr_reg;
	u32 pipeconf_reg = dsi_config->regs.pipeconf_reg;
	u32 mipi_reg = dsi_config->regs.mipi_reg;
	u32 device_ready_reg = dsi_config->regs.device_ready_reg;
	u32 dpll_reg = dsi_config->regs.dpll_reg;
	u32 fp_reg = dsi_config->regs.fp_reg;
	int mipi_enabled = 0;
	int dsi_controller_enabled = 0;
	int use_isbl = 0;
	struct mdfld_dsi_pkg_sender *sender =
		mdfld_dsi_get_pkg_sender(dsi_config);
	u32 val;
	u32 tmp;
	int retry;
	int pipe0_enabled;
	int pipe2_enabled;


	if (pipe == 0) {
		/*
		 * FIXME: WA to detect the panel connection status, and need to
		 * implement detection feature with get_power_mode DSI command.
		 */
		if (!ospm_power_using_hw_begin(OSPM_DISPLAY_ISLAND,
					OSPM_UHB_FORCE_POWER_ON)) {
			pr_err("hw begin failed\n");
			return -EAGAIN;
		}

		dpll_val = REG_READ(regs->dpll_reg);
		device_ready_val = REG_READ(regs->device_ready_reg);


		if (REG_READ(mipi_reg) & BIT31)
			mipi_enabled = 1;

		if (REG_READ(device_ready_reg) & BIT0)
			dsi_controller_enabled = 1;

		/* This is workarounds for Intel Stand-alone Bootloader.
		 * We can not distinguish bootloader between
		 * droidboot and ISBL. */
		if (REG_READ(pipeconf_reg) == 0xE0000000
			&& REG_READ(mipi_reg) == 0x80010000) {
			pr_err(
			"%s: It seems to use Intel Stand-alone Bootloader...\n",
			__func__);
			use_isbl = 1;
			goto jumpcode;
		}
		if (mipi_enabled == 0) {
			/*reconfig if dsi controller is active*/
			if (mipi_enabled || dsi_controller_enabled) {
				tmp = REG_READ(pipeconf_reg);
				val = REG_READ(dspcntr_reg) & ~BIT31;
				REG_WRITE(dspcntr_reg, val);
				val = tmp | 0x000c0000;
				REG_WRITE(pipeconf_reg, val);

				val = REG_READ(pipeconf_reg) & ~BIT31;
				REG_WRITE(pipeconf_reg, val);

				retry = 100000;
				while (--retry &&
					(REG_READ(pipeconf_reg) * BIT30))
					udelay(5);

				if (unlikely(!retry))
					pr_err("Failed to disable pipe\n");

				/*shutdown panel*/
				mdfld_dsi_send_dpi_spk_pkg_hs(sender,
					MDFLD_DSI_DPI_SPK_SHUT_DOWN);

				/*disable DSI controller*/
				REG_WRITE(device_ready_reg, 0x0);

				/*Disable mipi port*/
				REG_WRITE(mipi_reg,
					REG_READ(mipi_reg) & ~BIT31);

				/*Disable DSI PLL*/
				pipe0_enabled =
					(REG_READ(PIPEACONF) & BIT31) ? 1 : 0;
				pipe2_enabled =
					(REG_READ(PIPECCONF) & BIT31) ? 1 : 0;

				if (!pipe0_enabled && !pipe2_enabled) {
					REG_WRITE(dpll_reg , 0x0);
					/*power gate pll*/
					REG_WRITE(dpll_reg, BIT30);
				}
			}

			/*enable DSI PLL*/
			if (!(REG_READ(dpll_reg) & BIT31)) {
				REG_WRITE(dpll_reg, 0x0);
				REG_WRITE(fp_reg, 0xc1);
				REG_WRITE(dpll_reg, ((0x800000) & ~BIT30));
				udelay(2);
				val = REG_READ(dpll_reg);
				REG_WRITE(dpll_reg, (val | BIT31));

				/*wait for PLL lock on pipe*/
				retry = 10000;
				while (--retry &&
						!(REG_READ(PIPEACONF) & BIT29))
					udelay(3);
				if (unlikely(!retry))
					pr_err("PLL failed to lock on pipe\n");
			}

			/*reconfig mipi port lane configuration*/
			REG_WRITE(mipi_reg, REG_READ(mipi_reg)
				| MDFLD_DSI_DATA_LANE_4_0
				| PASS_FROM_SPHY_TO_AFE);

			dpll_val = REG_READ(regs->dpll_reg);

			/* Enable the pipe */
			tmp = REG_READ(pipeconf_reg);
			if ((tmp & PIPEACONF_ENABLE) == 0) {
				/* Enable Pipe */
				tmp |= PIPEACONF_ENABLE;
				/* Enable Display/Overplay Planes */
				tmp &= ~PIPECONF_PLANE_OFF;
				/* Enable Cursor Planes */
				tmp &= ~PIPECONF_CURSOR_OFF;
				REG_WRITE(pipeconf_reg, tmp);
				REG_READ(pipeconf_reg);

				/* Wait for for the pipe enable
				 * to take effect.*/
				intel_wait_for_pipe_enable_disable(dev,
					pipe, true);
			}

jumpcode:
			if ((device_ready_val & DSI_DEVICE_READY) &&
			    (dpll_val & DPLL_VCO_ENABLE)) {
				if (use_isbl)
					dsi_config->dsi_hw_context.panel_on =
									true;

				psb_enable_vblank(dev, pipe);
			} else {
				dsi_config->dsi_hw_context.panel_on = false;
				pr_info("%s: panel is not detected!\n",
					__func__);
			}
		}

		status = MDFLD_DSI_PANEL_CONNECTED;

		ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);
	} else {
		pr_info("%s: do NOT support dual panel\n", __func__);
		status = MDFLD_DSI_PANEL_DISCONNECTED;
	}

	return status;
}

static struct drm_display_mode *tc35876x_vid_get_config_mode(void)
{
	struct drm_display_mode *mode;
	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (unlikely(!mode))
		return NULL;

	mode->hdisplay = 1280;
	mode->vdisplay = 800;
	mode->hsync_start = mode->hdisplay + 80;
	mode->hsync_end = mode->hsync_start + 40;
	mode->htotal = mode->hsync_end + 40;

	mode->vsync_start = mode->vdisplay + 14;
	mode->vsync_end = mode->vsync_start + 10;
	mode->vtotal = mode->vsync_end + 14;
	mode->vrefresh = 60;
	mode->clock = mode->vrefresh * mode->htotal * mode->vtotal / 1000;

	pr_info("hdisplay is %d\n", mode->hdisplay);
	pr_info("vdisplay is %d\n", mode->vdisplay);
	pr_info("HSS is %d\n", mode->hsync_start);
	pr_info("HSE is %d\n", mode->hsync_end);
	pr_info("htotal is %d\n", mode->htotal);
	pr_info("VSS is %d\n", mode->vsync_start);
	pr_info("VSE is %d\n", mode->vsync_end);
	pr_info("vtotal is %d\n", mode->vtotal);
	pr_info("clock is %d\n", mode->clock);

	mode->type |= DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}

static void tc35876x_vid_get_panel_info(int pipe, struct panel_info *pi)
{
	if (pipe == 0) {
		pi->width_mm = TC35876X_PANEL_WIDTH;
		pi->height_mm = TC35876X_PANEL_HEIGHT;
	}
}

static int tc35876x_vid_set_brightness(
			struct mdfld_dsi_config *dsi_config, int level)
{
	return 0;
}

static void tc35876x_vid_dsi_controller_init(
					struct mdfld_dsi_config *dsi_config)
{
	struct mdfld_dsi_hw_context *hw_ctx = &dsi_config->dsi_hw_context;
	dsi_config->lane_count = 4;
	dsi_config->bpp = 24;
	dsi_config->lane_config = MDFLD_DSI_DATA_LANE_4_0;
	dsi_config->video_mode = MDFLD_DSI_VIDEO_NON_BURST_MODE_SYNC_EVENTS;

	/* This is for 400 mhz.  Set it to 0 for 800mhz */
	hw_ctx->cck_div = 0;
	hw_ctx->pll_bypass_mode = 0;

	hw_ctx->mipi_control = 0x18;
	hw_ctx->intr_en = 0xffffffff;
	hw_ctx->hs_tx_timeout = 0xdcf50;
	hw_ctx->lp_rx_timeout = 0xffff;
	hw_ctx->turn_around_timeout = 0x14;
	hw_ctx->device_reset_timer = 0xffff;
	hw_ctx->high_low_switch_count = 0x18;
	hw_ctx->init_count = 0x7d0;
	hw_ctx->eot_disable = 0x0;
	hw_ctx->lp_byteclk = 0x3;
	hw_ctx->clk_lane_switch_time_cnt = 0x18000b;
	hw_ctx->video_mode_format = 0xE;

	/*set up func_prg*/
	hw_ctx->dsi_func_prg = (0x200 | dsi_config->lane_count);
	hw_ctx->mipi = PASS_FROM_SPHY_TO_AFE | dsi_config->lane_config;

}

void tc35876x_vid_init(struct drm_device *dev, struct panel_funcs *p_funcs)
{
	struct tc35876x_platform_data *pdata = (struct tc35876x_platform_data *)
				tc35876x_client->dev.platform_data;

	p_funcs->get_config_mode = tc35876x_vid_get_config_mode;
	p_funcs->get_panel_info = tc35876x_vid_get_panel_info;
	p_funcs->dsi_controller_init = tc35876x_vid_dsi_controller_init;
	p_funcs->detect = tc35876x_vid_detect;
	p_funcs->reset = tc35876x_panel_reset;
	p_funcs->power_on = tc35876x_vid_power_on;
	p_funcs->power_off = tc35876x_vid_power_off;
	p_funcs->set_brightness = tc35876x_vid_set_brightness;
}

static int tc35876x_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct tc35876x_platform_data *pdata;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality() failed\n");
		return -ENODEV;
	}

	pdata = (struct tc35876x_platform_data *)
			client->dev.platform_data;
	if (unlikely(!pdata)) {
		pr_err("no platform data found for i2c client %s\n",
					client->name);
		return -EINVAL;
	}
	ret = pdata->platform_init();
	if (unlikely(ret != 0))	{
		pr_err("gpio initialisation failed\n");
		return ret;
	}

	tc35876x_client = client;
	pr_info("%s :tc35876x_client address: %x\n",
				__func__, tc35876x_client->addr);
	pr_info("%s :tc35876x_client name : %s\n",
				__func__, tc35876x_client->name);

	DRM_INFO("%s: register TC358764 panel p_func\n", __func__);
	intel_mid_panel_register(tc35876x_vid_init);

	return 0;
}

static int tc35876x_remove(struct i2c_client *client)
{
	struct tc35876x_platform_data *pdata = (struct tc35876x_platform_data *)
				client->dev.platform_data;

	if (unlikely(!pdata)) {
		pr_err("no platform data found for i2c client%s\n",
					client->name);
		return -EINVAL;
	}
	pdata->platform_deinit();
	return 0;
}

static const struct i2c_device_id tc35876x_bridge_id[] = {
	{ "mipi_tc358764", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, tc35876x_bridge_id);

static struct i2c_driver tc35876x_bridge_i2c_driver = {
	.driver = {
		.name = "mipi_tc358764",
	},
	.id_table = tc35876x_bridge_id,
	.probe = tc35876x_probe,
	.remove = __devexit_p(tc35876x_remove),
};

static int __init tc35876x_lvds_bridge_init(void)
{
	int ret;
	pr_info("[DISPLAY] %s: Enter\n", __func__);
	ret = i2c_add_driver(&tc35876x_bridge_i2c_driver);
	return ret;
}

static void __exit tc35876x_lvds_bridge_exit(void)
{
	pr_info("[DISPLAY] %s\n", __func__);

	i2c_del_driver(&tc35876x_bridge_i2c_driver);
}

module_init(tc35876x_lvds_bridge_init);
module_exit(tc35876x_lvds_bridge_exit);

