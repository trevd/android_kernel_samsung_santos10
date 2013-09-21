/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>

#ifdef CONFIG_ADC_STMPE811
#include <linux/mfd/stmpe811.h>
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charger.h>

#define SEC_BATTERY_PMIC_NAME ""

#include <asm/intel-mid.h>
#include "sec_libs/sec_common.h"
#include "board-santos10.h"

#include <asm/intel_mid_gpadc.h>
#define ADC_SAMPLING_CNT	5
#define ELENTEC_BATT_ID_AVG	315
#define BATTERY_TYPE_DIFF_VALUE	20
/*Handle for gpadc requests */
static void *sec_bat_gpadc_handle;

#ifdef CONFIG_EXTCON
#include <linux/extcon.h>

/*
 *	Detects TA and USB via extcon notifier.
 */
struct sec_bat_cable {
	struct work_struct work;
	struct notifier_block nb;
	struct extcon_specific_cable_nb extcon_nb;
	struct extcon_dev *edev;
	enum extcon_cable_name cable_type;
	int cable_state;
};

static struct sec_bat_cable support_cable_list[] = {
	{ .cable_type = EXTCON_TA, },
	{ .cable_type = EXTCON_USB, },
	{ .cable_type = EXTCON_CEA936_CHG, },
	{ .cable_type = EXTCON_DESKDOCK_VB, },
	{ .cable_type = EXTCON_CARDOCK_VB, },
	{ .cable_type = EXTCON_SMARTDOCK_TA, },
	{ .cable_type = EXTCON_SMARTDOCK_USB, },
	{ .cable_type = EXTCON_JIG_UARTOFF_VB, },
	{ .cable_type = EXTCON_JIG_USBOFF, },
	{ .cable_type = EXTCON_JIG_USBON, },
};

static void sec_bat_init_cable_notify(void);
#endif

static sec_charging_current_t charging_current_table[] = {
	{2000,	2100,	300,	40*60},	/* POWER_SUPPLY_TYPE_UNKNOWN */
	{460,	0,	0,	0},	/* POWER_SUPPLY_TYPE_BATTERY */
	{460,	0,	0,	0},	/* POWER_SUPPLY_TYPE_UPS */
	{2000,	2100,	300,	40*60},	/* POWER_SUPPLY_TYPE_MAINS */
	{460,	460,	300,	40*60},	/* POWER_SUPPLY_TYPE_USB */
	{460,	460,	300,	40*60},	/* POWER_SUPPLY_TYPE_USB_INVAL */
	{460,	460,	300,	40*60},	/* POWER_SUPPLY_TYPE_USB_DCP */
	{460,	460,	300,	40*60},	/* POWER_SUPPLY_TYPE_USB_CDP */
	{460,	460,	300,	40*60},	/* POWER_SUPPLY_TYPE_USB_ACA */
	{2000,	2100,	300,	40*60},	/* POWER_SUPPLY_TYPE_MISC */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_CARDOCK */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_WIRELESS */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_USB_HOST */
	{0,	0,	0,	0},	/* POWER_SUPPLY_TYPE_OTG */
	{2000,	2100,	300,	40*60},	/* POWER_SUPPLY_TYPE_UARTOFF */
};

static bool sec_bat_adc_none_init(struct platform_device *pdev) {return true; }
static bool sec_bat_adc_none_exit(void) {return true; }
static int sec_bat_adc_none_read(unsigned int channel) {return 0; }

static bool sec_bat_adc_ap_init(struct platform_device *pdev) {return true; }
static bool sec_bat_adc_ap_exit(void) {return true; }
static int sec_bat_adc_ap_read(unsigned int channel) {return 0; }

#define SMTPE811_CHANNEL_ADC_CHECK_1	6
#define SMTPE811_CHANNEL_VICHG		0

