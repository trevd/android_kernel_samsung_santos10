/*
 *  ctp_common.c - ASoc Machine driver for Intel Clovertrail MID platform
 *
 *  Copyright (C) 2011-13 Intel Corp
 *  Author: KP Jeeja<jeeja.kp@intel.com>
 *  Author: Vaibhav Agarwal <vaibhav.agarwal@intel.com>
 *  Author: Dharageswari.R<dharageswari.r@intel.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */


#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/rpmsg.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/registers.h>
#include <asm/intel_mid_gpadc.h>
#include <asm/intel_scu_pmic.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel_mid_rpmsg.h>
#include <asm/intel_mid_remoteproc.h>
#include <asm/platform_ctp_audio.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <sound/soc.h>
#include "ctp_common.h"
#include "../../../arch/x86/platform/intel-mid/board-santos10.h"
#include "../../../arch/x86/platform/intel-mid/sec_libs/sec_common.h"
#include "../codecs/wm8994.h"

#ifdef CONFIG_SAMSUNG_JACK
#include <linux/input.h>
#include <linux/sec_jack.h>
#include <asm/intel-mid.h>
#ifdef CONFIG_STMPE811_ADC
#include <linux/mfd/stmpe811.h>
#endif
#endif

#define MCLK2_FREQ	32768

#ifndef CONFIG_SND_CTP_MACHINE_WM1811
#define HPDETECT_POLL_INTERVAL	msecs_to_jiffies(1000)	/* 1sec */
#define JACK_DEBOUNCE_REMOVE	50
#define JACK_DEBOUNCE_INSERT	100
#endif

#ifdef CONFIG_SAMSUNG_JACK
#define EAR_DET			34	/* DET_3.5 */
#define EAR_SEND_END	58	/* EAR_SEND_END */
static int gpio_ear_micbias_en;

static struct sec_jack_zone jack_zones[] = {
	[0] = {
		.adc_high	= 0,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_3POLE,
	},
	[1] = {
		.adc_high	= 1250,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_3POLE,
	},
	[2] = {
		.adc_high	= 3400,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_4POLE,
	},
	[3] = {
		.adc_high	= 3600,
		.delay_ms	= 10,
		.check_count	= 10,
		.jack_type	= SEC_HEADSET_4POLE,
	},
	[4] = {
		.adc_high	= 0x7fffffff,
		.delay_ms	= 10,
		.check_count	= 200,
		.jack_type	= SEC_HEADSET_3POLE,
	},
};

/* To support 3-buttons earjack */
static struct sec_jack_buttons_zone jack_buttons_zones[] = {
	{
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 219,
	},
	{
		.code		= KEY_VOLUMEUP,
		.adc_low	= 220,
		.adc_high	= 430,
	},
	{
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 431,
		.adc_high	= 900,
	},
};

static struct sec_jack_buttons_zone jack_buttons_zones_3g[] = {
	{
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 210,
	},
	{
		.code		= KEY_VOLUMEUP,
		.adc_low	= 211,
		.adc_high	= 425,
	},
	{
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 426,
		.adc_high	= 900,
	},
};

static struct sec_jack_buttons_zone jack_buttons_zones_lte[] = {
	{
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 206,
	},
	{
		.code		= KEY_VOLUMEUP,
		.adc_low	= 207,
		.adc_high	= 420,
	},
	{
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 421,
		.adc_high	= 900,
	},
};

static struct sec_jack_buttons_zone jack_buttons_zones_wifi[] = {
	{
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 210,
	},
	{
		.code		= KEY_VOLUMEUP,
		.adc_low	= 211,
		.adc_high	= 425,
	},
	{
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 426,
		.adc_high	= 900,
	},
};

static void set_sec_micbias_state(bool state)
{
	pr_info("sec_jack: %s %s\n", __func__, state ? "on" : "off");

	gpio_set_value(gpio_ear_micbias_en, state);
}

#ifdef CONFIG_STMPE811_ADC
extern int stmpe811_adc_get_value(u8 channel);
#define STMPE811_CHANNEL_EAR_ADC	4
#endif

static int sec_jack_get_adc_value(void)
{
#ifdef CONFIG_STMPE811_ADC
	int ret = stmpe811_adc_get_value(STMPE811_CHANNEL_EAR_ADC);
	return ret;
#else
	return 10;
#endif
}

