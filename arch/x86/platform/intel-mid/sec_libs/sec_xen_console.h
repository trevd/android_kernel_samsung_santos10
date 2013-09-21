/* arch/x86/platform/intel-mid/sec_libs/sec_xen_console.h
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

#ifndef SEC_XEN_CONSOLE_H
#define SEC_XEN_CONSOLE_H

#define uint64_aligned_t		uint64_t __attribute__((aligned(8)))

#undef DEFINE_GUEST_HANDLE
#define DEFINE_GUEST_HANDLE_64(name, type)				\
	typedef struct { union { type *p; uint64_aligned_t q; }; }	\
		__guest_handle_64_ ## name

#define GUEST_HANDLE_64(name)		__guest_handle_64_ ## name

DEFINE_GUEST_HANDLE_64(char, char);

struct xen_sysctl_readconsole {
	/* IN: Non-zero -> clear after reading. */
	uint8_t clear;
	/* IN: Non-zero -> start index specified by @index field. */
	uint8_t incremental;
	uint8_t pad0, pad1;
	/*
	 * IN:  Start index for consuming from ring buffer (if @incremental);
	 * OUT: End index after consuming from ring buffer.
	 */
	uint32_t index;
	/* IN: Virtual address to write console data. */
	 GUEST_HANDLE_64(char) buffer;
	/* IN: Size of buffer; OUT: Bytes written to buffer. */
	uint32_t count;
};

struct xen_sysctl {
	uint32_t cmd;
	uint32_t interface_version;	/* XEN_SYSCTL_INTERFACE_VERSION */
	union {
		struct xen_sysctl_readconsole readconsole;
		uint8_t pad[128];
	} u;
};

struct xc_hypercall_buffer {
	/* Hypercall safe memory buffer. */
	void *sbuf;

	/*
	 * Reference to xc_hypercall_buffer passed as argument to the
	 * current function.
	 */
	struct xc_hypercall_buffer *param_shadow;

	/*
	 * Direction of copy for bounce buffering.
	 */
	int dir;

	/* Used iff dir != 0. */
	/*
	   void *ubuf;
	 */
	size_t sz;
};

typedef struct privcmd_hypercall {
	__u64 op;
	__u64 arg[5];
} privcmd_hypercall;

/*
 * HYPERCALL ARGUMENT BUFFERS
 *
 * Augment the public hypercall buffer interface with the ability to
 * bounce between user provided buffers and hypercall safe memory.
 *
 * Use xc_hypercall_bounce_pre/post instead of
 * xc_hypercall_buffer_alloc/free(_pages).  The specified user
 * supplied buffer is automatically copied in/out of the hypercall
 * safe memory.
 */
enum {
	XC_HYPERCALL_BUFFER_BOUNCE_NONE = 0,
	XC_HYPERCALL_BUFFER_BOUNCE_IN = 1,
	XC_HYPERCALL_BUFFER_BOUNCE_OUT = 2,
	XC_HYPERCALL_BUFFER_BOUNCE_BOTH = 3
};

#endif