static bool sec_bat_adc_ic_init(struct platform_device *pdev) {return true; }
static bool sec_bat_adc_ic_exit(void) {return true; }
static int sec_bat_adc_ic_read(unsigned int channel)
{
	int data;
	int max_voltage;

	data = 0;
	max_voltage = 3300;

	switch (channel) {
	case SEC_BAT_ADC_CHANNEL_CABLE_CHECK:
#ifdef CONFIG_ADC_STMPE811
		data = stmpe811_adc_get_value(
			SMTPE811_CHANNEL_ADC_CHECK_1);
		data = data * max_voltage / 4096;
#endif
		break;
	case SEC_BAT_ADC_CHANNEL_FULL_CHECK:
#ifdef CONFIG_ADC_STMPE811
		data = stmpe811_adc_get_value(
			SMTPE811_CHANNEL_VICHG);
		data = data * max_voltage / 4096;
#endif
		break;
	}

	return data;
}

static bool sec_bat_gpio_init(void)
{
	int err;

	err = gpio_request(get_gpio_by_name("TA_INT"), "TA_INT");
	if (err) {
		pr_err("%s : request of gpio TA_INT failed, %d\n",
			__func__, err);
		return false;
	}

	err = gpio_request(get_gpio_by_name("JIG_ON_18"), "JIG_ON_18");
	if (err) {
		pr_err("%s : request of gpio JIG_ON_18 failed, %d\n",
			__func__, err);
		return false;
	}

	return true;
}

static bool sec_fg_gpio_init(void)
{
	int err;

	err = gpio_request(get_gpio_by_name("FUEL_ALERT"), "FUEL_ALERT");
	if (err) {
		pr_err("%s : request of gpio FUEL_ALERT failed, %d\n",
			__func__, err);
		return false;
	}

	return true;
}

static bool sec_chg_gpio_init(void)
{
	return true;
}

static bool sec_bat_is_lpm(void)
{
	return sec_bootmode == REBOOT_MODE_CHARGING ? true : false;
}

int extended_cable_type;
static void sec_bat_initial_check(void)
{
	union power_supply_propval value;

#ifdef CONFIG_EXTCON
	sec_bat_init_cable_notify();
#endif

	if (POWER_SUPPLY_TYPE_BATTERY < current_cable_type) {
		value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
		psy_do_property("battery", set,
			POWER_SUPPLY_PROP_ONLINE, value);
	} else {
		psy_do_property("sec-charger", get,
				POWER_SUPPLY_PROP_ONLINE, value);
		if (value.intval == POWER_SUPPLY_TYPE_WIRELESS) {
			value.intval =
				POWER_SUPPLY_TYPE_WIRELESS<<ONLINE_TYPE_MAIN_SHIFT;
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		}
	}
}

static bool sec_bat_check_jig_status(void)
{
	int result = 0;

	result = gpio_get_value(get_gpio_by_name("JIG_ON_18"));
	pr_info("%s : gpio_get_value(JIG_ON_18) : result(%d)\n",
			__func__, result);

	return result;
}

static bool sec_bat_switch_to_check(void) { return true; }
static bool sec_bat_switch_to_normal(void) { 	return true; }

static int sec_bat_check_cable_callback(void)
{
	return current_cable_type;
}

