/* arch/x86/platform/intel-mid/sec_libs/sec_log_buf.c
 *
 * Copyright (C) 2010-2013 Samsung Electronics Co, Ltd.
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

#include <linux/bootmem.h>
#include <linux/console.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <asm-generic/sizes.h>
#include <asm/pat.h>

#include "sec_common.h"
#include "sec_debug.h"
#include "sec_getlog.h"
#include "sec_log_buf.h"

static unsigned s_log_buf_msk;
static struct sec_log_buf s_log_buf;
static struct device *sec_log_dev;

#define sec_log_buf_get_log_end(_idx)	\
	((char *)(s_log_buf.data + ((_idx) & s_log_buf_msk)))

static void sec_log_buf_write(struct console *console, const char *s,
			      unsigned int count)
{
	unsigned int f_len, s_len, remain_space;
	unsigned int buf_pos = ((*s_log_buf.count) & s_log_buf_msk);
	if (unlikely(!s_log_buf.enable)) {
		pr_info("%s", s);
		return;
	}

	remain_space = s_log_buf_msk + 1 - buf_pos;
	f_len = min(count, remain_space);
	memcpy(s_log_buf.data + buf_pos , s, f_len);

	s_len = count - f_len;

	if (unlikely(s_len))
		memcpy(s_log_buf.data , s + f_len, s_len);

	(*s_log_buf.count) += count;
}

static struct console sec_console = {
	.name	= "sec_log_buf",
	.write	= sec_log_buf_write,
	.flags	= CON_PRINTBUFFER | CON_ENABLED | CON_ANYTIME,
	.index	= -1,
};

static ssize_t sec_log_buf_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%p(flag) - 0x%08x(count) - 0x%p(data)\n",
		       s_log_buf.flag, *s_log_buf.count, s_log_buf.data);
}

static DEVICE_ATTR(log, S_IRUGO, sec_log_buf_show, NULL);

static unsigned int sec_log_buf_start;
static unsigned int sec_log_buf_size;
static const unsigned int sec_log_buf_flag_size = SZ_4K;
static const unsigned int sec_log_buf_magic = 0x404C4F47;	/* @LOG */

#ifdef CONFIG_SAMSUNG_USE_LAST_SEC_LOG_BUF
static char *last_log_buf;
static unsigned int last_log_buf_size;

static void __init sec_last_log_buf_setup(void)
{
	unsigned int max_size = s_log_buf_msk + 1;
	unsigned int head;

	last_log_buf = vmalloc(s_log_buf_msk + 1);

	if (*s_log_buf.count > max_size) {
		head = *s_log_buf.count & s_log_buf_msk;
		memcpy(last_log_buf, s_log_buf.data + head, max_size - head);
		if (head != 0)
			memcpy(last_log_buf + max_size - head,
			       s_log_buf.data, head);
		last_log_buf_size = max_size;
	} else {
		memcpy(last_log_buf, s_log_buf.data, *s_log_buf.count);
		last_log_buf_size = *s_log_buf.count;
	}
}

static ssize_t sec_last_log_buf_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_log_buf_size || !last_log_buf)
		return 0;

	count = min(len, (size_t) (last_log_buf_size - pos));
	if (copy_to_user(buf, last_log_buf + pos, count))
		return -EFAULT;

	*offset += count;

	return count;
}

static const struct file_operations last_log_buf_fops = {
	.owner	= THIS_MODULE,
	.read	= sec_last_log_buf_read,
};

#if defined(CONFIG_SAMSUNG_REPLACE_LAST_KMSG)
#define LAST_LOG_BUF_NODE		"last_kmsg"
#else
#define LAST_LOG_BUF_NODE		"last_sec_log_buf"
#endif

static int __init sec_last_log_buf_init(void)
{
	struct proc_dir_entry *entry;

	if (!last_log_buf)
		return 0;

	entry = create_proc_entry(LAST_LOG_BUF_NODE, S_IFREG | S_IRUGO, NULL);
	if (!entry) {
		pr_err("(%s): failed to create proc entry\n", __func__);
		return 0;
	}

	entry->proc_fops = &last_log_buf_fops;
	entry->size = last_log_buf_size;

	return 0;
}

