/*
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

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/switch.h>
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/extcon.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/otg.h>
#include <linux/irq.h>
#include <linux/power_supply.h>
#ifdef CONFIG_SAMSUNG_MHL
#include <linux/sii9234.h>
#endif
#include <asm/intel-mid.h>

#include <linux/regulator/machine.h>
#include "sec_libs/platform_max77693.h"
#include "sec_libs/sec_common.h"
#include "board-santos10.h"

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

#define NAME_USB_PATH_AP	"PDA"
#define NAME_USB_PATH_CP	"MODEM"
#define NAME_USB_PATH_NONE	"NONE"
#define NAME_UART_PATH_AP	"AP"
#define NAME_UART_PATH_CP	"CP"
#define NAME_UART_PATH_NONE	"NONE"

struct intel_otg_vbus {
	struct mutex vbus_lock;
	bool phy_vbus_on;
};

struct  santos10_otg {
	struct device dev;

	struct usb_phy *otg;
	struct intel_otg_vbus vbus_str;
	struct notifier_block otg_nb;
	struct workqueue_struct *vbus_notifier_wq;
	struct work_struct set_vbus_work;

	struct switch_dev dock_switch;
	struct device *switch_dev;

	bool reg_on;
	bool vbus_on;
	bool need_vbus_drive;
	bool usb_phy_suspend_lock;

#ifdef CONFIG_USB_HOST_NOTIFY
	struct host_notifier_platform_data *pdata;
#endif
};

struct intel_vbus_work {
	struct work_struct set_vbus_work;
	bool enable;
};

enum {
	SANTOS10_DOCK_NONE = 0,
	SANTOS10_DOCK_DESK,
	SANTOS10_DOCK_CAR,
	SANTOS10_DOCK_AUDIO = 7,
	SANTOS10_DOCK_SMART = 8,

};

enum {
	GPIO_OTG_EN = 0,
};

static struct santos10_otg santos10_otg_xceiv;
int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

static struct gpio usb_gpios[] = {
	[GPIO_OTG_EN] = {
		.flags	= GPIOF_OUT_INIT_LOW,
		.label = "OTG_EN",
	},
};


static ssize_t santos10_usb_sel_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int usb_sel;
	const char *mode;

	usb_sel = max77693_muic_get_usb_sel();

	switch (usb_sel) {
	case USB_SEL_AP:
		mode = NAME_USB_PATH_AP;
		break;
	case USB_SEL_CP:
		mode = NAME_USB_PATH_CP;
		break;
	default:
		mode = NAME_USB_PATH_NONE;
	};

	return sprintf(buf, "%s\n", mode);
}

static ssize_t santos10_usb_sel_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	if (!strncasecmp(buf, NAME_USB_PATH_AP, strlen(NAME_USB_PATH_AP)))
		max77693_muic_set_usb_sel(USB_SEL_AP);
	else if (!strncasecmp(buf, NAME_USB_PATH_CP, strlen(NAME_USB_PATH_CP)))
		max77693_muic_set_usb_sel(USB_SEL_CP);
	else
		pr_err("%s: input '%s' or '%s'",
				buf, NAME_USB_PATH_AP, NAME_USB_PATH_CP);

	return size;
}

static ssize_t santos10_uart_sel_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int uart_sel;
	const char *mode;

	uart_sel = max77693_muic_get_uart_sel();

	switch (uart_sel) {
	case UART_SEL_AP:
		mode = NAME_UART_PATH_AP;
		break;
	case UART_SEL_CP:
		mode = NAME_UART_PATH_CP;
		break;
	default:
		mode = NAME_UART_PATH_NONE;
	};

	return sprintf(buf, "%s\n", mode);
}

static ssize_t santos10_uart_sel_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	if (!strncasecmp(buf, NAME_UART_PATH_AP, strlen(NAME_UART_PATH_AP)))
		max77693_muic_set_uart_sel(USB_SEL_AP);
	else if (!strncasecmp(buf, NAME_UART_PATH_CP,
				strlen(NAME_UART_PATH_CP)))
		max77693_muic_set_uart_sel(USB_SEL_CP);
	else
		pr_err("%s: input '%s' or '%s'",
				buf, NAME_UART_PATH_AP, NAME_UART_PATH_CP);

	return size;
}

static ssize_t santos10_usb_state_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct santos10_otg *santos10_otg = dev_get_drvdata(dev);
	struct extcon_dev *edev;
	u32 usb_mask;
	const char *mode;

	edev = extcon_get_extcon_dev("max77693-muic");
	if (!edev) {
		pr_err("(%s): fail to get edev\n", __func__);
		return -ENODEV;
	}

	usb_mask = BIT(EXTCON_USB) | BIT(EXTCON_JIG_USBOFF) |
			BIT(EXTCON_JIG_USBON);

	if (usb_mask & edev->state)
		mode = "USB_STATE_CONFIGURED";
	else
		mode = "USB_STATE_NOT_CONFIGURED";

	return sprintf(buf, "%s\n", mode);
}

static ssize_t santos10_adc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int adc;

	adc = max77693_muic_get_status1_adc_value();
	pr_info("adc value = %d\n", adc);

	return sprintf(buf, "%x\n", adc);
}

static ssize_t santos10_jig_on_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct santos10_otg *santos10_otg = dev_get_drvdata(dev);
	struct extcon_dev *edev;
	u32 jig_mask;
	const char *mode;

	edev = extcon_get_extcon_dev("max77693-muic");
	if (!edev) {
		pr_err("(%s): fail to get edev\n", __func__);
		return -ENODEV;
	}

	jig_mask = BIT(EXTCON_JIG_UARTOFF) | BIT(EXTCON_JIG_UARTON) |
			BIT(EXTCON_JIG_USBOFF) | BIT(EXTCON_JIG_USBON);

	if (jig_mask & edev->state)
		mode = "1";
	else
		mode = "0";

	return sprintf(buf, "%s\n", mode);
}

static ssize_t santos10_factory_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	const char *mode;
	bool cardock_support;

	cardock_support = max77693_muic_get_cardock_support();
	if (cardock_support)
		mode = "NOT_FACTORY_MODE";
	else
		mode = "FACTORY_MODE";

	return sprintf(buf, "%s\n", mode);
}

static ssize_t santos10_factory_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	if (!strncasecmp(buf, "FACTORY_START", 13))
		max77693_muic_set_cardock_support(false);
	else
		max77693_muic_set_cardock_support(true);

	return size;
}


static DEVICE_ATTR(usb_sel, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			santos10_usb_sel_show, santos10_usb_sel_store);
static DEVICE_ATTR(uart_sel, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			santos10_uart_sel_show, santos10_uart_sel_store);
static DEVICE_ATTR(usb_state, S_IRUGO, santos10_usb_state_show, NULL);
static DEVICE_ATTR(adc, S_IRUSR | S_IRGRP, santos10_adc_show, NULL);
static DEVICE_ATTR(jig_on, S_IRUSR | S_IRGRP, santos10_jig_on_show, NULL);
static DEVICE_ATTR(apo_factory, S_IRUGO|S_IWUSR|S_IWUSR,
			santos10_factory_show, santos10_factory_store);

static struct attribute *manual_switch_sel_attributes[] = {
	&dev_attr_usb_sel.attr,
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_adc.attr,
	&dev_attr_jig_on.attr,
	&dev_attr_apo_factory.attr,
	NULL,
};

static const struct attribute_group manual_switch_sel_group = {
	.attrs	= manual_switch_sel_attributes,
};

static void santos10_otg_en(bool enable)
{
	int gpio_otg_en = usb_gpios[GPIO_OTG_EN].gpio;

	pr_info("%s %s\n", __func__, enable ? "ON" : "OFF");

	gpio_set_value(gpio_otg_en, enable);
}

static void santos10_accessory_power(u32 device, bool enable)
{
	static u32 acc_device;

	/*
		token info
		0 : power off,
		1 : Keyboard dock
		2 : USB
	*/

	pr_info("accessory_power: acc_device 0x%x, new %d : %s\n",
			acc_device, device, enable ? "ON" : "OFF");

	if (enable) {
		acc_device |= (1 << device);
		otg_control(enable);

	} else {

		if (device == 0) {
			pr_info("accessory_power: force turn off\n");
			otg_control(enable);

		} else {
			acc_device &= ~(1 << device);
			if (acc_device == 0) {
				pr_info("accessory_power: turn off\n");
				otg_control(enable);
			} else
				pr_info("accessory_power: skip\n");
		}
	}
}