static int sec_bat_get_cable_from_extended_cable_type(
	int input_extended_cable_type)
{
	int cable_main, cable_sub, cable_power;
	int cable_type = POWER_SUPPLY_TYPE_UNKNOWN;
	union power_supply_propval value;
	int charge_current_max = 0, charge_current = 0;

	cable_main = GET_MAIN_CABLE_TYPE(input_extended_cable_type);
	if (cable_main != POWER_SUPPLY_TYPE_UNKNOWN)
		extended_cable_type = (extended_cable_type &
			~(int)ONLINE_TYPE_MAIN_MASK) |
			(cable_main << ONLINE_TYPE_MAIN_SHIFT);
	cable_sub = GET_SUB_CABLE_TYPE(input_extended_cable_type);
	if (cable_sub != ONLINE_SUB_TYPE_UNKNOWN)
		extended_cable_type = (extended_cable_type &
			~(int)ONLINE_TYPE_SUB_MASK) |
			(cable_sub << ONLINE_TYPE_SUB_SHIFT);
	cable_power = GET_POWER_CABLE_TYPE(input_extended_cable_type);
	if (cable_power != ONLINE_POWER_TYPE_UNKNOWN)
		extended_cable_type = (extended_cable_type &
			~(int)ONLINE_TYPE_PWR_MASK) |
			(cable_power << ONLINE_TYPE_PWR_SHIFT);

	switch (cable_main) {
	case POWER_SUPPLY_TYPE_CARDOCK:
		switch (cable_power) {
		case ONLINE_POWER_TYPE_BATTERY:
			cable_type = POWER_SUPPLY_TYPE_BATTERY;
			break;
		case ONLINE_POWER_TYPE_TA:
			switch (cable_sub) {
			case ONLINE_SUB_TYPE_MHL:
				cable_type = POWER_SUPPLY_TYPE_USB;
				break;
			case ONLINE_SUB_TYPE_AUDIO:
			case ONLINE_SUB_TYPE_DESK:
			case ONLINE_SUB_TYPE_SMART_NOTG:
			case ONLINE_SUB_TYPE_KBD:
				cable_type = POWER_SUPPLY_TYPE_MAINS;
				break;
			case ONLINE_SUB_TYPE_SMART_OTG:
				cable_type = POWER_SUPPLY_TYPE_CARDOCK;
				break;
			}
			break;
		case ONLINE_POWER_TYPE_USB:
			cable_type = POWER_SUPPLY_TYPE_USB;
			break;
		default:
			cable_type = current_cable_type;
			break;
		}
		break;
	case POWER_SUPPLY_TYPE_MISC:
		switch (cable_sub) {
		case ONLINE_SUB_TYPE_MHL:
			switch (cable_power) {
			case ONLINE_POWER_TYPE_BATTERY:
				cable_type = POWER_SUPPLY_TYPE_BATTERY;
				break;
			case ONLINE_POWER_TYPE_MHL_500:
				cable_type = POWER_SUPPLY_TYPE_MISC;
				charge_current_max = 400;
				charge_current = 400;
				break;
			case ONLINE_POWER_TYPE_MHL_900:
				cable_type = POWER_SUPPLY_TYPE_MAINS;
				charge_current_max = 700;
				charge_current = 700;
				break;
			case ONLINE_POWER_TYPE_MHL_1500:
				cable_type = POWER_SUPPLY_TYPE_MAINS;
				charge_current_max = 1300;
				charge_current = 1300;
				break;
			case ONLINE_POWER_TYPE_USB:
				cable_type = POWER_SUPPLY_TYPE_USB;
				charge_current_max = 300;
				charge_current = 300;
				break;
			default:
				cable_type = cable_main;
			}
			break;
		default:
			cable_type = cable_main;
			break;
		}
		break;
	default:
		cable_type = cable_main;
		break;
	}

	if (charge_current_max == 0) {
		charge_current_max =
			charging_current_table[cable_type].input_current_limit;
		charge_current =
			charging_current_table[cable_type].
			fast_charging_current;
	}
	value.intval = charge_current_max;
	psy_do_property(sec_battery_pdata.charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
	value.intval = charge_current;
	psy_do_property(sec_battery_pdata.charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);
	return cable_type;
}

static bool sec_bat_check_cable_result_callback(
				int cable_type)
{
	current_cable_type = cable_type;

	pr_info("%s cable type (%d)\n",	__func__, cable_type);

	switch (cable_type) {
	case POWER_SUPPLY_TYPE_USB:
		pr_info("%s set vbus applied\n",
			__func__);
		break;
	case POWER_SUPPLY_TYPE_BATTERY:
		pr_info("%s set vbus cut\n",
			__func__);
		break;
	case POWER_SUPPLY_TYPE_MAINS:
		break;
	default:
		pr_err("%s cable type (%d)\n",
			__func__, cable_type);
		return false;
	}

	return true;
}

/* callback for battery check
 * return : bool
 * true - battery detected, false battery NOT detected
 */
static bool sec_bat_check_callback(void)
{
	struct power_supply *psy;
	union power_supply_propval value;

	psy = get_power_supply_by_name(("sec-charger"));
	if (!psy) {
		pr_err("%s: Fail to get psy (%s)\n",
			__func__, "sec_charger");
		value.intval = 1;
	} else {
		int ret;
		ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &(value));
		if (ret < 0) {
			pr_err("%s: Fail to sec-charger get_property (%d=>%d)\n",
				__func__, POWER_SUPPLY_PROP_PRESENT, ret);
			value.intval = 1;
		}
	}

	return value.intval;
}
static bool sec_bat_check_result_callback(void) {return true; }