static struct sec_jack_platform_data sec_jack_data = {
	.set_micbias_state	= set_sec_micbias_state,
	.get_adc_value		= sec_jack_get_adc_value,
	.zones			= jack_zones,
	.num_zones		= ARRAY_SIZE(jack_zones),
	.buttons_zones		= jack_buttons_zones,
	.num_buttons_zones	= ARRAY_SIZE(jack_buttons_zones),
	.det_gpio		= EAR_DET,
	.send_end_gpio		= EAR_SEND_END,
	.det_active_high = 0,
};

static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
	.dev            = {
		.platform_data  = &sec_jack_data,
	},
};

static void sec_jack_gpio_init(void)
{
	gpio_ear_micbias_en = get_gpio_by_name("EAR_MICBIAS_EN");

	if (gpio_request(gpio_ear_micbias_en, "EAR MICBIAS_EN"))
		pr_err(KERN_ERR "gpio_ear_micbias_en GPIO set error!\n");
	gpio_direction_output(gpio_ear_micbias_en, 0);

	sec_jack_data.det_gpio = get_gpio_by_name("DET_3.5");

	sec_jack_data.send_end_gpio = get_gpio_by_name("EAR_SEND_END");

	if(sec_board_id == sec_id_santos103g) {
		sec_jack_data.buttons_zones = jack_buttons_zones_3g;
		sec_jack_data.num_buttons_zones = ARRAY_SIZE(jack_buttons_zones_3g);
	}
	else if(sec_board_id == sec_id_santos10lte) {
		sec_jack_data.buttons_zones = jack_buttons_zones_lte;
		sec_jack_data.num_buttons_zones = ARRAY_SIZE(jack_buttons_zones_lte);
	}
	else if(sec_board_id == sec_id_santos10wifi) {
		sec_jack_data.buttons_zones = jack_buttons_zones_wifi;
		sec_jack_data.num_buttons_zones = ARRAY_SIZE(jack_buttons_zones_wifi);
	}
}
#endif

static int ctp_card_suspend_pre(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd->codec;
	if(!codec->active) {
		snd_soc_dapm_disable_pin(&codec->dapm, "AIF1CLK");
		snd_soc_dapm_sync(&codec->dapm);
		pr_info("%s disable AIF1CLK\n",__func__);
	}

	return 0;
}

static int ctp_card_resume_post(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd->codec;
	snd_soc_dapm_force_enable_pin(&codec->dapm, "AIF1CLK");
	snd_soc_dapm_sync(&codec->dapm);
	pr_info("%s enable AIF1CLK\n",__func__);

	return 0;
}

static int ctp_card_suspend_post(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd->codec;
	struct snd_soc_dai *aif1_dai = card->rtd[0].codec_dai;
	struct snd_soc_dai *aif2_dai = card->rtd[1].codec_dai;
	int ret;

	if(!codec->active) {
#ifndef SND_USE_BIAS_LEVEL
		pr_info("%s disable FLL\n",__func__);

		ret = snd_soc_dai_set_sysclk(aif2_dai, WM8994_SYSCLK_MCLK2,
				MCLK2_FREQ, SND_SOC_CLOCK_IN);
		if (ret < 0) {
			dev_err(aif2_dai->dev, "Failed to switch away from FLL: %d\n",
					ret);
			return ret;
		}

		ret = snd_soc_dai_set_pll(aif2_dai, WM8994_FLL2,
				0, 0, 0);
		if (ret < 0) {
			dev_err(aif2_dai->dev, "Failed to stop FLL: %d\n", ret);
			return ret;
		}

		ret = snd_soc_dai_set_sysclk(aif1_dai, WM8994_SYSCLK_MCLK2,
				MCLK2_FREQ, SND_SOC_CLOCK_IN);
		if (ret < 0) {
			dev_err(aif1_dai->dev, "Failed to switch away from FLL: %d\n",
					ret);
			return ret;
		}

		ret = snd_soc_dai_set_pll(aif1_dai, WM8994_FLL1,
				0, 0, 0);
		if (ret < 0) {
			dev_err(aif1_dai->dev, "Failed to stop FLL: %d\n", ret);
			return ret;
		}
#endif
		snd_soc_update_bits(codec, WM8994_AIF1_MASTER_SLAVE,
				WM8994_AIF1_TRI_MASK, WM8994_AIF1_TRI);
		intel_scu_ipc_set_osc_clk0(false, CLK0_MSIC);

	}

	return 0;
}