static void santos10_set_vbus_drive(bool enable)
{
	struct santos10_otg *santos_otg = &santos10_otg_xceiv;

	if (!host_notify_get_vbusdrive_en(&santos_otg->pdata->ndev)) {
		pr_info("%s vbus drive is disabled. type=%s\n", __func__,
			santos_otg->pdata->ndev.type_name);
		return;
	}

	mutex_lock(&santos_otg->vbus_str.vbus_lock);
#ifdef CONFIG_USB_HOST_NOTIFY
	if (enable)
		host_notify_set_ovc_en
			(&santos_otg->pdata->ndev, NOTIFY_SET_ON);
	else
		host_notify_set_ovc_en
			(&santos_otg->pdata->ndev, NOTIFY_SET_OFF);
	if (santos_otg->pdata->usbhostd_wakeup && enable)
		santos_otg->pdata->usbhostd_wakeup();
#endif
	santos10_accessory_power(2, enable);
	mutex_unlock(&santos_otg->vbus_str.vbus_lock);
}

static void santos10_otg_work(struct work_struct *data)
{
	struct intel_vbus_work *vbus_work =
		container_of(data, struct intel_vbus_work, set_vbus_work);

	pr_info("%s vbus %s\n", __func__, vbus_work->enable ? "on" : "off");
	santos10_set_vbus_drive(vbus_work->enable);
	kfree(vbus_work);
	return;
}

