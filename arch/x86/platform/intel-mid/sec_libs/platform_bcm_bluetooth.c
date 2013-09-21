/*
 * Copyright (C) 2012 Google, Inc.
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

#include "platform_bcm_bluetooth.h"

#define BT_LPM_ENABLE

enum {
	GPIO_BT_WAKE = 0,
	GPIO_BT_HOST_WAKE,
	GPIO_BTREG_ON,
#ifdef CONFIG_BLUETOOTH_BCM4330
	GPIO_BT_nRST,
#endif

};

enum {
	GPIO_BT_UART_RXD = 0,
	GPIO_BT_UART_TXD,
	GPIO_BT_UART_CTS,
	GPIO_BT_UART_RTS,
	MAX_GPIO_BT_UART,
};

static struct gpio bt_gpios[] = {
	[GPIO_BT_WAKE] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "BT_WAKE",
	},
	[GPIO_BT_HOST_WAKE] = {
		.flags	= GPIOF_IN,
		.label	= "BT_HOST_WAKE",
	},
	[GPIO_BTREG_ON] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "BT_EN",
	},
#ifdef CONFIG_BLUETOOTH_BCM4330
	[GPIO_BT_nRST] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label	= "BT_nRST",
	},
#endif
};

static struct intel_muxtbl *muxtbl_bt_uart[MAX_GPIO_BT_UART];

static struct gpio bt_uart_gpios[] = {
	[GPIO_BT_UART_RXD] = {
		.flags  = GPIOF_DIR_IN | GPIOF_INIT_HIGH,
		.label	= "BT_UART_RXD",
	},
	[GPIO_BT_UART_TXD] = {
		.flags  = GPIOF_DIR_OUT,
		.label	= "BT_UART_TXD",
	},
	[GPIO_BT_UART_CTS] = {
		.flags  = GPIOF_DIR_IN,
		.label	= "BT_UART_CTS",
	},
	[GPIO_BT_UART_RTS] = {
		.flags  = GPIOF_DIR_OUT,
		.label	= "BT_UART_RTS",
	},
};

static struct rfkill *bt_rfkill;

struct bcm_bt_lpm {
	int wake;
	int host_wake;

	struct hrtimer enter_lpm_timer;
	ktime_t enter_lpm_delay;

	struct uart_port *uport;

	struct wake_lock wake_lock;
	struct wake_lock host_wake_lock;
} bt_lpm;

static int bcm_bt_rfkill_set_power(void *data, bool blocked)
{
	int ret;
	unsigned int i;
	/* rfkill_ops callback. Turn transmitter on when blocked is false */
	if (!blocked) {
		pr_info("[BT] Bluetooth Power On.\n");
		gpio_set_value(bt_gpios[GPIO_BTREG_ON].gpio, 1);
 
		for (i = 0; i < ARRAY_SIZE(bt_uart_gpios); i++) {
			ret = config_pin_flis(muxtbl_bt_uart[i]->mux.pinname,
				PULL, muxtbl_bt_uart[i]->mux.pull);
			if (unlikely(ret))
				pr_err("(%s) failed to config pin flis\n",
					muxtbl_bt_uart[i]->gpio.label);
		}
#ifdef CONFIG_BLUETOOTH_BCM4330
		msleep(100);
		gpio_set_value(bt_gpios[GPIO_BT_nRST].gpio, 1);
#endif
		msleep(50);
	} else {
		pr_info("[BT] Bluetooth Power Off.\n");
#ifdef CONFIG_BLUETOOTH_BCM4330
		gpio_set_value(bt_gpios[GPIO_BT_nRST].gpio, 0);
#endif
		gpio_set_value(bt_gpios[GPIO_BTREG_ON].gpio, 0);
		for (i = 0; i < ARRAY_SIZE(bt_uart_gpios); i++) {
			ret = config_pin_flis(muxtbl_bt_uart[i]->mux.pinname,
				PULL, NONE);
			if (unlikely(ret))
				pr_err("(%s) failed to config pin flis\n",
					muxtbl_bt_uart[i]->gpio.label);
			gpio_direction_output(bt_uart_gpios[i].gpio, 0);
		}
	}
	return 0;
}

