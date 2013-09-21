/* drivers/platform/x86/intel_scu_flis_sec.c
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <asm/intel_scu_ipcutil.h>
#include <asm/intel_scu_flis.h>
#include <asm/intel_muxtbl.h>
#include <asm/sec_muxtbl.h>

#ifdef CONFIG_INTEL_SCU_FLIS_SEC_DEBUG

#define FLIS_DEBUG_MSG_LEN		128
#define UNKNOWN				0xFF

struct flis_debug_info {
	u8 mask;
	char *cfg_name;
};

#define FLIS_DEBUG_INFO(_MASK)						\
{									\
	.mask = _MASK,							\
	.cfg_name = #_MASK,						\
}

static char __init *__strcat_pull(char *msg, u8 pull)
{
	struct flis_debug_info dbg_pull[] = {
		FLIS_DEBUG_INFO(DOWN_20K),
		FLIS_DEBUG_INFO(DOWN_2K),
		/* FLIS_DEBUG_INFO(DOWN_72K), is reserved */
		FLIS_DEBUG_INFO(UP_20K),
		FLIS_DEBUG_INFO(UP_2K),
		FLIS_DEBUG_INFO(UP_910),
	};
	char tmp[FLIS_DEBUG_MSG_LEN];
	int i, n;


	if (pull == NONE) {
		sprintf(tmp, "%s(0x%02X)\t", "NONE", 0);
		return strcat(msg, tmp);
	}

	for (i = 0, n = 0; i < ARRAY_SIZE(dbg_pull); i++) {
		if (pull & dbg_pull[i].mask) {
			if (n > 0)
				n += sprintf(&tmp[n], " | ");
			n += sprintf(&tmp[n], "%s", dbg_pull[i].cfg_name);
		}
	}
	sprintf(&tmp[n], " (0x%02X)\t", pull);

	return strcat(msg, tmp);
}

static char __init *__strcat_dir(char *msg, u8 dir)
{
	struct flis_debug_info dbg_dir[] = {
		FLIS_DEBUG_INFO(MUX_EN_INPUT_EN),
		FLIS_DEBUG_INFO(INPUT_EN),
		FLIS_DEBUG_INFO(MUX_EN_OUTPUT_EN),
		FLIS_DEBUG_INFO(OUTPUT_EN),
	};
	char tmp[FLIS_DEBUG_MSG_LEN];
	int i, n;

	if (dir == 0) {
		sprintf(tmp, "BYPASS (0x%02X)\t", 0);
		return strcat(msg, tmp);
	}

	for (i = 0, n = 0; i < ARRAY_SIZE(dbg_dir); i++) {
		if (dir & dbg_dir[i].mask) {
			if (n > 0)
				n += sprintf(&tmp[n], " | ");
			n += sprintf(&tmp[n], "%s", dbg_dir[i].cfg_name);
		}
	}
	sprintf(&tmp[n], "(0x%02X)\t", dir);

	return strcat(msg, tmp);
}

static char __init *__strcat_open_drain(char *msg, u8 open_drain)
{
	struct flis_debug_info dbg_open_drain[] = {
		FLIS_DEBUG_INFO(OD_ENABLE),
		FLIS_DEBUG_INFO(OD_DISABLE),
	};
	char tmp[FLIS_DEBUG_MSG_LEN];

	if (open_drain == dbg_open_drain[0].mask)
		sprintf(tmp, "%s (0x%02X)\t", dbg_open_drain[0].cfg_name,
				open_drain);
	else if (open_drain == dbg_open_drain[1].mask)
		sprintf(tmp, "%s (0x%02X)\t", dbg_open_drain[1].cfg_name,
				open_drain);

	return strcat(msg, tmp);
}

