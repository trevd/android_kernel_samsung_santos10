/* arch/x86/platform/intel-mid/sec_libs/sec_xen_console.c
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

#include <linux/console.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <asm-generic/sizes.h>
#include <asm/pat.h>
#include <linux/debugfs.h>

#include <xen/interface/xen.h>
#include <xen/interface/vcpu.h>

#include <asm/xen/interface.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/interface.h>
#include <xen/xen.h>
#include <xen/page.h>
#include <xen/events.h>

#include "sec_common.h"
#include "sec_debug.h"
#include "sec_getlog.h"
#include "sec_xen_console.h"

#define __HYPERVISOR_sysctl		35

#define XEN_SYSCTL_readconsole		1

#define XC__HYPERCALL_BUFFER_NAME(_name)	xc__hypercall_buffer_##_name

#define XEN_SYSCTL_INTERFACE_VERSION	0x00000008

#define HYPERCALL_BUFFER(_name)						\
({  struct xc_hypercall_buffer _val1;					\
	typeof(XC__HYPERCALL_BUFFER_NAME(_name)) *_val2 = 		\
			&XC__HYPERCALL_BUFFER_NAME(_name);		\
	(void)(&_val1 == _val2);					\
	(_val2)->param_shadow ? (_val2)->param_shadow : (_val2);	\
})

/*
 * Get the hypercall buffer data pointer in a form suitable for use
 * directly as a hypercall argument.
 */
#define HYPERCALL_BUFFER_AS_ARG(_name)					\
({  struct xc_hypercall_buffer _val1;					\
	typeof(XC__HYPERCALL_BUFFER_NAME(_name)) *_val2 =		\
			HYPERCALL_BUFFER(_name);			\
	(void)(&_val1 == _val2);					\
	(unsigned long)(_val2)->sbuf;					\
})

/*
 * Declare a named bounce buffer.
 *
 * Normally you should use DECLARE_HYPERCALL_BOUNCE (see below).
 *
 * This declaration should only be used when the user pointer is
 * non-trivial, e.g. when it is contained within an existing data
 * structure.
 */
#define DECLARE_NAMED_HYPERCALL_BOUNCE(_name, _ubuf, _sz, _dir)		\
struct xc_hypercall_buffer XC__HYPERCALL_BUFFER_NAME(_name) = {		\
	.sbuf = _ubuf,							\
	.param_shadow = NULL,						\
	.sz = _sz, .dir = _dir,						\
}

/*
 * Declare a bounce buffer shadowing the named user data pointer.
 */
#define DECLARE_HYPERCALL_BOUNCE(_ubuf, _sz, _dir)			\
	DECLARE_NAMED_HYPERCALL_BOUNCE(_ubuf, _ubuf, _sz, _dir)

#undef set_xen_guest_handle
#define set_xen_guest_handle(hnd, val)					\
do { if ( sizeof(hnd) == 8 ) *(uint64_t *)&(hnd) = 0;			\
	(hnd).p = val;							\
} while (0)

struct xen_readconsole {
	char *xen_buffer;
	char *print_buffer;
	struct work_struct work;
};

static struct xen_readconsole xen_console;

static struct dentry *g_xen_dir;
static struct dentry *g_xen_log_file;

static inline int do_sysctl(struct xen_sysctl *sysctl)
{
	long ret;
	privcmd_hypercall hypercall;
	DECLARE_HYPERCALL_BOUNCE(sysctl, sizeof(*sysctl),
			XC_HYPERCALL_BUFFER_BOUNCE_BOTH);

	sysctl->interface_version = XEN_SYSCTL_INTERFACE_VERSION;

	memset(&hypercall, 0x0, sizeof(hypercall));
	hypercall.op = __HYPERVISOR_sysctl;
	hypercall.arg[0] = HYPERCALL_BUFFER_AS_ARG(sysctl);

	ret = privcmd_call(hypercall.op,
			hypercall.arg[0], hypercall.arg[1],
			hypercall.arg[2], hypercall.arg[3],
			hypercall.arg[4]);

	return ret;
}

static int xc_readconsolering(char *buffer, unsigned int *char_num, int *index)
{
	int ret;
	struct xen_sysctl sysctl;

	sysctl.cmd = XEN_SYSCTL_readconsole;
	set_xen_guest_handle(sysctl.u.readconsole.buffer, buffer);
	sysctl.u.readconsole.count = *char_num;
	sysctl.u.readconsole.clear = 0;
	sysctl.u.readconsole.incremental = 1;
	sysctl.u.readconsole.index = *index;

	if ((ret = do_sysctl(&sysctl)) == 0) {
		*index = sysctl.u.readconsole.index;
		*char_num = sysctl.u.readconsole.count;
		pr_debug("[%s] index=%d size=%d ret=%d\n", __func__, *index,
				*char_num, ret);
	}

	return ret;
}