static const struct rfkill_ops bcm_bt_rfkill_ops = {
	.set_block = bcm_bt_rfkill_set_power,
};

#ifdef BT_LPM_ENABLE
void uart_enable(struct uart_port *uport, int enable)
{
	if (!uport) {
		pr_err("[BT] uport is not ready\n");
		return;
	}

	if (enable)
		pm_runtime_get(uport->dev);
	else
		pm_runtime_put(uport->dev);
}

static void set_wake_locked(int wake)
{
	if (wake == bt_lpm.wake)
		return;

	bt_lpm.wake = wake;

	if (wake) {
		wake_lock(&bt_lpm.wake_lock);
		uart_enable(bt_lpm.uport, 1);
		gpio_set_value(bt_gpios[GPIO_BT_WAKE].gpio, wake);
	} else {
		gpio_set_value(bt_gpios[GPIO_BT_WAKE].gpio, wake);
		uart_enable(bt_lpm.uport, 0);
		wake_unlock(&bt_lpm.wake_lock);
	}
}

static enum hrtimer_restart enter_lpm(struct hrtimer *timer)
{
	pr_info("[BT] enter_lpm\n");
	set_wake_locked(0);

	return HRTIMER_NORESTART;
}

void bcm_bt_lpm_exit_lpm_locked(struct device *tty)
{
	bt_lpm.uport = (struct uart_port *)dev_get_drvdata(tty);

	hrtimer_try_to_cancel(&bt_lpm.enter_lpm_timer);
	set_wake_locked(1);

	hrtimer_start(&bt_lpm.enter_lpm_timer, bt_lpm.enter_lpm_delay,
		HRTIMER_MODE_REL);
}

static void update_host_wake_locked(int host_wake)
{
	if (host_wake == bt_lpm.host_wake)
		return;

	bt_lpm.host_wake = host_wake;

	if (host_wake) {
		wake_lock(&bt_lpm.host_wake_lock);
		uart_enable(bt_lpm.uport, 1);
	} else  {
		if (wake_lock_active(&bt_lpm.host_wake_lock))
			uart_enable(bt_lpm.uport, 0);
		/* Take a timed wakelock, so that upper layers can take it.
		 * The chipset deasserts the hostwake lock, when there is no
		 * more data to send.
		 */
		wake_lock_timeout(&bt_lpm.host_wake_lock, HZ/2);
	}
}

static irqreturn_t host_wake_isr(int irq, void *dev)
{
	int host_wake;

	pr_info("[BT] getting the Hoskwakeup\n");
	host_wake = gpio_get_value(bt_gpios[GPIO_BT_HOST_WAKE].gpio);
	if(host_wake)
		pr_info("[BT] Hoskwakeup is trigger high, host_wake = %d\n",
			    host_wake);
	else
		pr_info("[BT] Hoskwakeup is trigger Low,  host_wake = %d\n",
				host_wake);

	irq_set_irq_type(irq, host_wake ?
			IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING);

	if (!bt_lpm.uport) {
		bt_lpm.host_wake = host_wake;
		return IRQ_HANDLED;
	}

	update_host_wake_locked(host_wake);

	return IRQ_HANDLED;
}

