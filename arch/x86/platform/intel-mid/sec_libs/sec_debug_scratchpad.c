/* arch/x86/platform/intel-mid/sec_libs/sec_debug_scratchpad.c
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

#include <asm/intel_scu_ipc.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/version.h>

#include "sec_debug.h"
#include "sec_gaf.h"


#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,20))
struct scu_ipc_oshob_info {
	__u32	oshob_type;     /* Set from magic number extracted from      */
				/* OSHOB structure. Indicates if old or      */
				/* extended version of OSHOB will be used.   */
	__u32	oshob_base;     /* Base address of OSHOB. Use ioremap to     */
				/* remap for access.                         */
	__u8	oshob_majrev;   /* Major revision number of OSHOB structure. */
	__u8	oshob_minrev;   /* Minor revision number of OSHOB structure. */
	__u16	oshob_size;     /* Total size (bytes) of OSHOB structure.    */
	__u32   scu_trace;      /* SCU trace buffer.                         */
	__u32   ia_trace;       /* IA trace buffer.                          */
	__u16	osnib_size;     /* Total size (bytes) of OSNIB structure.    */
	__u16	oemnib_size;    /* Total size (bytes) of OEMNIB area.        */
	__u32	posnibr;        /* Pointer to Intel read zone.               */
	__u32	posnibw;        /* Pointer to Intel write zone.              */
	__u32	poemnibr;       /* Pointer to OEM read zone.                 */
	__u32	poemnibw;       /* Pointer to OEM write zone.                */
};
#else
#define OSHOB_SCU_BUFFER_SIZE	4    /* In dwords. On Merrifield the needed */
				     /* SCU trace buffer size is 4 dwords.  */
struct scu_ipc_oshob_info {
	__u32	oshob_base;     /* Base address of OSHOB. Use ioremap to     */
				/* remap for access.                         */
	__u8	oshob_majrev;   /* Major revision number of OSHOB structure. */
	__u8	oshob_minrev;   /* Minor revision number of OSHOB structure. */
	__u16	oshob_size;     /* Total size (bytes) of OSHOB structure.    */
	__u32   scu_trace[OSHOB_SCU_BUFFER_SIZE]; /* SCU trace buffer.       */
				/* Buffer max size is OSHOB_SCU_BUFFER_SIZE  */
				/* dwords for MRFLD. On other platforms,     */
				/* only the first dword is stored and read.  */
	__u32   ia_trace;       /* IA trace buffer.                          */
	__u16	osnib_size;     /* Total size (bytes) of OSNIB structure.    */
	__u16	oemnib_size;    /* Total size (bytes) of OEMNIB area.        */
	__u32	osnibr_ptr;     /* Pointer to Intel read zone.               */
	__u32	osnibw_ptr;     /* Pointer to Intel write zone.              */
	__u32	oemnibr_ptr;    /* Pointer to OEM read zone.                 */
	__u32	oemnibw_ptr;    /* Pointer to OEM write zone.                */

	int (*scu_ipc_write_osnib)(u8 *data, int len, int offset);
	int (*scu_ipc_read_osnib)(u8 *data, int len, int offset);

	int platform_type;     /* Identifies the platform (list of supported */
			       /* platforms is given in intel-mid.h).        */

	u16 offs_add;          /* The additional shift bytes to consider     */
			       /* giving the offset at which the OSHOB param */
			       /* will be read. If MRFLD it must be set to   */
			       /* OSHOB_SCU_BUFFER_SIZE dwords.              */

};
#endif

struct scu_ipc_emergency_oshob_info {
	__u16	oemnib_size;    /* Total size (bytes) of OEMNIB area.        */
	__u32	oemnibw_ptr;    /* Pointer to OEM write zone.                */
	void __iomem *oemnibw_addr;
};

static struct scu_ipc_emergency_oshob_info oshob_info;

int intel_scu_ipc_emergency_write_oemnib(u8 *oemnib, int len, int offset)
{

	#define IPCMSG_WRITE_OEMNIB		0xDF
	int i;
	int ret = 0;
	u32 sptr_dw_mask;

	if (!oshob_info.oemnibw_addr) {
		ret = -EINVAL;
		goto out;
	}
		
	if ((len == 0) || (len > oshob_info.oemnib_size)) {
		pr_err("%s: bad OEMNIB data length (%d) to write "
			"(max=%d bytes)\n",
				__func__, len, oshob_info.oemnib_size);
		ret = -EINVAL;
		goto out;
	}

	/* offset shall start at 0 from the OEMNIB base address and shall */
	/* not exceed the OEMNIB allowed size.                            */
	if ((offset < 0) || (offset >= oshob_info.oemnib_size) ||
	    (len + offset > oshob_info.oemnib_size)) {
		pr_err("%s: Bad OEMNIB data offset/len for writing "
			"(offset=%d , len=%d)\n",
				__func__, offset, len);
		ret = -EINVAL;
		goto out;
	}
	pr_info("%s: POEMNIB remap poemnibw 0x%x size %d\n",
		__func__, oshob_info.oemnibw_ptr, oshob_info.oemnib_size);

	for (i = 0; i < len; i++)
		writeb(*(oemnib + i), (oshob_info.oemnibw_addr + offset + i));

	system_state = SYSTEM_HALT;

	sptr_dw_mask = 0xFFFFFFFF;
	ret = intel_scu_ipc_raw_cmd(IPCMSG_WRITE_OEMNIB, 0, NULL, 0, NULL,
			0, 0, sptr_dw_mask);
	if (ret < 0)
		pr_err("%s: ipc_write_osnib failed!!\n", __func__);

out:
	return ret;
}
static int __init intel_scu_ipc_emergency_setup(void)
{
	int ret = 0;
	struct scu_ipc_oshob_info *ptr;
	ptr = (struct scu_ipc_oshob_info *)
					intel_scu_ipc_get_oshob();

	if (!ptr) {
		pr_err("%s: OSHOB is Null!!!!!\n", __func__);
		ret = -ENODEV;
		goto out;
	}
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,20))
	oshob_info.oemnib_size = ptr->oemnib_size;
	oshob_info.oemnibw_ptr = ptr->oemnibw;
#else
	oshob_info.oemnib_size = ptr->oemnib_size;
	oshob_info.oemnibw_ptr = ptr->oemnibw_ptr;
#endif

	oshob_info.oemnibw_addr = ioremap_nocache(oshob_info.oemnibw_ptr,
				oshob_info.oemnib_size);
	if (!oshob_info.oemnibw_addr) {
		pr_err("%s: ioremap failed!\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	pr_info("%s: success remaping!!\n", __func__);
out:
	return ret;
}
device_initcall(intel_scu_ipc_emergency_setup);
