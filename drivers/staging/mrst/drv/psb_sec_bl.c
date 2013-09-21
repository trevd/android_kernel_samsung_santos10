/* pbs backlight for sec device
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

#include <linux/backlight.h>
#include <linux/version.h>
#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_powermgmt.h"
#include "mdfld_dsi_dbi.h"
#include <linux/delay.h>
#include <video/psb_sec_bl.h>

#define BRIGHTNESS_INIT_LEVEL		-1
#define BRIGHTNESS_MAX_LEVEL		255
#define BRIGHTNESS_MIN_LEVEL		1
#define BLC_ADJUSTMENT_MAX		100

enum auto_brt_val {
	AUTO_BRIGHTNESS_MANUAL = 0,
	AUTO_BRIGHTNESS_VAL1,
	AUTO_BRIGHTNESS_VAL2,
	AUTO_BRIGHTNESS_VAL3,
	AUTO_BRIGHTNESS_VAL4,
	AUTO_BRIGHTNESS_VAL5,
	MAX_AUTO_BRIGHTNESS,
};
enum auto_brt_val auto_brightness;

int psb_brightness;
static int psb_backlight_enabled;
static struct backlight_device *psb_backlight_device;
static struct drm_device *drm_dev;

int lastFailedBrightness;


int psb_set_brightness(struct backlight_device *bd)
{
	unsigned int level;
	struct psb_backlight_platform_data *pdata;

	if (unlikely(!bd)) {
		pr_err("%s: ERR: backlight device is NULL\n", __func__);
		return -EINVAL;
	}
	level = bd->props.brightness;
	dev_info(&bd->dev, "brightness = %d\n", level);

	if (unlikely(level < 0)) {
		dev_info(&bd->dev, "brightness(=%d) is less than 0, return.\n",
				level);
		return -EINVAL;
	}

	pdata = dev_get_drvdata(&bd->dev);
	if (unlikely(!pdata)) {
		dev_err(&bd->dev, "failed to find backlight data\n");
		return -EINVAL;
	}

	if (level > 0) {
		if (psb_backlight_enabled == 0) {
			pdata->backlight_power(1);
			mdelay(1);
		}

		if (level < BRIGHTNESS_MIN_LEVEL)
			level = BRIGHTNESS_MIN_LEVEL;

		if (pdata->backlight_set_pwm(level)) {
			psb_brightness = level;
			lastFailedBrightness = true;
			return -1;
		}

	} else {
		pdata->backlight_set_pwm(level);
		pdata->backlight_power(0);
	}
	lastFailedBrightness = false;
	psb_backlight_enabled = !!level;
	psb_brightness = level;
	return 0;
}

int psb_get_brightness(struct backlight_device *bd)
{
	DRM_DEBUG_DRIVER("brightness = 0x%x\n", psb_brightness);
	/* return locally cached var instead of HW read (due to DPST etc.) */
	return psb_brightness;
}

const struct backlight_ops psb_ops = {
	.get_brightness = psb_get_brightness,
	.update_status  = psb_set_brightness,
};

static ssize_t auto_brightness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct psb_backlight_platform_data *pdata = dev_get_drvdata(dev);

	pr_info("%s: auto_brightness=%d\n", __func__, auto_brightness);

	return sprintf(buf, "%d\n", auto_brightness);
}

static ssize_t auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct psb_backlight_platform_data *pdata = dev_get_drvdata(dev);
	int value;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (unlikely(ret < 0))
		return -EINVAL;

	if (value > MAX_AUTO_BRIGHTNESS || value < AUTO_BRIGHTNESS_MANUAL) {
		pr_err("%s: invalid input value(%s)\n", __func__, buf);
		return count;
	}

	dev_info(dev, "%s: value = %d\n", __func__, value);

	if (value == auto_brightness)
		return count;

	auto_brightness = value;

	if (pdata->set_auto_brt)
		pdata->set_auto_brt(value);

	psb_set_brightness(psb_backlight_device);

	return count;
}

static DEVICE_ATTR(auto_brightness, S_IRUGO|S_IWUSR|S_IWGRP,
				auto_brightness_show, auto_brightness_store);

static struct attribute *psb_bl_attributes[] = {
	&dev_attr_auto_brightness.attr,
	NULL,
};

static const struct attribute_group psb_bl_group = {
	.attrs  = psb_bl_attributes,
};

static int device_backlight_init(struct drm_device *dev)
{
	unsigned long CoreClock;
	/* u32 bl_max_freq; */
	/* unsigned long value; */
	u16 bl_max_freq;
	uint32_t value;
	uint32_t blc_pwm_precision_factor;
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;

	dev_priv->blc_adj1 = BLC_ADJUSTMENT_MAX;
	dev_priv->blc_adj2 = BLC_ADJUSTMENT_MAX * 100;
	return 0;
}

static int psb_backlight_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct backlight_properties props;
	struct psb_backlight_platform_data *pdata =
		(struct psb_backlight_platform_data *)pdev->dev.platform_data;

	if (!pdata) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_PLATFORM;
	props.max_brightness = BRIGHTNESS_MAX_LEVEL;

	psb_backlight_device =
			backlight_device_register("panel", NULL,
					(void *)pdata, &psb_ops, &props);

	if (IS_ERR(psb_backlight_device))
		return PTR_ERR(psb_backlight_device);

	ret = sysfs_create_group(&psb_backlight_device->dev.kobj,
							&psb_bl_group);
	if (unlikely(ret < 0)) {
		dev_warn(&pdev->dev, "failed to create sysfs\n");
		goto err_sysfs;
	}

	if (!drm_dev) {
		dev_err(&pdev->dev, "drm_dev is not initialized\n");
		goto err_init;

	}
	ret = device_backlight_init(drm_dev);
	if (ret != 0)
		goto err_init;

	psb_backlight_device->props.brightness = BRIGHTNESS_INIT_LEVEL;
	psb_backlight_device->props.max_brightness = BRIGHTNESS_MAX_LEVEL;

	return 0;

err_init:
	sysfs_remove_group(&psb_backlight_device->dev.kobj, &psb_bl_group);
err_sysfs:
	backlight_device_unregister(psb_backlight_device);

	return ret;
}

static int psb_backlight_remove(struct platform_device *pdev)
{
	psb_backlight_device->props.brightness = 0;
	backlight_update_status(psb_backlight_device);
	backlight_device_unregister(psb_backlight_device);

	return 0;
}

static struct platform_driver psb_backlight_driver = {
	.probe		= psb_backlight_probe,
	.remove         = psb_backlight_remove,
	.driver         = {
		.name   = "psb_backlight",
		.owner  = THIS_MODULE,
	},
};

int psb_backlight_init(struct drm_device *dev)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	drm_dev = dev;
#endif
	return 0;
}

void psb_backlight_exit(void)
{
	return;
}

struct backlight_device *psb_get_backlight_device(void)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	return psb_backlight_device;
#endif
	return NULL;
}

static int __init psb_backlight_sec_init(void)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	return platform_driver_register(&psb_backlight_driver);
#endif
	return 0;
}


module_init(psb_backlight_sec_init);