static int santos10_otg_notifier(struct notifier_block *nb,
		unsigned long event, void *param)
{
	struct santos10_otg *santos_otg
		= container_of(nb, struct santos10_otg, otg_nb);
	struct intel_vbus_work *vbus_work;

	if (!param || event != USB_EVENT_DRIVE_VBUS)
		return NOTIFY_DONE;

	pr_info("%s event=%lu\n", __func__, event);
	vbus_work = kmalloc(sizeof(struct intel_vbus_work), GFP_ATOMIC);
	if (!vbus_work)
		return notifier_from_errno(-ENOMEM);
	INIT_WORK(&vbus_work->set_vbus_work, santos10_otg_work);
	vbus_work->enable = *(bool *)param;
	queue_work(santos_otg->vbus_notifier_wq, &vbus_work->set_vbus_work);
	return NOTIFY_OK;
}

static void santos10_otg_register_notifier(struct santos10_otg *santos_otg)
{
	int ret = 0;
	if (santos_otg->otg == NULL) {
		santos_otg->otg = usb_get_transceiver();
		if (!santos_otg->otg) {
			pr_err("failed to get usb_get_transceiver in %s\n",
				__func__);
			return;
		}
		santos_otg->otg_nb.notifier_call = santos10_otg_notifier;
		ret = usb_register_notifier(santos_otg->otg,
						&santos_otg->otg_nb);
		if (ret < 0) {
			usb_put_transceiver(santos_otg->otg);
			santos_otg->otg = NULL;
			pr_err("failed to register to OTG notifications in %s\n",
				__func__);
		}
		pr_info("registered to OTG notifications in %s\n",
			__func__);
	}
}

static void santos10_ap_usb_attach(struct santos10_otg *otg)
{
}

static void santos10_ap_usb_detach(struct santos10_otg *otg)
{
}

static void santos10_usb_host_attach(struct santos10_otg *otg)
{
	pr_info("%s\n", __func__);
	host_notify_set_type(&otg->pdata->ndev, NOTIFY_GENERALHOST);
	santos10_accessory_power(0, 0);
	santos10_otg_register_notifier(otg);
#ifdef CONFIG_USB_HOST_NOTIFY
	if (otg->pdata && otg->pdata->usbhostd_start) {
		host_notify_set_ovc_en
			(&otg->pdata->ndev, NOTIFY_SET_OFF);
		otg->pdata->ndev.mode = NOTIFY_HOST_MODE;
		otg->pdata->usbhostd_start();
	}
#endif
	santos10_otg_en(1);
}

static void santos10_usb_host_detach(struct santos10_otg *otg)
{
	pr_info("%s\n", __func__);
#ifdef CONFIG_USB_HOST_NOTIFY
	if (otg->pdata && otg->pdata->usbhostd_stop) {
		otg->pdata->ndev.mode = NOTIFY_NONE_MODE;
		otg->pdata->usbhostd_stop();
	}
#endif
	santos10_otg_en(0);
}

static void santos10_detected_otg(struct santos10_otg *otg, bool attach)
{
	if (attach)
		santos10_usb_host_attach(otg);
	else
		santos10_usb_host_detach(otg);
}

/*
 * Detects JIG_UART and docks via extcon notifier.
 */

struct jig_notifier_block {
	struct notifier_block nfb;
	struct pci_dev *pdev;
	bool forbid;
	bool attached;
};

