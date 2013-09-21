/*
 * Copyright (C) 2013 Samsung Electronics Co. Ltd. All Rights Reserved.
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

#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/lnw_gpio.h>
#include <linux/workqueue.h>

#include <asm/intel-mid.h>
#include <asm/intel_scu_flis.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>
#include <asm/intel_scu_pmic.h>

#include "board-santos10.h"
#include "sec_libs/sec_common.h"

#include <../drivers/staging/android/timed_output.h>

#define PMIC_PWM2CLKDIV0		0x66
#define PMIC_PWM2CLKDIV1		0x65
#define PMIC_PWM2DUTYCYCLE		0x69
#define PMIC_PWMxDUTYCYCLE_MASK		0x7F

#define PWM_DUTY_CYCLE			0x63	/* 99% Duty Cycle */
#define PWM_DEFAULT_VAL			28

#define MAX_TIMEOUT			10000	/* 10sec */

static u16 pwmval = PWM_DEFAULT_VAL;
static int set_pwm_value(u16);

static ssize_t pwmvalue_show(struct device *dev, struct device_attribute *attr,
						char *buf)
{
	int count;

	count = sprintf(buf, "%d\n", pwmval);
	pr_info("secvib: pwmval: %d\n", pwmval);

	return count;
}

ssize_t pwmvalue_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	if (kstrtou16(buf, 0, &pwmval))
		pr_err("secvib: error in storing pwm value\n");

	set_pwm_value(pwmval);

	pr_info("secvib: pwmval: %d\n", pwmval);

	return size;
}

static DEVICE_ATTR(pwmvalue, S_IRUGO | S_IWUSR, pwmvalue_show, pwmvalue_store);

static int santos10_create_secvib_sysfs(void)
{
	int ret;
	struct kobject *vibetonz_kobj;

	vibetonz_kobj = kobject_create_and_add("vibetonz", NULL);
	if (unlikely(!vibetonz_kobj))
		return -ENOMEM;

	ret = sysfs_create_file(vibetonz_kobj, &dev_attr_pwmvalue.attr);
	if (unlikely(ret < 0)) {
		pr_err("secvib: sysfs_create_file failed: %d\n", ret);
		return ret;
	}

	return 0;
}

enum {
	GPIO_MOTOR_EN = 0,
};

static struct gpio vibrator_gpio[] = {
	[GPIO_MOTOR_EN] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "MOT_EN",
	},
};

static struct vibrator {
	struct wake_lock wklock;
	struct hrtimer timer;
	struct mutex lock;
	struct work_struct viboff_wk;
	bool enabled;
} vibdata;

static void vibrator_off(void)
{
	if (!vibdata.enabled)
		return;
	gpio_set_value(vibrator_gpio[GPIO_MOTOR_EN].gpio, 0);
	vibdata.enabled = false;
	wake_unlock(&vibdata.wklock);
}

static void vibrator_off_linear(struct work_struct *dummy)
{
	if (!vibdata.enabled)
		return;
	max77693_haptic_enable(false);
	intel_scu_ipc_update_register(PMIC_PWM2DUTYCYCLE, 0,
						PMIC_PWMxDUTYCYCLE_MASK);
	gpio_set_value(vibrator_gpio[GPIO_MOTOR_EN].gpio, 0);
	vibdata.enabled = false;
	wake_unlock(&vibdata.wklock);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibdata.timer)) {
		ktime_t r = hrtimer_get_remaining(&vibdata.timer);
		return ktime_to_ms(r);
	}

	return 0;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	mutex_lock(&vibdata.lock);

	/* cancel previous timer and set GPIO according to value */
	hrtimer_cancel(&vibdata.timer);

	if (value) {
		wake_lock(&vibdata.wklock);

		gpio_set_value(vibrator_gpio[GPIO_MOTOR_EN].gpio, 1);

		vibdata.enabled = true;

		if (value > 0) {
			if (value > MAX_TIMEOUT)
				value = MAX_TIMEOUT;

			hrtimer_start(&vibdata.timer,
				ns_to_ktime((u64)value * NSEC_PER_MSEC),
				HRTIMER_MODE_REL);
		}
	} else {
		vibrator_off();
	}

	mutex_unlock(&vibdata.lock);
}