/* callback for OVP/UVLO check
 * return : int
 * battery health
 */
static int sec_bat_ovp_uvlo_callback(void)
{
	int health;
	health = POWER_SUPPLY_HEALTH_GOOD;

	return health;
}

static bool sec_bat_ovp_uvlo_result_callback(int health) {return true; }

/*
 * val.intval : temperature
 */
static bool sec_bat_get_temperature_callback(
		enum power_supply_property psp,
		union power_supply_propval *val) {return true; }

static bool sec_fg_fuelalert_process(bool is_fuel_alerted) {return true; }

/*
 * get_battid_adc from INTEL PMIC
 */
static int sec_bat_get_battid_adc(void)
{
	int ret = 0;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i, adc_data;
	int gpadc_sensor_val = 0;

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {
		ret = intel_mid_gpadc_sample(sec_bat_gpadc_handle,
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

static const sec_bat_adc_table_data_t temp_table[] = {
	{  497,	 700 },
	{  570,	 650 },
	{  616,	 620 },
	{  633,	 610 },
	{  655,	 600 },
	{  676,	 590 },
	{  684,	 580 },
	{  735,	 550 },
	{  822,	 500 },
	{  930,	 450 },
	{  990,	 420 },
	{ 1013,	 410 },
	{ 1035,	 400 },
	{ 1058,	 390 },
	{ 1080,	 380 },
	{ 1126,	 350 },
	{ 1256,	 300 },
	{ 1350,	 250 },
	{ 1458,	 200 },
	{ 1566,	 150 },
	{ 1652,	 100 },
	{ 1725,	 50 },
	{ 1768,	 20 },
	{ 1782,	 10 },
	{ 1796,	   0 },
	{ 1808,  -10 },
	{ 1818,  -20 },
	{ 1828,  -30 },
	{ 1838,  -40 },
	{ 1847,  -50 },
	{ 1858,  -60 },
	{ 1868,  -70 },
	{ 1888,  -100 },
	{ 1923,  -150 },
	{ 1970,  -200 },
	{ 1995,  -250 },
};

/* ADC region should be exclusive */
static sec_bat_adc_region_t cable_adc_value_table[] = {
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
};

static int polling_time_table[] = {
	10,	/* BASIC */
	30,	/* CHARGING */
	30,	/* DISCHARGING */
	30,	/* NOT_CHARGING */
	3600,	/* SLEEP */
};

/* for MAX17050 */
#if defined(CONFIG_FUELGAUGE_MAX17050)
static struct battery_data_t santos10_battery_data[] = {
	/* SDI battery data */
	{
		.Capacity = 0x32CD,
		.low_battery_comp_voltage = 3600,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000,	0,	0},	/* dummy for top limit */
			{-1250, 0,	3320},
			{-750, 97,	3451},
			{-100, 96,	3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0,	0},	/* dummy for top limit */
		},
		.type_str = "SDI 6800mAh",
	}
};
#endif

sec_battery_platform_data_t sec_battery_pdata = {
	/* NO NEED TO BE CHANGED */
	.initial_check = sec_bat_initial_check,
	.bat_gpio_init = sec_bat_gpio_init,
	.fg_gpio_init = sec_fg_gpio_init,
	.chg_gpio_init = sec_chg_gpio_init,

	.is_lpm = sec_bat_is_lpm,
	.check_jig_status = sec_bat_check_jig_status,
	.check_cable_callback =
		sec_bat_check_cable_callback,
	.get_cable_from_extended_cable_type =
		sec_bat_get_cable_from_extended_cable_type,
	.cable_switch_check = sec_bat_switch_to_check,
	.cable_switch_normal = sec_bat_switch_to_normal,
	.check_cable_result_callback =
		sec_bat_check_cable_result_callback,
	.check_battery_callback =
		sec_bat_check_callback,
	.check_battery_result_callback =
		sec_bat_check_result_callback,
	.ovp_uvlo_callback = sec_bat_ovp_uvlo_callback,
	.ovp_uvlo_result_callback =
		sec_bat_ovp_uvlo_result_callback,
	.fuelalert_process = sec_fg_fuelalert_process,
	.get_temperature_callback =
		sec_bat_get_temperature_callback,

	.adc_api[SEC_BATTERY_ADC_TYPE_NONE] = {
		.init = sec_bat_adc_none_init,
		.exit = sec_bat_adc_none_exit,
		.read = sec_bat_adc_none_read
		},
	.adc_api[SEC_BATTERY_ADC_TYPE_AP] = {
		.init = sec_bat_adc_ap_init,
		.exit = sec_bat_adc_ap_exit,
		.read = sec_bat_adc_ap_read
		},
	.adc_api[SEC_BATTERY_ADC_TYPE_IC] = {
		.init = sec_bat_adc_ic_init,
		.exit = sec_bat_adc_ic_exit,
		.read = sec_bat_adc_ic_read
		},
	.cable_adc_value = cable_adc_value_table,
	.charging_current = charging_current_table,
	.polling_time = polling_time_table,
	/* NO NEED TO BE CHANGED */

	.pmic_name = SEC_BATTERY_PMIC_NAME,

	.adc_check_count = 6,

	.adc_type = {
		SEC_BATTERY_ADC_TYPE_NONE,	/* CABLE_CHECK */
		SEC_BATTERY_ADC_TYPE_AP,	/* BAT_CHECK */
		SEC_BATTERY_ADC_TYPE_AP,	/* TEMP */
		SEC_BATTERY_ADC_TYPE_AP,	/* TEMP_AMB */
		SEC_BATTERY_ADC_TYPE_AP,	/* FULL_CHECK */
	},

	/* Battery */
	.vendor = "SDI SDI",
	.technology = POWER_SUPPLY_TECHNOLOGY_LION,
#if defined(CONFIG_FUELGAUGE_MAX17050)
	.battery_data = (void *)santos10_battery_data,
#else
	.battery_data = 0,
#endif
	.bat_gpio_ta_nconnected =
		93, //PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_TA_nCONNECTED),
	.bat_polarity_ta_nconnected = 0,
	.bat_irq =
		93 + INTEL_MID_IRQ_OFFSET, //PM8921_GPIO_IRQ(PM8921_IRQ_BASE, PMIC_GPIO_TA_nCONNECTED),
	.bat_irq_attr =
		IRQF_TRIGGER_FALLING,
	.cable_check_type =
		SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE |
		SEC_BATTERY_CABLE_CHECK_PSY,
	.cable_source_type =
		SEC_BATTERY_CABLE_SOURCE_EXTERNAL |
		SEC_BATTERY_CABLE_SOURCE_EXTENDED,

	.event_check = true,
	.event_waiting_time = 600,

	/* Monitor setting */
	.polling_type = SEC_BATTERY_MONITOR_ALARM,
	.monitor_initial_count = 3,

	/* Battery check */
	.battery_check_type = SEC_BATTERY_CHECK_NONE,
	.check_count = 3,
	/* Battery check by ADC */
	.check_adc_max = 1440,
	.check_adc_min = 0,

	/* OVP/UVLO check */
	.ovp_uvlo_check_type = SEC_BATTERY_OVP_UVLO_CHGPOLLING,

	/* Temperature check */
	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_FG,
	.temp_adc_table = temp_table,
	.temp_adc_table_size =
		sizeof(temp_table)/sizeof(sec_bat_adc_table_data_t),
	.temp_amb_adc_table = temp_table,
	.temp_amb_adc_table_size =
		sizeof(temp_table)/sizeof(sec_bat_adc_table_data_t),

	.temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP,
	.temp_check_count = 1,
	.temp_high_threshold_event = 645,
	.temp_high_recovery_event = 427,
	.temp_low_threshold_event = -25,
	.temp_low_recovery_event = 18,
	.temp_high_threshold_normal = 600,
	.temp_high_recovery_normal = 429,
	.temp_low_threshold_normal = -34,
	.temp_low_recovery_normal = -9,
	.temp_high_threshold_lpm = 451,
	.temp_high_recovery_lpm = 425,
	.temp_low_threshold_lpm = -19,
	.temp_low_recovery_lpm = 6,

	.full_check_type = SEC_BATTERY_FULLCHARGED_CHGPSY,
	.full_check_type_2nd = SEC_BATTERY_FULLCHARGED_TIME,
	.full_check_count = 1,
	.chg_gpio_full_check =
		152, //PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_CHG_STAT),
	.chg_polarity_full_check = 0,
	.full_condition_type =
		SEC_BATTERY_FULL_CONDITION_SOC |
		SEC_BATTERY_FULL_CONDITION_NOTIMEFULL |
		SEC_BATTERY_FULL_CONDITION_VCELL,
	.full_condition_soc = 97,
	.full_condition_vcell = 4300,

	.recharge_check_count = 2,
	.recharge_condition_type =
		SEC_BATTERY_RECHARGE_CONDITION_VCELL,
	.recharge_condition_soc = 98,
	.recharge_condition_vcell = 4300,

	.charging_total_time = 10 * 60 * 60,
	.recharging_total_time =  90 * 60,
	.charging_reset_time = 0,

	/* Fuel Gauge */
	.fg_irq = 94 + INTEL_MID_IRQ_OFFSET, //MSM_GPIO_TO_INT(GPIO_FUEL_INT),
	.fg_irq_attr =
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	.fuel_alert_soc = 1,
	.repeated_fuelalert = false,
	.capacity_calculation_type =
		SEC_FUELGAUGE_CAPACITY_TYPE_RAW |
		SEC_FUELGAUGE_CAPACITY_TYPE_SCALE |
		SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE,
		/* SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC, */
	.capacity_max = 1000,
	.capacity_max_margin = 30,
	.capacity_min = 0,

	/* Charger */
	.charger_name = "sec-charger",
	.chg_gpio_en = 0,
	.chg_polarity_en = 0,
	.chg_gpio_status =
		152, //PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_CHG_STAT),
	.chg_polarity_status = 0,
	.chg_irq = 0,
	.chg_irq_attr = IRQF_TRIGGER_FALLING,
	.chg_float_voltage = 4350,
};

void *max17050_platform_data(void *info)
{
	pr_info("(%s:%d)\n", __func__, __LINE__);
	return &sec_battery_pdata;
}

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev.platform_data = &sec_battery_pdata,
};

