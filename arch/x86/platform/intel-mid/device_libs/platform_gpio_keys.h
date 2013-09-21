/*
 * platform_gpio_keys.h: gpio_keys platform data header file
 *
 * (C) Copyright 2008 Intel Corporation
 * Author:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */
#ifndef _PLATFORM_GPIO_KEYS_H_
#define _PLATFORM_GPIO_KEYS_H_

#define DEVICE_NAME "gpio-keys"

void intel_mid_customize_gpio_keys(struct gpio_keys_button *button,
		size_t num) __attribute__((weak));

#endif