static int ctp_card_resume_pre(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd->codec;
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dai;
	int ret;
	int pll_out = 48000 * 256; // LPE send always 48kHz

	if (codec->active)
		return 0;

	intel_scu_ipc_set_osc_clk0(true, CLK0_MSIC);

	snd_soc_update_bits(codec, WM8994_AIF1_MASTER_SLAVE,
					WM8994_AIF1_TRI_MASK, 0);

#ifndef SND_USE_BIAS_LEVEL
	pr_info("%s enable FLL\n",__func__);
	/*
	 * Make sure that we have a system clock not derived from the
	 * FLL, since we cannot change the FLL when the system clock
	 * is derived from it.
	 */
	ret = snd_soc_dai_set_sysclk(codec_dai,
					WM8994_SYSCLK_MCLK2,
					MCLK2_FREQ, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Failed to switch away from FLL: %d\n",
									ret);
		return ret;
	}

	/* Switch the FLL */
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1,
				WM8994_FLL_SRC_MCLK1, DEFAULT_MCLK,
				pll_out);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Unable to start FLL1\n");
		return ret;
	}

	/* Then switch AIF1CLK to it */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
				pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Unable to switch to FLL1\n");
		return ret;
	}
#endif

	return 0;
}

struct snd_soc_card snd_soc_card_ctp = {
	.name = "cloverview_audio",
#ifdef SND_USE_BIAS_LEVEL
	.set_bias_level = ctp_set_bias_level,
	.set_bias_level_post = ctp_set_bias_level_post,
#endif
	.suspend_pre = ctp_card_suspend_pre,
	.resume_post = ctp_card_resume_post,
	.suspend_post = ctp_card_suspend_post,
	.resume_pre = ctp_card_resume_pre,
};

unsigned int rates_8000_16000[] = {
	8000,
	16000,
};

struct snd_pcm_hw_constraint_list constraints_8000_16000 = {
	.count = ARRAY_SIZE(rates_8000_16000),
	.list = rates_8000_16000,
};

unsigned int rates_48000[] = {
	48000,
};

struct snd_pcm_hw_constraint_list constraints_48000 = {
	.count  = ARRAY_SIZE(rates_48000),
	.list   = rates_48000,
};

unsigned int rates_16000[] = {
	16000,
};

struct snd_pcm_hw_constraint_list constraints_16000 = {
	.count  = ARRAY_SIZE(rates_16000),
	.list   = rates_16000,
};

#ifndef CONFIG_SND_CTP_MACHINE_WM1811
static struct snd_soc_jack_gpio hs_gpio[] = {
	[CTP_HSDET_GPIO] = {
		.name = "cs-hsdet-gpio",
		.report = SND_JACK_HEADSET,
		.debounce_time = JACK_DEBOUNCE_INSERT,
		.jack_status_check = ctp_soc_jack_gpio_detect,
		.irq_flags = IRQF_TRIGGER_FALLING,
	},
	[CTP_BTN_GPIO] = {
		.name = "cs-hsbutton-gpio",
		.report = SND_JACK_HEADSET | SND_JACK_BTN_0,
		.debounce_time = 100,
		.jack_status_check = ctp_soc_jack_gpio_detect_bp,
		.irq_flags = IRQF_TRIGGER_FALLING,
	},
};
#endif

int ctp_startup_asp(struct snd_pcm_substream *substream)
{
	pr_debug("%s - applying rate constraint\n", __func__);
	snd_pcm_hw_constraint_list(substream->runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				&constraints_48000);
	return 0;
}
int ctp_startup_vsp(struct snd_pcm_substream *substream)
{
	pr_debug("%s - applying rate constraint\n", __func__);
	snd_pcm_hw_constraint_list(substream->runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				&constraints_16000);
	return 0;
}

int ctp_startup_bt_xsp(struct snd_pcm_substream *substream)
{
	pr_debug("%s - applying rate constraint\n", __func__);
	snd_pcm_hw_constraint_list(substream->runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				&constraints_8000_16000);
	return 0;
}
int get_ssp_bt_sco_master_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(card);
	struct comms_mc_private *ctl = &(ctx->comms_ctl);

	ucontrol->value.integer.value[0] = ctl->ssp_bt_sco_master_mode;
	return 0;
}
EXPORT_SYMBOL_GPL(get_ssp_bt_sco_master_mode);

int set_ssp_bt_sco_master_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(card);
	struct comms_mc_private *ctl = &(ctx->comms_ctl);

	if (ucontrol->value.integer.value[0] == ctl->ssp_bt_sco_master_mode)
		return 0;

	ctl->ssp_bt_sco_master_mode = ucontrol->value.integer.value[0];

	return 0;
}
EXPORT_SYMBOL_GPL(set_ssp_bt_sco_master_mode);