static struct platform_device *santos10_battery_devices[] __initdata = {
	&sec_device_battery,
};

#ifdef CONFIG_EXTCON
/*
 * Detects all cables related charging via extcon notifier.
 */
static void sec_bat_cable_event_worker(struct work_struct *work)
{
	struct sec_bat_cable *cable =
			    container_of(work, struct sec_bat_cable, work);

	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;

	pr_info("%s: '%s' is %s\n", __func__,
			extcon_cable_name[cable->cable_type],
			cable->cable_state ? "attached" : "detached");

	switch (cable->cable_type) {
	case EXTCON_TA:
		if (cable->cable_state)
			current_cable_type = POWER_SUPPLY_TYPE_MAINS;
		else
			current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	case EXTCON_USB:
	case EXTCON_CEA936_CHG:
	case EXTCON_JIG_USBON:
	case EXTCON_JIG_USBOFF:
	case EXTCON_SMARTDOCK_USB:
		if (cable->cable_state)
			current_cable_type = POWER_SUPPLY_TYPE_USB;
		else
			current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	case EXTCON_JIG_UARTOFF_VB:
		if (cable->cable_state)
			current_cable_type = POWER_SUPPLY_TYPE_UARTOFF;
		else
			current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	case EXTCON_DESKDOCK_VB:
	case EXTCON_CARDOCK_VB:
	case EXTCON_SMARTDOCK_TA:
		if (cable->cable_state)
			current_cable_type = POWER_SUPPLY_TYPE_MISC;
		else
			current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable value (%d, %d)\n", __func__,
					cable->cable_type, cable->cable_state);
		break;
	}

	if (!psy || !psy->set_property) {
		pr_err("%s: fail to get battery psy\n", __func__);
		return;
	} else {
		value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
		psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
		pr_info("%s: current_cable_type(%d) set_property\n",
			__func__, current_cable_type);
	}

	return;
}