late_initcall(sec_last_log_buf_init);

#else /* CONFIG_SAMSUNG_USE_LAST_SEC_LOG_BUF */
#define sec_last_log_buf_setup()
#endif /* CONFIG_SAMSUNG_USE_LAST_SEC_LOG_BUF */

static int __init sec_log_buf_setup_early(char *str)
{
	unsigned long res;

	sec_log_buf_size = memparse(str, &str);

	memset(&s_log_buf, 0x00, sizeof(struct sec_log_buf));

	if (sec_log_buf_size && (*str == '@')) {
		if (kstrtoul(++str, 16, &res))
			goto __err;
		sec_log_buf_start = res;
	}

	return 0;

__err:
	sec_log_buf_start = 0;
	sec_log_buf_size = 0;
	return 0;
}

/* 1st handler for 'sec_log' command-line option */
early_param("sec_log", sec_log_buf_setup_early);

static int __init sec_log_buf_reserve_late(char *str)
{
	/* TODO: we don't need to parse 'str' variable because it is already
	 * translated in early_param */
	if (unlikely(!sec_log_buf_start || !sec_log_buf_size))
		return 0;

	/* call memblock_remove to use ioremap */
	if (memblock_remove(sec_log_buf_start, sec_log_buf_size)) {
		pr_err("(%s): failed to remove size %d@0x%x\n",
		       __func__, sec_log_buf_size / 1024, sec_log_buf_start);
		goto __err_memblock_remove;
	}
	s_log_buf_msk = sec_log_buf_size - sec_log_buf_flag_size - 1;

	return 0;

__err_memblock_remove:
	sec_log_buf_start = 0;
	sec_log_buf_size = 0;
	return 0;
}

/* 2nd handler for 'sec_log' command-line option */
__setup("sec_log=", sec_log_buf_reserve_late);

static void __init sec_log_buf_create_sysfs(void)
{
	sec_log_dev = device_create(sec_class, NULL, 0, NULL, "sec_log");
	if (IS_ERR(sec_log_dev))
		pr_err("(%s): failed to create device(sec_log)!\n", __func__);

	if (device_create_file(sec_log_dev, &dev_attr_log))
		pr_err("(%s): failed to create device file(log)!\n", __func__);
}

#if defined(CONFIG_ARCH_OMAP) && defined(CONFIG_SAMSUNG_SIABLE_SEC_LOG_WHEN_TTY)
static bool __init sec_log_buf_disable(void)
{
	char console_opt[32];
	int i;

	/* using ttyOx means, developer is watching kernel logs through out
	 * the uart-serial. at this time, we don't need sec_log_buf to save
	 * serial log and __log_buf is enough to analyze kernel log.
	 */
	for (i = 0; i < OMAP_MAX_HSUART_PORTS; i++) {
		sprintf(console_opt, "console=%s%d", OMAP_SERIAL_NAME, i);
		if (cmdline_find_option(console_opt))
			return true;
	}

	return false;
}
#else
#define sec_log_buf_disable()		false
#endif