static void xen_log_read(struct work_struct *work)
{
	struct xen_readconsole *x_con =
		container_of(work, struct xen_readconsole, work);
	char *bufptr = x_con->xen_buffer;
	unsigned int size = SZ_16K;
	static int read_idx = 0;

	if (xc_readconsolering(bufptr, &size, &read_idx) == 0 && size > 0) {
		char *last_byte = bufptr + size - 1;

		while (bufptr <= last_byte) {
			char *nl = memchr(bufptr, '\n', last_byte + 1 - bufptr);
			int found_nl = (nl != NULL);
			if (!found_nl)
				nl = last_byte;

			snprintf(x_con->print_buffer, nl - bufptr +1, "%s\n",
				 bufptr);
			pr_info("<FROM> %s", x_con->print_buffer);
			bufptr = nl + 1;

			if (found_nl) {
				while (bufptr <= last_byte
				       && (*bufptr == '\r' || *bufptr == '\n'))
					bufptr++;
			}
		}
	}

	return;
}

static irqreturn_t xen_log_interrupt(int irq, void *dev_id)
{
	queue_work(system_nrt_wq, &xen_console.work);
	return IRQ_HANDLED;
}

static int sec_xen_log_open(struct inode *inode, struct file *file)
{
	pr_info("%s\n", __FUNCTION__) ;
	file->private_data = NULL;
	return 0;
}

static ssize_t sec_xen_log_get(struct file *file, char __user *user_buf,
                                size_t count, loff_t *ppos)
{
	char	tmpbuf[128];
	char	*ptrbuf = tmpbuf;
	int	bytesread = 0;
	unsigned int size = SZ_128;
	int read_idx = (int)*ppos;

	pr_debug("%s count=%u pos=%d\n", __FUNCTION__, count, (int)*ppos) ;

	while (xc_readconsolering(ptrbuf, &size, &read_idx) == 0
			&& size > 0
			&& ((bytesread + size) <= count)) {

		if (copy_to_user(user_buf+bytesread, tmpbuf, size)) {
			pr_err("%s() copy_to_user failed\n", __FUNCTION__) ;
			return -EFAULT;
		}

		bytesread += size;
		size = SZ_128;
	}

	*ppos += bytesread;
	pr_debug("%s count=%u res=%d\n", __FUNCTION__, count, bytesread) ;

	return bytesread;
}

static const struct file_operations sec_xen_log_ops = {
	.owner	= THIS_MODULE,
	.open	= sec_xen_log_open,
	.read	= sec_xen_log_get,
	.llseek	= NULL,
};

static int __init xen_log_buf_init(void)
{
	int ret;

	if (!xen_start_info)
		return 0;

	INIT_WORK(&xen_console.work, xen_log_read);

	xen_console.xen_buffer = kmalloc(SZ_64K, GFP_KERNEL);
	if (!xen_console.xen_buffer) {
		ret = -ENOMEM;
		goto xen_buffer_err;
	}

	xen_console.print_buffer = kmalloc(SZ_64K, GFP_KERNEL);
	if (!xen_console.print_buffer) {
		ret = -ENOMEM;
		goto print_buffer_err;
	}

	ret = bind_virq_to_irqhandler(VIRQ_CON_RING, 0, xen_log_interrupt,
			IRQF_DISABLED | IRQF_PERCPU | IRQF_NOBALANCING,
			"xen_log", NULL);
	if (ret < 0) {
		ret = -EIO;
		goto virq_err;
	}

	g_xen_dir = debugfs_create_dir("xen", NULL);
	if (IS_ERR_OR_NULL(g_xen_dir)) {
		ret = PTR_ERR(g_xen_dir);
		goto virq_err;
	}

	g_xen_log_file = debugfs_create_file("log", S_IFREG | S_IRUSR,
							g_xen_dir, NULL, &sec_xen_log_ops);

	pr_info("[%s] (%d)\n", __FUNCTION__, ret);

	return 0;

virq_err:
	kfree(xen_console.xen_buffer);
print_buffer_err:
	kfree(xen_console.print_buffer);
xen_buffer_err:
	return ret;
}
arch_initcall(xen_log_buf_init);