static int bcm_bt_lpm_init(struct platform_device *pdev)
{
	int irq, ret;

	pr_info("[BT] bcm_bt_lpm_init\n");
	/* initialize the lpm timer */
	hrtimer_init(&bt_lpm.enter_lpm_timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	bt_lpm.enter_lpm_delay = ktime_set(3, 0);
	bt_lpm.enter_lpm_timer.function = enter_lpm;

	bt_lpm.host_wake = 0;

	/* initialize the wake_lock */
	wake_lock_init(&bt_lpm.wake_lock, WAKE_LOCK_SUSPEND,
			 "BTWakeLowPower");
	wake_lock_init(&bt_lpm.host_wake_lock, WAKE_LOCK_SUSPEND,
			 "BTHostWakeLowPower");

	irq = gpio_to_irq(bt_gpios[GPIO_BT_HOST_WAKE].gpio);
	ret = request_irq(irq, host_wake_isr, IRQF_TRIGGER_RISING,
		"bt_host_wake", NULL);
	if (ret) {
		pr_err("[BT] Request_host wake irq failed.\n");
		goto err_lpm_init;
	}
	ret = irq_set_irq_wake(irq, 1);
	if (ret) {
		pr_err("[BT] Set_irq_wake failed.\n");
		goto err_lpm_init;
	}

	return 0;

err_lpm_init:
	wake_lock_destroy(&bt_lpm.wake_lock);
	wake_lock_destroy(&bt_lpm.host_wake_lock);
	return ret;
}
#endif

static int bcm_bluetooth_probe(struct platform_device *pdev)
{
	int rc;
	int i;
	pr_info("[BT] bcm_bluetooth_probe\n");
	for (i = 0; i < ARRAY_SIZE(bt_gpios); i++)
		bt_gpios[i].gpio = get_gpio_by_name(bt_gpios[i].label);

	for (i = 0; i < ARRAY_SIZE(bt_uart_gpios); i++) {
		muxtbl_bt_uart[i] =
			intel_muxtbl_find_by_net_name(
				bt_uart_gpios[i].label);

		bt_uart_gpios[i].gpio = muxtbl_bt_uart[i]->gpio.gpio;
	}

	/* REQUEST GPIO WITHOUT UART */
	gpio_request_array(bt_gpios, ARRAY_SIZE(bt_gpios));

	bt_rfkill = rfkill_alloc("bcm Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &bcm_bt_rfkill_ops,
				NULL);
	if (unlikely(!bt_rfkill)) {
		pr_err("[BT] bt_rfkill alloc failed.\n");
		rc =  -ENOMEM;
		goto err_rfkill_alloc;
	}

	rfkill_init_sw_state(bt_rfkill, false);
	rc = rfkill_register(bt_rfkill);
	if (unlikely(rc)) {
		pr_err("[BT] bt_rfkill register failed.\n");
		rc = -1;
		goto err_rfkill_register;
	}
	rfkill_set_sw_state(bt_rfkill, true);
#ifdef BT_LPM_ENABLE
	rc = bcm_bt_lpm_init(pdev);
	if (rc)
		goto err_lpm_init;
#endif
	return rc;

#ifdef BT_LPM_ENABLE
err_lpm_init:
	rfkill_unregister(bt_rfkill);
#endif
err_rfkill_register:
	rfkill_destroy(bt_rfkill);
err_rfkill_alloc:
	gpio_free_array(bt_gpios, ARRAY_SIZE(bt_gpios));
	return rc;
}

static int bcm_bluetooth_remove(struct platform_device *pdev)
{
	int irq;

	pr_info("[BT] bcm_bluetooth_remove\n");
#ifdef BT_LPM_ENABLE
	irq = gpio_to_irq(bt_gpios[GPIO_BT_HOST_WAKE].gpio);
	irq_set_irq_wake(irq, 0);
	free_irq(irq, NULL);
	set_wake_locked(0);
	wake_unlock(&bt_lpm.host_wake_lock);
	hrtimer_try_to_cancel(&bt_lpm.enter_lpm_timer);
	wake_lock_destroy(&bt_lpm.wake_lock);
	wake_lock_destroy(&bt_lpm.host_wake_lock);
#endif
	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);

	gpio_free_array(bt_gpios, ARRAY_SIZE(bt_gpios));

	return 0;
}

static struct platform_driver bcm_bluetooth_platform_driver = {
	.probe = bcm_bluetooth_probe,
	.remove = bcm_bluetooth_remove,
	.driver = {
		   .name = "bcm_bluetooth",
		   .owner = THIS_MODULE,
	},
};

static int __init bcm_bluetooth_init(void)
{
	pr_info("[BT] bcm_bluetooth_init\n");

	return platform_driver_register(&bcm_bluetooth_platform_driver);
}

static void __exit bcm_bluetooth_exit(void)
{
	platform_driver_unregister(&bcm_bluetooth_platform_driver);
}


module_init(bcm_bluetooth_init);
module_exit(bcm_bluetooth_exit);

MODULE_ALIAS("platform:bcm");
MODULE_DESCRIPTION("bcm_bluetooth");
MODULE_LICENSE("GPL");