static int jigcable_pm_callback(struct notifier_block *nfb,
		unsigned long action, void *ignored)
{
	struct jig_notifier_block *jig_nfb =
		(struct jig_notifier_block *)nfb;
	struct pci_dev *pdev = jig_nfb->pdev;

	if (!pdev)
		return NOTIFY_DONE;

	pr_debug("(%s): action=%ld\n", __func__, action);
	switch (action) {
	case PM_SUSPEND_PREPARE:
		if (jig_nfb->forbid) {
			pm_runtime_allow(&pdev->dev);
			jig_nfb->forbid = false;
		}
		return NOTIFY_OK;
	case PM_POST_SUSPEND:
		if (!jig_nfb->forbid) {
			pm_runtime_forbid(&pdev->dev);
			jig_nfb->forbid = true;
		}
		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static struct jig_notifier_block jigcable_pm_notifier = {
	.nfb = {
		.notifier_call = jigcable_pm_callback,
		.priority = 0,
	},
};

struct sec_santos10_cable {
	struct work_struct work;
	struct notifier_block nb;
	struct extcon_specific_cable_nb extcon_nb;
	struct extcon_dev *edev;
	enum extcon_cable_name cable_type;
	int cable_state;
};

static struct sec_santos10_cable support_cable_list[] = {
	{ .cable_type = EXTCON_JIG_UARTOFF, },
	{ .cable_type = EXTCON_JIG_UARTON, },
	{ .cable_type = EXTCON_JIG_USBOFF, },
	{ .cable_type = EXTCON_JIG_USBON, },
	{ .cable_type = EXTCON_USB_HOST, },

	{ .cable_type = EXTCON_DESKDOCK, },
	{ .cable_type = EXTCON_CARDOCK, },
	{ .cable_type = EXTCON_AUDIODOCK, },
	{ .cable_type = EXTCON_SMARTDOCK, },
};

static void sec_santos10_cable_event_worker(struct work_struct *work)
{
	struct sec_santos10_cable *cable =
			    container_of(work, struct sec_santos10_cable, work);
	struct santos10_otg *otg = &santos10_otg_xceiv;

	pr_info("%s: '%s' is %s\n", __func__,
			extcon_cable_name[cable->cable_type],
			cable->cable_state ? "attached" : "detached");

	switch (cable->cable_type) {

	case EXTCON_JIG_USBOFF:
	case EXTCON_JIG_USBON:
		break;
	case EXTCON_USB_HOST:
		santos10_detected_otg(otg, cable->cable_state);
		break;

	case EXTCON_DESKDOCK:
		if (cable->cable_state)
			switch_set_state(&otg->dock_switch, SANTOS10_DOCK_DESK);
		else
			switch_set_state(&otg->dock_switch, SANTOS10_DOCK_NONE);
		break;
	case EXTCON_CARDOCK:
		if (cable->cable_state)
			switch_set_state(&otg->dock_switch, SANTOS10_DOCK_CAR);
		else
			switch_set_state(&otg->dock_switch, SANTOS10_DOCK_NONE);

		break;
	case EXTCON_AUDIODOCK:
	case EXTCON_SMARTDOCK:
		pr_debug("11pin tablet doesn't support audio/smart dock\n");
		break;
	case EXTCON_JIG_UARTOFF:
	case EXTCON_JIG_UARTON:
		if (cable->cable_state) {
			if (!jigcable_pm_notifier.forbid) {
				pm_runtime_forbid(
					&jigcable_pm_notifier.pdev->dev);
				jigcable_pm_notifier.forbid = true;
			}
			if (!jigcable_pm_notifier.attached) {
				jigcable_pm_notifier.attached = true;
				register_pm_notifier(
						(struct notifier_block *)&jigcable_pm_notifier);
			}
		}
		else {
			if (jigcable_pm_notifier.forbid) {
				pm_runtime_allow(
					&jigcable_pm_notifier.pdev->dev);
				jigcable_pm_notifier.forbid = false;
			}
			if (jigcable_pm_notifier.attached) {
				jigcable_pm_notifier.attached = false;
				unregister_pm_notifier(
						(struct notifier_block *)&jigcable_pm_notifier);
			}
		}
		break;
	default:
		pr_err("%s: invalid cable value (%d, %d)\n", __func__,
					cable->cable_type, cable->cable_state);
		break;
	}
}


static int sec_santos10_cable_notifier(struct notifier_block *nb,
					unsigned long stat, void *ptr)
{
	struct sec_santos10_cable *cable =
			container_of(nb, struct sec_santos10_cable, nb);

	cable->cable_state = stat;
	schedule_work(&cable->work);

	return NOTIFY_DONE;
}


static void sec_santos10_init_cable_notify(void)
{
	struct sec_santos10_cable *cable;
	int i;
	int ret;

	pr_info("register extcon notifier for JIG and docks\n");
	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];

		INIT_WORK(&cable->work, sec_santos10_cable_event_worker);
		cable->nb.notifier_call = sec_santos10_cable_notifier;

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

	jigcable_pm_notifier.pdev =
		pci_get_device(PCI_VENDOR_ID_INTEL, 0x08FE, NULL);
}

#ifdef CONFIG_USB_HOST_NOTIFY
static void santos10_booster(int enable)
{
	santos10_set_vbus_drive(!!enable);
}

static struct host_notifier_platform_data host_notifier_pdata = {
	.ndev.name	= "usb_otg",
	.booster	= santos10_booster,
	.thread_enable = 0,
};

static struct platform_device host_notifier_device = {
	.name = "host_notifier",
	.dev.platform_data = &host_notifier_pdata,
};

static void santos10_host_notifier_init(struct santos10_otg *otg)
{
	int acc_out =
		get_gpio_by_name("V_BUS");

	if (unlikely(acc_out == -1))
		pr_err("%s V_BUS is invalid.\n", __func__);

	host_notifier_pdata.gpio = acc_out;
	otg->pdata = &host_notifier_pdata;

	platform_device_register(&host_notifier_device);
}
#endif

static void santos10_usb_gpio_init(void)
{
	int i;
	int ret;

	pr_info("%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(usb_gpios); i++) {
		usb_gpios[i].gpio =
			get_gpio_by_name(usb_gpios[i].label);
		ret = gpio_request_one(usb_gpios[i].gpio,
			usb_gpios[i].flags, usb_gpios[i].label);
		if (ret)
			pr_err("%s %s error gpio=%d ret=%d\n", __func__
				, usb_gpios[i].label, usb_gpios[i].gpio, ret);
	}
}

/* device_initcall */
int __init sec_santos10_usb_init(void)
{
	struct santos10_otg *santos_otg = &santos10_otg_xceiv;
	int ret;

	santos_otg->vbus_notifier_wq
		= create_singlethread_workqueue("clt-vbus");
	mutex_init(&santos_otg->vbus_str.vbus_lock);

	/* sysfs */
	santos_otg->switch_dev =
			device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(santos_otg->switch_dev)) {
		pr_err("(%s): failed to created device (switch_dev)!\n",
								__func__);
		ret = -ENODEV;
		goto err_switch_dev;
	}

	dev_set_drvdata(santos_otg->switch_dev, santos_otg);
	ret = sysfs_create_group(&santos_otg->switch_dev->kobj,
						&manual_switch_sel_group);
	if (ret < 0) {
		pr_err("(%s): fail to  create switch_sel sysfs group (%d)\n",
								__func__, ret);
		goto err_sysfs;
	}

	santos_otg->dock_switch.name = "dock";
	switch_dev_register(&santos_otg->dock_switch);

#ifdef CONFIG_USB_HOST_NOTIFY
	santos10_host_notifier_init(santos_otg);
#endif
	santos10_usb_gpio_init();

	return 0;

err_sysfs:
	device_unregister(santos_otg->switch_dev);
err_switch_dev:
	mutex_destroy(&santos_otg->vbus_str.vbus_lock);

	return ret;
}

/* device_initcall_sync */
int __init sec_santos10_max77693_gpio_init(void)
{
	int gpio_irq;
	int ret;

	gpio_irq = get_gpio_by_name("IF_PMIC_IRQ");
	if (unlikely(gpio_irq < 0)) {
		pr_err("%s: fail to get IF_PMIC_IRQ gpio (%d)\n",
						__func__, gpio_irq);
		return -EINVAL;
	}

	ret = gpio_request(gpio_irq, "IF_PMIC_IRQ");
	if (unlikely(ret)) {
		pr_err("%s: failed requesting gpio(%d)\n", __func__, gpio_irq);
		return ret;
	}
	gpio_direction_input(gpio_irq);

	pr_info("%s: gpio for max77693 irq = %d\n", __func__, gpio_irq);

	sec_santos10_init_cable_notify();

	return 0;
}