static void vibrator_enable_linear(struct timed_output_dev *dev, int value)
{
	mutex_lock(&vibdata.lock);

	/* cancel previous timer and vibeoff workqueue */
	hrtimer_cancel(&vibdata.timer);
	cancel_work_sync(&vibdata.viboff_wk);

	if (value) {
		wake_lock(&vibdata.wklock);

		gpio_set_value(vibrator_gpio[GPIO_MOTOR_EN].gpio, 1);

		max77693_haptic_enable(true);
		vibdata.enabled = true;

		if (value > 0) {
			if (value > MAX_TIMEOUT)
				value = MAX_TIMEOUT;

			hrtimer_start(&vibdata.timer,
				ns_to_ktime((u64)value * NSEC_PER_MSEC),
				HRTIMER_MODE_REL);

			intel_scu_ipc_update_register(PMIC_PWM2DUTYCYCLE,
				PWM_DUTY_CYCLE, PMIC_PWMxDUTYCYCLE_MASK);
		}
	} else {
		schedule_work(&vibdata.viboff_wk);
	}

	mutex_unlock(&vibdata.lock);
}

static struct timed_output_dev to_dev = {
	.name		= "vibrator",
	.get_time	= vibrator_get_time,
	.enable		= vibrator_enable,
};

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vibrator_off();
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart vibrator_timer_func_linear(struct hrtimer *timer)
{
	schedule_work(&vibdata.viboff_wk);
	return HRTIMER_NORESTART;
}

static int set_pwm_value(u16 value)
{
	int ret;
	u8 pwm_clkdiv0 = (u8)(value & 0xFF);
	u8 pwm_clkdiv1 = (u8)((value >> 8) & 0xFF);

	ret = intel_scu_ipc_iowrite8(PMIC_PWM2CLKDIV0, pwm_clkdiv0);
	if (ret < 0) {
		pr_err("vib: %s: Set a PWM division failed.", __func__);
		return -1;
	}

	ret = intel_scu_ipc_iowrite8(PMIC_PWM2CLKDIV1, pwm_clkdiv1);
	if (ret < 0) {
		pr_err("vib: %s: Set a PWM division failed.", __func__);
		return -1;
	}

	return ret;
}

static int __init vibrator_init(void)
{
	int ret;

	vibdata.enabled = false;

	hrtimer_init(&vibdata.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	if (sec_board_rev == 5 || sec_board_rev >= 8 ||
		(sec_board_id == sec_id_santos10lte && sec_board_rev >= 7))
		vibdata.timer.function = vibrator_timer_func_linear;
	else
		vibdata.timer.function = vibrator_timer_func;

	wake_lock_init(&vibdata.wklock, WAKE_LOCK_SUSPEND, "vibrator");
	mutex_init(&vibdata.lock);

	ret = timed_output_dev_register(&to_dev);
	if (ret < 0)
		goto err_to_dev_reg;

	return 0;

err_to_dev_reg:
	mutex_destroy(&vibdata.lock);
	wake_lock_destroy(&vibdata.wklock);

	return -1;
}

/* late_initcall */
int __init sec_santos10_vibrator_init(void)
{
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(vibrator_gpio); i++)
		vibrator_gpio[i].gpio =
				get_gpio_by_name(vibrator_gpio[i].label);
	err = gpio_request_array(vibrator_gpio, ARRAY_SIZE(vibrator_gpio));
	if (unlikely(err)) {
		pr_err("(%s): can't request gpios!\n", __func__);
		return -ENODEV;
	}

	if (sec_board_rev == 5 || sec_board_rev >= 8 ||
		(sec_board_id == sec_id_santos10lte && sec_board_rev >= 7)) {
		set_pwm_value(pwmval);
		INIT_WORK(&vibdata.viboff_wk, vibrator_off_linear);
		to_dev.enable = vibrator_enable_linear;
		santos10_create_secvib_sysfs();
	}

	return vibrator_init();
}

void __init sec_santos10wifi_no_vibrator_init_rest(void)
{
	struct intel_muxtbl *muxtbl;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(vibrator_gpio); i++) {
		muxtbl = intel_muxtbl_find_by_net_name(vibrator_gpio[i].label);
		if (unlikely(!muxtbl))
			continue;
		muxtbl->mux.alt_func = LNW_GPIO;
		muxtbl->mux.pull = DOWN_20K;
		muxtbl->mux.pin_direction = INPUT_ONLY;
		muxtbl->mux.open_drain = OD_ENABLE;
		muxtbl->gpio.label = kasprintf(GFP_KERNEL, "%s.nc",
				muxtbl->gpio.label);
	}
}