static int sec_bat_cable_notifier(struct notifier_block *nb,
					unsigned long stat, void *ptr)
{
	struct sec_bat_cable *cable =
			container_of(nb, struct sec_bat_cable, nb);

	cable->cable_state = stat;
	schedule_work(&cable->work);

	return NOTIFY_DONE;
}

static void sec_bat_init_cable_notify(void)
{
	struct sec_bat_cable *cable;
	int i;
	int ret;

	pr_info("%s: register extcon notifier for TA and USB\n", __func__);
	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];

		INIT_WORK(&cable->work, sec_bat_cable_event_worker);
		cable->nb.notifier_call = sec_bat_cable_notifier;

		ret = extcon_register_interest(&cable->extcon_nb,
				"max77693-muic",
				extcon_cable_name[cable->cable_type],
				&cable->nb);

		if (ret)
			pr_err("%s: fail to register extcon notifier(%s, %d)\n",
				__func__, extcon_cable_name[cable->cable_type],
				ret);

		cable->edev = cable->extcon_nb.edev;
		if (!cable->edev)
			pr_err("%s: fail to get extcon device\n", __func__);
	}
}
#endif

/* rootfs_initcall */
int __init sec_santos10_battery_init(void)
{
	u32 batt_id_adc = 0;

	/* board dependent changes in booting */
	pr_info("(%s:%d)\n", __func__, __LINE__);

	/* get gpadc handle for BATTID */
	sec_bat_gpadc_handle =
		intel_mid_gpadc_alloc(1,
			      0x02 | CH_NEED_VCALIB | CH_NEED_VREF);

	if (sec_bat_gpadc_handle == NULL) {
		pr_err(
		"%s, ADC allocation failed: Check if ADC driver came up\n",
		__func__);

		return -1;
	}

	/* check battery type */
	batt_id_adc = sec_bat_get_battid_adc();

	if (batt_id_adc >= ELENTEC_BATT_ID_AVG - BATTERY_TYPE_DIFF_VALUE &&
		batt_id_adc <= ELENTEC_BATT_ID_AVG + BATTERY_TYPE_DIFF_VALUE) {
		santos10_battery_data[0].Capacity = 0x34C4;
		santos10_battery_data[0].type_str = "ELENTEC";
		pr_info(
			"%s: batt_id_adc(%d), Capacity(0x%x), type_str(%s)\n",
			__func__, batt_id_adc,
			santos10_battery_data[0].Capacity,
			santos10_battery_data[0].type_str);
	}

	platform_add_devices(
		santos10_battery_devices,
		ARRAY_SIZE(santos10_battery_devices));

	return 0;
}

#endif