static void ramoops_do_dump(struct kmsg_dumper *dumper,
		enum kmsg_dump_reason reason, const char *s1,
		unsigned long l1, const char *s2, unsigned long l2)
{
	unsigned long flags;
	unsigned long record_size = SZ_64K;
	char oops_buf[64];
	struct timeval timestamp;
	unsigned long s1_start, s2_start;
	unsigned long l1_cpy, l2_cpy;

	if (reason != KMSG_DUMP_OOPS && reason != KMSG_DUMP_PANIC)
		return;

	/* Only dump oopses */
	if (reason == KMSG_DUMP_OOPS)
		return;

	if (!is_console_locked())
		return;

	spin_lock_irqsave(&s_log_buf.dump_lock, flags);
	do_gettimeofday(&timestamp);

	/*show ramoops information */
	memset(oops_buf, 0x0, 64);
	sprintf(oops_buf,
		"///////////////////////////////////////////////////////\n");
	sec_log_buf_write(NULL, oops_buf, strlen(oops_buf));

	memset(oops_buf, 0x0, 64);
	sprintf(oops_buf, "-- [ SEC RAMOOPS reason(%d): %lu.%lu  ] --\n",
		reason, (long)timestamp.tv_sec, (long)timestamp.tv_usec);
	sec_log_buf_write(NULL, oops_buf, strlen(oops_buf));

	memset(oops_buf, 0x0, 64);
	sprintf(oops_buf,
		"///////////////////////////////////////////////////////\n");
	sec_log_buf_write(NULL, oops_buf, strlen(oops_buf));

	l2_cpy = min(l2, record_size);
	l1_cpy = min(l1, record_size - l2_cpy);

	s2_start = l2 - l2_cpy;
	s1_start = l1 - l1_cpy;

	sec_log_buf_write(NULL, s1 + s1_start, l1_cpy);
	sec_log_buf_write(NULL, s2 + s2_start, l2_cpy);

	memset(oops_buf, 0x0, 64);
	sprintf(oops_buf,
		"///////////////////////////////////////////////////////\n");
	sec_log_buf_write(NULL, oops_buf, strlen(oops_buf));

	spin_unlock_irqrestore(&s_log_buf.dump_lock, flags);
}

static int __init sec_log_buf_init(void)
{
	void *start;
	int err = 0;
	unsigned long want_flags = 0x0;
	int tmp_console_loglevel =
	    (console_loglevel > CONFIG_DEFAULT_MESSAGE_LOGLEVEL) ?
			console_loglevel : CONFIG_DEFAULT_MESSAGE_LOGLEVEL;

	if (unlikely(sec_log_buf_disable())) {
		s_log_buf.enable = false;
		return 0;
	} else
		s_log_buf.enable = true;

	if (unlikely(!sec_log_buf_start || !sec_log_buf_size))
		return 0;

	start = phys_to_virt(sec_log_buf_start);
	s_log_buf.count = (unsigned int *)(start + 4);

	s_log_buf.flag = (unsigned int *)start;
	s_log_buf.data = (char *)(start + sec_log_buf_flag_size);
	s_log_buf.phys_data = sec_log_buf_start + sec_log_buf_flag_size;


	/*set uncached*/
	want_flags = pgprot_val(PAGE_KERNEL_IO_NOCACHE);
	kernel_map_sync_memtype(sec_log_buf_start, sec_log_buf_size, want_flags);

	sec_last_log_buf_setup();

	if (sec_debug_get_level())
		tmp_console_loglevel = 7;	/* KERN_DEBUG */

	if (console_loglevel < tmp_console_loglevel)
		console_loglevel = tmp_console_loglevel;

	register_console(&sec_console);
	sec_log_buf_create_sysfs();

	spin_lock_init(&s_log_buf.dump_lock);
	s_log_buf.dump.dump = ramoops_do_dump;
	err = kmsg_dump_register(&s_log_buf.dump);
	if (err)
		pr_err("registering kmsg dumper failed\n");

	return 0;
}
arch_initcall(sec_log_buf_init);

static int __init default_sec_log_buf_reserve(void)
{
	phys_addr_t mem = 0x10000000;
	size_t size = SZ_2M + SZ_4K;

	if (unlikely(sec_log_buf_start && sec_log_buf_size))
		return 0;

	memblock_remove(mem, mem + size);

	sec_log_buf_start = 0x10000000;
	sec_log_buf_size = size;
	s_log_buf_msk = sec_log_buf_size - sec_log_buf_flag_size - 1;

	pr_info("[%s] SEC LOG BUF start=0x%llx size=0x%x\n", __func__,
			(unsigned long long)mem, size);

	return 0;
}
core_initcall(default_sec_log_buf_reserve);