int get_ssp_voip_master_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(card);
	struct comms_mc_private *ctl = &(ctx->comms_ctl);

	ucontrol->value.integer.value[0] = ctl->ssp_voip_master_mode;
	return 0;
}
EXPORT_SYMBOL_GPL(get_ssp_voip_master_mode);

int set_ssp_voip_master_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(card);
	struct comms_mc_private *ctl = &(ctx->comms_ctl);

	if (ucontrol->value.integer.value[0] == ctl->ssp_voip_master_mode)
		return 0;

	ctl->ssp_voip_master_mode = ucontrol->value.integer.value[0];

	return 0;
}
EXPORT_SYMBOL_GPL(set_ssp_voip_master_mode);

int get_ssp_modem_master_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(card);
	struct comms_mc_private *ctl = &(ctx->comms_ctl);

	ucontrol->value.integer.value[0] = ctl->ssp_modem_master_mode;
	return 0;
}
EXPORT_SYMBOL_GPL(get_ssp_modem_master_mode);

int set_ssp_modem_master_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(card);
	struct comms_mc_private *ctl = &(ctx->comms_ctl);

	if (ucontrol->value.integer.value[0] == ctl->ssp_modem_master_mode)
		return 0;

	ctl->ssp_modem_master_mode = ucontrol->value.integer.value[0];

	return 0;
}
EXPORT_SYMBOL_GPL(set_ssp_modem_master_mode);

static int mc_driver_ops(struct ctp_mc_private *ctx,
			 struct ctp_audio_platform_data *pdata)
{
	switch (pdata->spid->product_line_id) {
	case INTEL_CLVTP_PHONE_RHB_ENG:
	case INTEL_CLVTP_PHONE_RHB_PRO:
#ifdef CONFIG_SND_CTP_MACHINE_WM1811
		ctx->ops = ctp_get_rhb_ops();
		return 0;
#else
		if (pdata->spid->hardware_id == CLVTP_PHONE_RHB_VBDV1) {
			ctx->ops = ctp_get_vb_ops();
			return 0;
		} else {
			ctx->ops = ctp_get_rhb_ops();
			return 0;
		}
#endif
	default:
		pr_err("No data for prod line id: %x",
				pdata->spid->product_line_id);
		return -EINVAL;
	};
}

#ifdef SND_USE_BIAS_LEVEL
static int ctp_start_fll1(struct snd_soc_dai *codec_dai)
{
	int ret;
	int pll_out = 48000 * 256; // LPE send always 48kHz

	/*
	 * Make sure that we have a system clock not derived from the
	 * FLL, since we cannot change the FLL when the system clock
	 * is derived from it.
	 */
	ret = snd_soc_dai_set_sysclk(codec_dai,
					WM8994_SYSCLK_MCLK2,
					MCLK2_FREQ, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Failed to switch away from FLL: %d\n",
									ret);
		return ret;
	}

	/* Switch the FLL */
	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1,
				WM8994_FLL_SRC_MCLK1, DEFAULT_MCLK,
				pll_out);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Unable to start FLL1\n");
		return ret;
	}

	/* Then switch AIF1CLK to it */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
				pll_out, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Unable to switch to FLL1\n");
		return ret;
	}

	return 0;
}

