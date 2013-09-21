/*
 * INTEL MID PINCTRL subsystem Head File
 *
 * Copyright (C) 2013 Intel, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _ASM_X86_INTEL_MID_PINCTRL_H_
#define _ASM_X86_INTEL_MID_PINCTRL_H_

struct intel_mid_pinctrl_platform_data {
	char name[16];
	struct pinstruct_t *pin_t;
	int pin_num;
};

enum platform_descs {
	ctp_pin_desc,
	mrfl_pin_desc,
};

#endif