static void __init intel_flis_muxtbl_print_debug(struct intel_muxtbl *muxtbl,
		const u8 *cfg_old, const u8 *cfg_new)
{
	char msg[FLIS_DEBUG_MSG_LEN];

	/* do NOT print any if configuration is same */
	if (cfg_old[0] == cfg_new[0] && cfg_old[1] == cfg_new[1] &&
	    cfg_old[2] == cfg_new[2])
		return;

	sprintf(msg, "-old %-16.16s(%03d) : ",
			muxtbl->gpio.label, muxtbl->gpio.gpio);
	__strcat_dir(msg, cfg_old[MUX]);
	__strcat_open_drain(msg, cfg_old[OPEN_DRAIN]);
	__strcat_pull(msg, cfg_old[PULL]);
	pr_info("%s\n", msg);

	sprintf(msg, "+new %-16.16s(%03d) : ",
			muxtbl->gpio.label, muxtbl->gpio.gpio);
	__strcat_dir(msg, cfg_new[MUX]);
	__strcat_open_drain(msg, cfg_new[OPEN_DRAIN]);
	__strcat_pull(msg, cfg_new[PULL]);
	pr_info("%s\n", msg);
}
#else /* CONFIG_INTEL_SCU_FLIS_SEC_DEBUG */
#define intel_flis_muxtbl_print_debug(muxtbl, cfg_old, cfg_new)
#endif /* CONFIG_INTEL_SCU_FLIS_SEC_DEBUG */

static int __init intel_flis_muxtbl_iterator(struct intel_muxtbl *muxtbl,
		void *private)
{
	u8 cfg_old[3];	/* PULL / MUX / OPEN_DRAIN */
	u8 cfg_new[3];
	int ret = 0;
	int update[3] = { 0, 0, 0 };
	int *err = (int *)private;
	int i;
	unsigned int retry;
	const unsigned int retry_max = 2;

	if (muxtbl->mux.pinname == INTEL_MUXTBL_NO_CTRL)
		return 0;

	cfg_new[PULL] = muxtbl->mux.pull;
	cfg_new[MUX] = muxtbl->mux.pin_direction;
	cfg_new[OPEN_DRAIN] = muxtbl->mux.open_drain;

	/* read the current configuration */
	for (i = 0; i < ARRAY_SIZE(cfg_old); i++) {
		if (get_pin_flis(muxtbl->mux.pinname, i, &cfg_old[i])) {
			pr_err("(%s): (R) %s(%d) - %d(%d)\n", __func__,
					muxtbl->gpio.label,
					muxtbl->gpio.gpio,
					muxtbl->mux.pinname, i);
			cfg_old[i] = cfg_new[i];
			update[i] = -1;
			*err -= 1;
		}
		ret += update[i];
	}

	if (ret == -ARRAY_SIZE(update))
		return 0;	/* nothing to do */

	for (i = 0; i < ARRAY_SIZE(cfg_old); i++) {
		/* we don't need to update if alread configure with
		 * same value or fail to read the current value*/
		if (cfg_old[i] == cfg_new[i] || update[i])
			continue;

		ret = config_pin_flis(muxtbl->mux.pinname, i, cfg_new[i]);
		if (likely(!ret))
			continue;
		usleep_range(4500, 5500);	/* about 5msec delay */

		retry = 0;
		do {
			ret = config_pin_flis(muxtbl->mux.pinname, i,
					cfg_new[i]);
			if (likely(!ret))
				break;
			usleep_range(4500, 5500);
		} while (++retry < retry_max);

		if (unlikely(ret)) {
			pr_err("(%s): (W) %s(%d) - %d(%d)\n", __func__,
					muxtbl->gpio.label,
					muxtbl->gpio.gpio,
					muxtbl->mux.pinname, i);
			*err -= 1;
		}
	}

	intel_flis_muxtbl_print_debug(muxtbl, cfg_old, cfg_new);

	return 0;
}

static int __init intel_flis_sec(void)
{
	int err = 0;

	intel_muxtbl_run_iterator(intel_flis_muxtbl_iterator, &err);

	if (unlikely(err < 0))
		pr_warning("(%s): error is occurred while setting FLIS!\n",
				__func__);

	return 0;
}
rootfs_initcall(intel_flis_sec);