static int ctp_stop_fll1(struct snd_soc_dai *aif1_dai, struct snd_soc_dai *aif2_dai)
{
	int ret;

	/*
	 * Playback/capture has stopped, so switch to the slower
	 * MCLK2 for reduced power consumption. hw_params handles
	 * turning the FLL back on when needed.
	 */
	ret = snd_soc_dai_set_sysclk(aif2_dai, WM8994_SYSCLK_MCLK2,
					MCLK2_FREQ, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(aif2_dai->dev, "Failed to switch away from FLL: %d\n",
									ret);
		return ret;
	}

	ret = snd_soc_dai_set_pll(aif2_dai, WM8994_FLL2,
					0, 0, 0);
	if (ret < 0) {
		dev_err(aif2_dai->dev, "Failed to stop FLL: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(aif1_dai, WM8994_SYSCLK_MCLK2,
					MCLK2_FREQ, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(aif1_dai->dev, "Failed to switch away from FLL: %d\n",
									ret);
		return ret;
	}

	ret = snd_soc_dai_set_pll(aif1_dai, WM8994_FLL1,
					0, 0, 0);
	if (ret < 0) {
		dev_err(aif1_dai->dev, "Failed to stop FLL: %d\n", ret);
		return ret;
	}


	return 0;
}

int ctp_set_bias_level(struct snd_soc_card *card,
					struct snd_soc_dapm_context *dapm,
					enum snd_soc_bias_level level)
{
	struct snd_soc_dai *codec_dai = card->rtd[0].codec_dai;
	int ret = 0;

	if (dapm->dev != codec_dai->dev)
		return 0;

	if ((level == SND_SOC_BIAS_PREPARE) &&
			(dapm->bias_level == SND_SOC_BIAS_STANDBY)) {
		pr_info("%s starting FLL1\n", __func__);
		ret = ctp_start_fll1(codec_dai);
	}

	return ret;
}

int ctp_set_bias_level_post(struct snd_soc_card *card,
					struct snd_soc_dapm_context *dapm,
					enum snd_soc_bias_level level)
{
	struct snd_soc_dai *aif1_dai = card->rtd[0].codec_dai;
	struct snd_soc_dai *aif2_dai = card->rtd[1].codec_dai;
	struct manta_wm1811 *machine =
				snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (dapm->dev != codec_dai->dev)
		return 0;

	if (level == SND_SOC_BIAS_STANDBY) {
		pr_info("%s stopping FLL1\n", __func__);
		ret = ctp_stop_fll1(aif1_dai, aif2_dai);
	}

	dapm->bias_level = level;

	return ret;
}
#endif

#ifndef CONFIG_SND_CTP_MACHINE_WM1811
static int set_mic_bias(struct snd_soc_jack *jack,
			const char *bias_widget, bool enable)
{
	struct snd_soc_codec *codec = jack->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	if (enable)
		snd_soc_dapm_force_enable_pin(dapm, bias_widget);
	else
		snd_soc_dapm_disable_pin(dapm, bias_widget);

	snd_soc_dapm_sync(&codec->dapm);

	return 0;
}

int ctp_amp_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k, int event)
{
	int ret;

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		/*Enable  IHFAMP_SD_N  GPIO */
		ret = intel_scu_ipc_iowrite8(GPIOHVCTL, GPIO_AMP_ON);
		if (ret)
			pr_err("write of  failed, err %d\n", ret);
	} else {
		/*Disable  IHFAMP_SD_N  GPIO */
		ret = intel_scu_ipc_iowrite8(GPIOHVCTL, GPIO_AMP_OFF);
		if (ret)
			pr_err("write of  failed, err %d\n", ret);
	}
	return 0;
}

static inline void set_bp_interrupt(struct ctp_mc_private *ctx, bool enable)
{
	if (!enable) {
		if (!atomic_dec_return(&ctx->bpirq_flag)) {
			pr_debug("Disable %d interrupt line\n", ctx->bpirq);
			disable_irq_nosync(ctx->bpirq);
		} else
			atomic_inc(&ctx->bpirq_flag);
	} else {
		if (atomic_inc_return(&ctx->bpirq_flag) == 1) {
			/* If BP intr not enabled */
			pr_debug("Enable %d interrupt line\n", ctx->bpirq);
			enable_irq(ctx->bpirq);
		} else
			atomic_dec(&ctx->bpirq_flag);
	}
}

void cancel_all_work(struct ctp_mc_private *ctx)
{
	struct snd_soc_jack_gpio *gpio;
	cancel_delayed_work_sync(&ctx->jack_work);
	gpio = &hs_gpio[CTP_BTN_GPIO];
	cancel_delayed_work_sync(&gpio->work);
}

int ctp_soc_jack_gpio_detect(void)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio[CTP_HSDET_GPIO];
	int enable, status;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	struct ctp_mc_private *ctx =
		container_of(jack, struct ctp_mc_private, ctp_jack);

	/* During jack removal, spurious BP interrupt may occur.
	 * Better to disable interrupt until jack insert/removal stabilize.
	 * Also cancel the BP and jack_status_verify work if already sceduled */
	cancel_all_work(ctx);
	set_bp_interrupt(ctx, false);
	enable = gpio_get_value(gpio->gpio);
	if (gpio->invert)
		enable = !enable;
	pr_debug("%s:gpio->%d=0x%d\n", __func__, gpio->gpio, enable);
	pr_debug("Current jack status = 0x%x\n", jack->status);

	set_mic_bias(jack, "MIC2 Bias", true);
	status = ctx->ops->hp_detection(codec, jack, enable);
	set_mic_bias(jack, "MIC2 Bias", false);
	if (!status) {
		/* Jack removed, Disable BP interrupts if not done already */
		set_bp_interrupt(ctx, false);
	} else { /* If jack inserted, schedule delayed_wq */
		schedule_delayed_work(&ctx->jack_work, HPDETECT_POLL_INTERVAL);
#ifdef CONFIG_HAS_WAKELOCK
		/*
		 * Take wakelock for one second to give time for the detection
		 * to finish. Jack detection is happening rarely so this doesn't
		 * have big impact to power consumption.
		 */
		wake_lock_timeout(ctx->jack_wake_lock,
				HPDETECT_POLL_INTERVAL + msecs_to_jiffies(50));
#endif
	}

	return status;
}

