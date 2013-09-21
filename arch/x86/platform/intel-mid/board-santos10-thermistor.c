/*
 * thermistor of CLT Project
 *
 * Copyright (C) 2011 Samsung Electrnoics
 * SangYoung Son <hello.son@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/intel_mid_gpadc.h>
#include <linux/platform_device.h>
#include <linux/sec_thermistor.h>

#ifdef CONFIG_SEC_THERMISTOR
#define ADC_SAMPLING_CNT	7

/*Handle for gpadc requests */
static void *intel_mid_gpadc_handle;

/*Below adc table is same as batt_temp_adc table*/
static struct sec_therm_adc_table temper_table_ap[] = {
	{145,	900},
	{150,	890},
	{154,	880},
	{158,	870},
	{163,	860},
	{167,	850},
	{172,	840},
	{177,	830},
	{181,	820},
	{186,	810},
	{192,	800},
	{196,	790},
	{201,	780},
	{207,	770},
	{212,	760},
	{218,	750},
	{224,	740},
	{230,	730},
	{237,	720},
	{243,	710},
	{250,	700},
	{256,	690},
	{263,	680},
	{270,	670},
	{277,	660},
	{284,	650},
	{292,	640},
	{299,	630},
	{307,	620},
	{315,	610},
	{323,	600},
	{331,	590},
	{339,	580},
	{348,	570},
	{357,	560},
	{366,	550},
	{378,	540},
	{388,	530},
	{397,	520},
	{406,	510},
	{416,	500},
	{425,	490},
	{435,	480},
	{445,	470},
	{455,	460},
	{465,	450},
	{476,	440},
	{486,	430},
	{496,	420},
	{507,	410},
	{517,	400},
	{528,	390},
	{538,	380},
	{549,	370},
	{560,	360},
	{570,	350},
	{581,	340},
	{592,	330},
	{603,	320},
	{613,	310},
	{624,	300},
	{636,	290},
	{646,	280},
	{657,	270},
	{667,	260},
	{677,	250},
	{688,	240},
	{698,	230},
	{708,	220},
	{718,	210},
	{728,	200},
	{739,	190},
	{748,	180},
	{758,	170},
	{767,	160},
	{776,	150},
	{785,	140},
	{794,	130},
	{803,	120},
	{811,	110},
	{819,	100},
	{828,	90},
	{836,	80},
	{844,	70},
	{851,	60},
	{858,	50},
	{865,	40},
	{872,	30},
	{879,	20},
	{885,	10},
	{892,	0},
	{898,	-10},
	{904,	-20},
	{910,	-30},
	{915,	-40},
	{921,	-50},
	{925,	-60},
	{930,	-70},
	{935,	-80},
	{940,	-90},
	{944,	-100},
	{948,	-110},
	{952,	-120},
	{956,	-130},
	{960,	-140},
	{963,	-150},
	{986,	-160},
	{990,	-170},
	{993,	-180},
	{996,	-190},
	{999,	-200},
};

static int ctp_get_adc_data(void)
{
	int ret = 0;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i, adc_data;
	int gpadc_sensor_val = 0;

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {
		ret = intel_mid_gpadc_sample(intel_mid_gpadc_handle,
					    1,
					    &gpadc_sensor_val);

		if (ret) {
			pr_err("error reading temp adc from channel, rc = %d\n",
						ret);
			goto err;
		}
		adc_data = gpadc_sensor_val;

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}

		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (ADC_SAMPLING_CNT - 2);

err:
	return ret;
}

static struct sec_therm_platform_data sec_therm_pdata = {
	.adc_arr_size	= ARRAY_SIZE(temper_table_ap),
	.adc_table	= temper_table_ap,
	.polling_interval = 30 * 1000, /* msecs */
	.get_siop_level = NULL,
	.get_adc_data = ctp_get_adc_data,
	.no_polling     = 1,
};

struct platform_device sec_device_thermistor = {
	.name = "sec-thermistor",
	.id = -1,
	.dev.platform_data = &sec_therm_pdata,
};

static struct platform_device *clt_sec_thermistor[] __initdata = {
	&sec_device_thermistor,
};

/* late_initcall */
int __init sec_santos10_thermistor_init(void)
{
	/* board dependent changes in booting */
	pr_info("(%s:%d)\n", __func__, __LINE__);

	intel_mid_gpadc_handle =
		intel_mid_gpadc_alloc(1,
			      0x04 | CH_NEED_VCALIB | CH_NEED_VREF);

	if (intel_mid_gpadc_handle == NULL) {
		pr_err(
		"%s, ADC allocation failed: Check if ADC driver came up\n",
		__func__);

		return -1;
	}

	platform_add_devices(
		clt_sec_thermistor,
		ARRAY_SIZE(clt_sec_thermistor));

	return 0;
}
#else
int __init sec_santos10_thermistor_init(void)
{
	return 0;
}
#endif /* CONFIG_SEC_THERMISTOR */