/* Func to verify Jack status after HPDETECT_POLL_INTERVAL */
void headset_status_verify(struct work_struct *work)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio[CTP_HSDET_GPIO];
	int enable, status;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	unsigned int mask = SND_JACK_HEADSET;
	struct ctp_mc_private *ctx =
		container_of(jack, struct ctp_mc_private, ctp_jack);

	enable = gpio_get_value(gpio->gpio);
	if (gpio->invert)
		enable = !enable;
	pr_debug("%s:gpio->%d=0x%d\n", __func__, gpio->gpio, enable);
	pr_debug("Current jack status = 0x%x\n", jack->status);

	set_mic_bias(jack, "MIC2 Bias", true);
	status = ctx->ops->hp_detection(codec, jack, enable);

	/* Enable Button_press interrupt if HS is inserted
	 * and interrupts are not already enabled
	 */
	if (status == SND_JACK_HEADSET) {
		set_bp_interrupt(ctx, true);
		/* Decrease the debounce time for HS removal detection */
		gpio->debounce_time = JACK_DEBOUNCE_REMOVE;
	} else {
		set_mic_bias(jack, "MIC2 Bias", false);
		/* Disable Button_press interrupt if no Headset */
		set_bp_interrupt(ctx, false);
		/* Restore the debounce time for HS insertion detection */
		gpio->debounce_time = JACK_DEBOUNCE_INSERT;
	}

	if (jack->status != status)
		snd_soc_jack_report(jack, status, mask);

	pr_debug("%s: status 0x%x\n", __func__, status);
}

int ctp_soc_jack_gpio_detect_bp(void)
{
	struct snd_soc_jack_gpio *gpio = &hs_gpio[CTP_BTN_GPIO];
	int enable, hs_status, status;
	struct snd_soc_jack *jack = gpio->jack;
	struct snd_soc_codec *codec = jack->codec;
	struct ctp_mc_private *ctx =
		container_of(jack, struct ctp_mc_private, ctp_jack);

	status = 0;
	enable = gpio_get_value(gpio->gpio);
	if (gpio->invert)
		enable = !enable;
	pr_debug("%s: gpio->%d=0x%x\n", __func__, gpio->gpio, enable);

	/* Check for headset status before processing interrupt */
	gpio = &hs_gpio[CTP_HSDET_GPIO];
	hs_status = gpio_get_value(gpio->gpio);
	if (gpio->invert)
		hs_status = !hs_status;
	pr_debug("%s: gpio->%d=0x%x\n", __func__, gpio->gpio, hs_status);
	pr_debug("Jack status = %x\n", jack->status);
	if (((jack->status & SND_JACK_HEADSET) == SND_JACK_HEADSET)
						&& (!hs_status)) {
		/* HS present, process the interrupt */
		if (!enable) {
			/* Jack removal might be in progress, check interrupt status
			 * before proceeding for button press detection */
			if (!atomic_dec_return(&ctx->bpirq_flag)) {
				status = ctx->ops->bp_detection(codec, jack, enable);
				atomic_inc(&ctx->bpirq_flag);
			} else
				atomic_inc(&ctx->bpirq_flag);
		} else {
			status = jack->status;
			pr_debug("%s:Invalid BP interrupt\n", __func__);
		}
	} else {
		pr_debug("%s:Spurious BP interrupt : jack_status 0x%x, HS_status 0x%x\n",
				__func__, jack->status, hs_status);
		set_mic_bias(jack, "MIC2 Bias", false);
		/* Disable Button_press interrupt if no Headset */
		set_bp_interrupt(ctx, false);
	}
	pr_debug("%s: status 0x%x\n", __func__, status);

	return status;
}
#endif /* !CONFIG_SND_CTP_MACHINE_WM1811 */

#ifdef CONFIG_PM

static int snd_ctp_prepare(struct device *dev)
{
	pr_debug("In %s device name\n", __func__);
	return snd_soc_suspend(dev);
}
static void snd_ctp_complete(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	snd_soc_resume(dev);
}
static int snd_ctp_poweroff(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	snd_soc_poweroff(dev);
	return 0;
}

#else
#define snd_ctp_suspend NULL
#define snd_ctp_resume NULL
#define snd_ctp_poweroff NULL
#endif


static int __devexit snd_ctp_mc_remove(struct platform_device *pdev)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
#ifndef CONFIG_SND_CTP_MACHINE_WM1811
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(soc_card);
#endif

	pr_debug("In %s\n", __func__);
#ifndef CONFIG_SND_CTP_MACHINE_WM1811
	cancel_delayed_work_sync(&ctx->jack_work);
#ifdef CONFIG_HAS_WAKELOCK
	if (wake_lock_active(ctx->jack_wake_lock))
		wake_unlock(ctx->jack_wake_lock);
	wake_lock_destroy(ctx->jack_wake_lock);
#endif
	snd_soc_jack_free_gpios(&ctx->ctp_jack, 2, ctx->hs_gpio_ops);
#endif /* !CONFIG_SND_CTP_MACHINE_WM1811 */
	snd_soc_card_set_drvdata(soc_card, NULL);
	snd_soc_unregister_card(soc_card);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

int snd_ctp_init(struct snd_soc_pcm_runtime *runtime)
{
	int ret, irq;
	struct ctp_mc_private *ctx = snd_soc_card_get_drvdata(runtime->card);
#ifndef CONFIG_SND_CTP_MACHINE_WM1811
	struct snd_soc_jack_gpio *gpio = &hs_gpio[CTP_BTN_GPIO];
#endif
	struct snd_soc_codec *codec = runtime->codec;

	ret = ctx->ops->ctp_init(runtime);
	if (ret) {
		pr_err("CTP init returned failure\n");
		return ret;
	}

#ifndef CONFIG_SND_CTP_MACHINE_WM1811
	/* Setup the HPDET timer */
	INIT_DELAYED_WORK(&ctx->jack_work, headset_status_verify);

	/* Headset and button jack detection */
	ret = snd_soc_jack_new(codec, "Intel MID Audio Jack",
			SND_JACK_HEADSET | SND_JACK_BTN_0, &ctx->ctp_jack);
	if (ret) {
		pr_err("jack creation failed\n");
		return ret;
	}
	ret = snd_soc_jack_add_gpios(&ctx->ctp_jack, 2, ctx->hs_gpio_ops);
	if (ret) {
		pr_err("adding jack GPIO failed\n");
		return ret;
	}
	irq = gpio_to_irq(gpio->gpio);
	if (irq < 0) {
		pr_err("%d:Failed to map gpio_to_irq\n", irq);
		return irq;
	}

	/* Disable Button_press interrupt if no Headset */
	pr_err("Disable %d interrupt line\n", irq);
	disable_irq_nosync(irq);
	atomic_set(&ctx->bpirq_flag, 0);
#endif /* !CONFIG_SND_CTP_MACHINE_WM1811 */
	return ret;
}

/* SoC card */
static int snd_ctp_mc_probe(struct platform_device *pdev)
{
	int ret_val = 0;
	struct ctp_mc_private *ctx;
	struct ctp_audio_platform_data *pdata = pdev->dev.platform_data;

	pr_debug("In %s\n", __func__);
	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_ATOMIC);
	if (!ctx) {
		pr_err("allocation failed\n");
		return -ENOMEM;
	}
#ifdef CONFIG_SND_CTP_MACHINE_WM1811
	/* register the soc card */
	snd_soc_card_ctp.dev = &pdev->dev;

	if (0 != mc_driver_ops(ctx, pdata)) {
		ret_val = -EINVAL;
		goto unalloc;
	}
	ctx->ops->dai_link(&snd_soc_card_ctp);
#else /* !CONFIG_SND_CTP_MACHINE_WM1811 */
#ifdef CONFIG_HAS_WAKELOCK
	ctx->jack_wake_lock =
		devm_kzalloc(&pdev->dev, sizeof(*(ctx->jack_wake_lock)), GFP_ATOMIC);
	if (!ctx->jack_wake_lock) {
		pr_err("allocation failed for wake_lock\n");
		return -ENOMEM;
	}
	wake_lock_init(ctx->jack_wake_lock, WAKE_LOCK_SUSPEND,
			"jack_detect");
#endif
	/* register the soc card */
	snd_soc_card_ctp.dev = &pdev->dev;

	if (0 != mc_driver_ops(ctx, pdata)) {
		ret_val = -EINVAL;
		goto unalloc;
	}
	ctx->ops->dai_link(&snd_soc_card_ctp);
	if (pdata->codec_gpio_hsdet >= 0 && pdata->codec_gpio_button >= 0) {
		hs_gpio[CTP_HSDET_GPIO].gpio = pdata->codec_gpio_hsdet;
		hs_gpio[CTP_BTN_GPIO].gpio = pdata->codec_gpio_button;
		ret_val = gpio_to_irq(hs_gpio[CTP_BTN_GPIO].gpio);
		if (ret_val < 0) {
			pr_err("%d:Failed to map button irq\n", ret_val);
			goto unalloc;
		}
		ctx->bpirq = ret_val;
	}
	ctx->hs_gpio_ops = hs_gpio;
#endif /* CONFIG_SND_CTP_MACHINE_WM1811 */

	snd_soc_card_set_drvdata(&snd_soc_card_ctp, ctx);
	ret_val = snd_soc_register_card(&snd_soc_card_ctp);
	if (ret_val) {
		pr_err("snd_soc_register_card failed %d\n", ret_val);
		goto unalloc;
	}

	platform_set_drvdata(pdev, &snd_soc_card_ctp);
	pr_debug("successfully exited probe\n");
	return ret_val;

unalloc:
#if !defined(CONFIG_SND_CTP_MACHINE_WM1811) && defined(CONFIG_HAS_WAKELOCK)
	if (wake_lock_active(ctx->jack_wake_lock))
		wake_unlock(ctx->jack_wake_lock);
	wake_lock_destroy(ctx->jack_wake_lock);
#endif
	return ret_val;
}

const struct dev_pm_ops snd_ctp_mc_pm_ops = {
	.prepare = snd_ctp_prepare,
	.complete = snd_ctp_complete,
	.poweroff = snd_ctp_poweroff,
};

static struct platform_driver snd_ctp_mc_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ctp_audio",
		.pm   = &snd_ctp_mc_pm_ops,
	},
	.probe = snd_ctp_mc_probe,
	.remove = __devexit_p(snd_ctp_mc_remove),
};

static int __init snd_ctp_driver_init(void)
{
	pr_info("In %s\n", __func__);
	pr_info("%s ret = %d\n", __func__, intel_scu_ipc_osc_clk(OSC_CLK_AUDIO, 19200));

#ifdef CONFIG_SAMSUNG_JACK
	sec_jack_gpio_init();
	platform_device_register(&sec_device_jack);
#endif

	return platform_driver_register(&snd_ctp_mc_driver);
}

static void snd_ctp_driver_exit(void)
{
	pr_debug("In %s\n", __func__);
	platform_driver_unregister(&snd_ctp_mc_driver);
}

static int snd_clv_rpmsg_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;

	if (rpdev == NULL) {
		pr_err("rpmsg channel not created\n");
		ret = -ENODEV;
		goto out;
	}

	dev_info(&rpdev->dev, "Probed snd_clv rpmsg device\n");

	ret = snd_ctp_driver_init();

out:
	return ret;
}

static void snd_clv_rpmsg_remove(struct rpmsg_channel *rpdev)
{
	snd_ctp_driver_exit();
	dev_info(&rpdev->dev, "Removed snd_clv rpmsg device\n");
}

static void snd_clv_rpmsg_cb(struct rpmsg_channel *rpdev, void *data,
				int len, void *priv, u32 src)
{
	dev_warn(&rpdev->dev, "unexpected, message\n");

	print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
			data, len,  true);
}

static struct rpmsg_device_id snd_clv_rpmsg_id_table[] = {
	{ .name = "rpmsg_msic_clv_audio" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, snd_clv_rpmsg_id_table);

static struct rpmsg_driver snd_clv_rpmsg = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= snd_clv_rpmsg_id_table,
	.probe		= snd_clv_rpmsg_probe,
	.callback	= snd_clv_rpmsg_cb,
	.remove		= snd_clv_rpmsg_remove,
};

static int __init snd_clv_rpmsg_init(void)
{
	return register_rpmsg_driver(&snd_clv_rpmsg);
}

late_initcall(snd_clv_rpmsg_init);

static void __exit snd_clv_rpmsg_exit(void)
{
	return unregister_rpmsg_driver(&snd_clv_rpmsg);
}
module_exit(snd_clv_rpmsg_exit);

