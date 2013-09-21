/* arch/x86/platform/intel-mid/sec_libs/sec_debug.c
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

#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/keyboard.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/sysrq.h>
#include <linux/uaccess.h>
#include <linux/bootmem.h>
#include <linux/memblock.h>
#include <linux/export.h>
#include <asm-generic/sizes.h>

#include <asm/desc.h>
#include <asm/reboot.h>
#include <asm/intel_scu_ipcutil.h>
#include <asm/intel_scu_ipc.h>
#include <asm/pat.h>
#include "sec_debug.h"
#include "sec_gaf.h"
#include "../intel_mid_scu.h"

enum sec_debug_upload_cause_t {
	UPLOAD_CAUSE_INIT = 0xCAFEBABE,
	UPLOAD_CAUSE_KERNEL_PANIC = 0x000000C8,
	UPLOAD_CAUSE_FORCED_UPLOAD = 0x00000022,
	UPLOAD_CAUSE_CP_ERROR_FATAL = 0x000000CC,
	UPLOAD_CAUSE_USER_FAULT = 0x0000002F,
	UPLOAD_CAUSE_HSIC_DISCONNECTED = 0x000000DD,
};

struct sec_debug_mmu_reg_t {
	struct desc_ptr gdt;
	struct desc_ptr idt;
	unsigned int tr;
};

/* x86 CORE regs mapping structure */
struct sec_debug_core_t {
	/*General Purpose Registers */
	unsigned long eax;
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;

	/*Segment Registers */
	unsigned long cs;
	unsigned long ds;
	unsigned long es;
	unsigned long fs;
	unsigned long gs;
	unsigned long ss;

	/*Pointer Registers */
	unsigned long ebp;
	unsigned long esp;

	/*Index Register */
	unsigned long esi;
	unsigned long edi;

	/*Instruction Register */
	unsigned long eip;

	/*Flag Register */
	unsigned long eflags;

	/*Control Register */
	unsigned long cr0;
	unsigned long cr1;
	unsigned long cr2;
	unsigned long cr3;
	unsigned long cr4;

	/*Debug Register */
	unsigned long dr0;
	unsigned long dr1;
	unsigned long dr2;
	unsigned long dr3;
	unsigned long dr4;
	unsigned long dr5;
	unsigned long dr6;
	unsigned long dr7;
};

static char *cp_crashstr;
/* enable/disable sec_debug feature
 * level = 0 when enable = 0 && enable_user = 0
 * level = 1 when enable = 1 && enable_user = 0
 * level = 0x10001 when enable = 1 && enable_user = 1
 * The other cases are not considered
 */
union sec_debug_level_t sec_debug_level = { .en.kernel_fault = 1, };

module_param_named(enable, sec_debug_level.en.kernel_fault, ushort,
		   S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(enable_user, sec_debug_level.en.user_fault, ushort,
		   S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(level, sec_debug_level.uint_val, uint,
		   S_IRUGO | S_IWUSR | S_IWGRP);

static int __init sec_debug_parse_enable(char *str)
{
	if (kstrtou16(str, 0, &sec_debug_level.en.kernel_fault))
		sec_debug_level.en.kernel_fault = 1;

	return 0;
}

early_param("sec_debug.enable", sec_debug_parse_enable);

static int __init sec_debug_parse_enable_user(char *str)
{
	if (kstrtou16(str, 0, &sec_debug_level.en.user_fault))
		sec_debug_level.en.user_fault = 0;

	return 0;
}

early_param("sec_debug.enable_user", sec_debug_parse_enable_user);

static int __init sec_debug_parse_level(char *str)
{
	if (kstrtou32(str, 0, &sec_debug_level.uint_val))
		sec_debug_level.uint_val = 1;

	return 0;
}

early_param("sec_debug.level", sec_debug_parse_level);

static const char *const gkernel_sec_build_info_date_time[] = {
	__DATE__,
	__TIME__
};

static char gkernel_sec_build_info[100];

/* klaatu - schedule log */
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
#define SCHED_LOG_MAX			2048

struct sched_log {
	struct task_log {
		unsigned long long time;
		char comm[TASK_COMM_LEN];
		pid_t pid;
	} task[CONFIG_NR_CPUS][SCHED_LOG_MAX];
	struct irq_log {
		unsigned long long time;
		int irq;
		void *fn;
		int en;
	} irq[CONFIG_NR_CPUS][SCHED_LOG_MAX];
	struct work_log {
		unsigned long long time;
		struct worker *worker;
		struct work_struct *work;
		work_func_t f;
		int en;
	} work[CONFIG_NR_CPUS][SCHED_LOG_MAX];
	struct hrtimer_log {
		unsigned long long time;
		struct hrtimer *timer;
		enum hrtimer_restart (*fn)(struct hrtimer *);
		int en;
	} hrtimers[CONFIG_NR_CPUS][8];
	struct cpuidle_log {
		unsigned long long time;
		int	state;
		int en;
	} cpuidles[CONFIG_NR_CPUS][64];
	struct hypercall_log {
		unsigned long long time;
		unsigned int op;
		int en;
		int	argc;
		unsigned long arg[5];
	} hypercalls[CONFIG_NR_CPUS][SCHED_LOG_MAX];
	struct xen_ipi_function_call_log {
		unsigned long long time;
		void	*func;
		int		en;
		struct call_single_data *data;
	} xen_ipi_function_calls[CONFIG_NR_CPUS][64];
	struct {
		unsigned long long time;
		unsigned long pc;
		unsigned long lr;
		unsigned long sp;
	} last_wdtkick_regs[CONFIG_NR_CPUS];
	struct {
		unsigned long long time;
		unsigned long pc;
		unsigned long lr;
		unsigned long sp;
	} last_irq_regs[CONFIG_NR_CPUS][4];
	struct {
		unsigned long long time;
		unsigned int cmd;
		unsigned int sub;
		unsigned char in[16];
		unsigned int inlen;
		unsigned int *out;
		unsigned int outlen;
		unsigned int dptr;
		unsigned int sptr;
		int en;
	} last_ipc_log[CONFIG_NR_CPUS][16];
};


static struct sched_log (*psec_debug_log);

static atomic_t task_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t irq_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t work_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t hrtimer_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t cpuidle_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t hypercall_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1),
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t xen_ipi_function_call_log_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1),
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t irq_regs_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static atomic_t irq_ipc_idx[NR_CPUS] = {
	ATOMIC_INIT(-1), ATOMIC_INIT(-1)
};

static int checksum_sched_log(void)
{
	int sum = 0, i;

	if (unlikely(!psec_debug_log))
		return 0;

	for (i = 0; i < sizeof(*psec_debug_log); i++)
		sum += *((char *)psec_debug_log + i);

	return sum;
}

void __sec_debug_ipc_log(u32 cmd, u32 sub, u8 *in, u32 inlen, u32 *out,
			u32 outlen, u32 dptr, u32 sptr, int en)
{
	int cpu = raw_smp_processor_id();
	unsigned int i,j;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&irq_ipc_idx[cpu]) &
	    (ARRAY_SIZE(psec_debug_log->last_ipc_log[0]) - 1);
	psec_debug_log->last_ipc_log[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->last_ipc_log[cpu][i].cmd = cmd;
	psec_debug_log->last_ipc_log[cpu][i].sub = sub;
    memcpy(psec_debug_log->last_ipc_log[cpu][i].in, in, inlen);
	psec_debug_log->last_ipc_log[cpu][i].inlen = inlen;
	psec_debug_log->last_ipc_log[cpu][i].out = out;
	psec_debug_log->last_ipc_log[cpu][i].outlen = outlen;
	psec_debug_log->last_ipc_log[cpu][i].dptr = dptr;
	psec_debug_log->last_ipc_log[cpu][i].sptr = sptr;
	psec_debug_log->last_ipc_log[cpu][i].en = en;
}

#else
static int checksum_sched_log(void)
{
	return 0;
}
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

/* klaatu - semaphore log */
#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
#define SEMAPHORE_LOG_MAX		100

struct sem_debug {
	struct list_head list;
	struct semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	/* char comm[TASK_COMM_LEN]; */
};

enum {
	READ_SEM,
	WRITE_SEM,
};

static struct sem_debug sem_debug_free_head;
static struct sem_debug sem_debug_done_head;
static int sem_debug_free_head_cnt;
static int sem_debug_done_head_cnt;
static int sem_debug_init;
static spinlock_t sem_debug_lock;

/* rwsemaphore logging */
#define RWSEMAPHORE_LOG_MAX		100

struct rwsem_debug {
	struct list_head list;
	struct rw_semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	int direction;
	/* char comm[TASK_COMM_LEN]; */
};

static struct rwsem_debug rwsem_debug_free_head;
static struct rwsem_debug rwsem_debug_done_head;
static int rwsem_debug_free_head_cnt;
static int rwsem_debug_done_head_cnt;
static int rwsem_debug_init;
static spinlock_t rwsem_debug_lock;

#endif /* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

DEFINE_PER_CPU(struct sec_debug_core_t, sec_debug_core_reg);
DEFINE_PER_CPU(struct sec_debug_mmu_reg_t, sec_debug_mmu_reg);
DEFINE_PER_CPU(enum sec_debug_upload_cause_t, sec_debug_upload_cause);

/* core reg dump function*/
static inline void sec_debug_save_core_reg(struct sec_debug_core_t *pt_regs)
{
	/* we will be in SVC mode when we enter this function. Collect
	   SVC registers along with cmn registers. */
	asm volatile ("movl %%eax,%0" : "=m" (pt_regs->eax));
	asm volatile ("movl %%ebx,%0" : "=m" (pt_regs->ebx));
	asm volatile ("movl %%ecx,%0" : "=m" (pt_regs->ecx));
	asm volatile ("movl %%edx,%0" : "=m" (pt_regs->edx));
	asm volatile ("movl %%esi,%0" : "=m" (pt_regs->esi));
	asm volatile ("movl %%edi,%0" : "=m" (pt_regs->edi));
	asm volatile ("movl %%ebp,%0" : "=m" (pt_regs->ebp));
	asm volatile ("movl %%esp,%0" : "=m" (pt_regs->esp));
	asm volatile ("mov $1f, %0; 1:" : "=r" (pt_regs->eip));

	asm volatile ("mov %%db0, %0" : "=r" (pt_regs->dr0));
	asm volatile ("mov %%db1, %0" : "=r" (pt_regs->dr1));
	asm volatile ("mov %%db2, %0" : "=r" (pt_regs->dr2));
	asm volatile ("mov %%db3, %0" : "=r" (pt_regs->dr3));
	asm volatile ("mov %%db6, %0" : "=r" (pt_regs->dr6));
	asm volatile ("mov %%db7, %0" : "=r" (pt_regs->dr7));

	asm volatile ("movl %%cs, %%eax;" : "=a" (pt_regs->cs));
	asm volatile ("movl %%ds, %%eax;" : "=a" (pt_regs->ds));
	asm volatile ("movl %%es, %%eax;" : "=a" (pt_regs->es));
	asm volatile ("movl %%fs, %%eax;" : "=a" (pt_regs->fs));
	asm volatile ("movl %%gs, %%eax;" : "=a" (pt_regs->gs));
	asm volatile ("movl %%ss, %%eax;" : "=a" (pt_regs->ss));

	asm volatile ("pushfl; popl %0" : "=m" (pt_regs->eflags));

	pt_regs->cr0 = read_cr0();
	pt_regs->cr2 = read_cr2();
	pt_regs->cr3 = read_cr3();
	pt_regs->cr4 = read_cr4();
}

static inline void sec_debug_save_mmu_reg(struct sec_debug_mmu_reg_t *mmu_reg)
{
	store_gdt(&mmu_reg->gdt);
	store_idt(&mmu_reg->idt);
	store_tr(mmu_reg->tr);
}

#define __sec_debug_show_1_reg(_container, _reg0)			\
do {									\
	pr_emerg(" %-4s: %08lx\n", #_reg0, _container->_reg0);		\
} while (0)

#define __sec_debug_show_2_reg(_container, _reg0, _reg1)		\
do {									\
	pr_emerg(" %-4s: %08lx %-4s: %08lx\n",				\
		#_reg0, _container->_reg0, #_reg1, _container->_reg1);	\
} while (0)

#define __sec_debug_show_3_reg(_container, _reg0, _reg1, _reg2)		\
do {									\
	pr_emerg(" %-4s: %08x %-4s: %08x %-4s: %08x\n",			\
		#_reg0, _container->_reg0, #_reg1, _container->_reg1,	\
		#_reg2,	_container->_reg2);				\
} while (0)

#define __sec_debug_show_4_reg(_container, _reg0, _reg1, _reg2, _reg3)	\
do {									\
	pr_emerg(" %-4s: %08lx %-4s: %08lx %-4s: %08lx %-4s: %08lx\n",	\
		#_reg0, _container->_reg0, #_reg1, _container->_reg1,	\
		#_reg2,	_container->_reg2, #_reg3, _container->_reg3);	\
} while (0)

static void sed_debug_show_core_reg(struct sec_debug_core_t *core_reg)
{
	pr_emerg("Sec Core REGs:\n");

	pr_emerg(" - General Purpose Registers\n");
	__sec_debug_show_4_reg(core_reg, eax, ebx, ecx, edx);
	pr_emerg(" - Index Registers\n");
	__sec_debug_show_2_reg(core_reg, esi, edi);
	pr_emerg(" - Pointer Registers\n");
	__sec_debug_show_2_reg(core_reg, ebp, esp);
	pr_emerg(" - Instruction Pointer\n");
	__sec_debug_show_1_reg(core_reg, eip);
	pr_emerg(" - Flag Register\n");
	__sec_debug_show_1_reg(core_reg, eflags);
	pr_emerg(" - Segment Registers\n");
	__sec_debug_show_4_reg(core_reg, ds, es, fs, gs);
	__sec_debug_show_2_reg(core_reg, ss, cs);
	pr_emerg(" - Control Registers\n");
	__sec_debug_show_4_reg(core_reg, cr0, cr2, cr3, cr4);

}

static void sec_debug_show_mmu_reg(struct sec_debug_mmu_reg_t *mmu_reg)
{
	pr_emerg("Sec MMU REGs:\n");

	pr_emerg(" GDT: BASE 0x%08lx LIMIT 0x%04x\n",
		 mmu_reg->gdt.address, mmu_reg->gdt.size);
	pr_emerg(" IDT: BASE 0x%08lx LIMIT 0x%04x\n",
		 mmu_reg->idt.address, mmu_reg->idt.size);
	pr_emerg(" TR : 0x%08x\n", mmu_reg->tr);

}

static void sec_debug_save_context(void)
{
	unsigned long flags;

	local_irq_save(flags);
	sec_debug_save_mmu_reg(
			&per_cpu(sec_debug_mmu_reg, smp_processor_id()));
	sec_debug_save_core_reg(
			&per_cpu(sec_debug_core_reg, smp_processor_id()));

	pr_emerg("(%s) context saved(CPU:%d)\n", __func__, smp_processor_id());
	sec_debug_show_mmu_reg(
			&per_cpu(sec_debug_mmu_reg, smp_processor_id()));
	sed_debug_show_core_reg(
			&per_cpu(sec_debug_core_reg, smp_processor_id()));
	local_irq_restore(flags);
}

static struct sec_debug_mmu_reg_t sec_emerg_mmu_reg;

static int __init sec_debug_emerg_mmu_reg_init(void)
{
	if (!sec_debug_level.en.kernel_fault)
		return 0;

	sec_debug_save_mmu_reg(&sec_emerg_mmu_reg);

	return 0;
}

arch_initcall(sec_debug_emerg_mmu_reg_init);


static void sec_debug_init_oemnib(void)
{
	u32 magic_dump = SEC_DEBUG_MAGIC_DUMP;
	u32 cause_init = UPLOAD_CAUSE_INIT;
	u32 p_cpcrashstr =
		(u32)(virt_to_phys(cp_crashstr) & UINT_MAX);

	intel_scu_ipc_write_oemnib((u8 *)&magic_dump,
				   0x4, SEC_DEBUG_MAGIC_ADDR);
	intel_scu_ipc_write_oemnib((u8 *)&cause_init,
				   0x4, SEC_DEBUG_UPLOAD_CAUSE_ADDR);

	intel_scu_ipc_write_oemnib((u8 *)&p_cpcrashstr,
				   0x4, SEC_DEBUG_CP_CRASHSTR_ADDR);
	/*flush_cache_all */
	asm volatile ("wbinvd\n");
}
static void sec_debug_set_upload_magic(unsigned int magic)
{
	intel_scu_ipc_emergency_write_oemnib((u8 *)&magic,
					     0x4, SEC_DEBUG_MAGIC_ADDR);
	pr_debug("(%s) %x\n", __func__, magic);

	/*flush_cache_all */
	asm volatile ("wbinvd\n");
}

static int sec_debug_normal_reboot_handler(struct notifier_block *nb,
					   unsigned long l, void *p)
{
	u32 value = 0x0;

	intel_scu_ipc_write_oemnib((u8 *)&value, 0x4, SEC_DEBUG_MAGIC_ADDR);

	return 0;
}

static void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type)
{
	per_cpu(sec_debug_upload_cause, smp_processor_id()) = type;
	intel_scu_ipc_emergency_write_oemnib((u8 *)&type,
					     0x4, SEC_DEBUG_UPLOAD_CAUSE_ADDR);
	pr_debug("(%s) %x\n", __func__, type);
}

/*
 * Called from dump_stack()
 * This function call does not necessarily mean that a fatal error
 * had occurred. It may be just a warning.
 */
static inline int sec_debug_dump_stack(void)
{
	if (!sec_debug_level.en.kernel_fault)
		return -1;

	sec_debug_save_context();

	/* flush L1 from each core.
	   L2 will be flushed later before reset. */
	asm volatile ("wbinvd\n");

	return 0;
}

static void sec_emergency_reboot(void)
{
	/* Change system state to poll IPC status until IPC not busy*/
	system_state = SYSTEM_RESTART;

	while (intel_scu_ipc_check_status())
		udelay(10);

	intel_scu_ipc_raw_cmd(IPCMSG_COLD_RESET,
			0, NULL, 0, NULL, 0, 0, 0);
}
static inline void sec_debug_hw_reset(void)
{
	pr_emerg("(%s) %s\n", __func__, gkernel_sec_build_info);

	/* flush L1 from each core.
	   L2 will be flushed later before reset. */
 	pr_emerg("(%s) rebooting...\n", __func__);
	asm volatile ("wbinvd\n");

	pr_emerg("(%s) rebooting...\n", __func__);
	sec_emergency_reboot();
	pr_emerg("(%s) Oops! fail!\n", __func__);
	while (1)
		;
}
void sec_store_cp_crash_info(char *info, size_t len)
{
	size_t remain_len =  0;
	size_t max_len =  len > SZ_1K ? SZ_1K : len;

	char *ptr_null = NULL;

	if (!cp_crashstr)
		return;

	ptr_null = cp_crashstr;
	memcpy(cp_crashstr, info, max_len);

	/*remove null pointer from message*/
	remain_len = max_len - 1;
	while(remain_len) {
		ptr_null = memchr(ptr_null, '\0', remain_len);
		if (!ptr_null ||
			(*(ptr_null) =='\0' && *(ptr_null+1) =='\0'))
			break;

		*ptr_null = '.';
		ptr_null++;
		remain_len = (cp_crashstr + max_len - 1) - ptr_null;

	}
	cp_crashstr[max_len]='\0';

	pr_info("[%s] Len=%d : %s\n", __func__, len, cp_crashstr);
	return;
}

int dump_rte(char *page, char **start, off_t off,
		int count, int *eof, void *data);

static int sec_debug_panic_handler(struct notifier_block *nb,
		unsigned long l, void *buf)
{

	local_irq_disable();

	sec_debug_set_upload_magic(SEC_DEBUG_MAGIC_DUMP);

	if (!strcmp(buf, "User Fault"))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FAULT);
	else if (!strcmp(buf, "Crash Key"))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_FORCED_UPLOAD);
	else if (!strncmp(buf, "CP Crash", 8)) {
		/* more information will be provided after "CP Crash" string */
		sec_debug_set_upload_cause(UPLOAD_CAUSE_CP_ERROR_FATAL);
	} else if (!strcmp(buf, "HSIC Disconnected"))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_HSIC_DISCONNECTED);
	else
		sec_debug_set_upload_cause(UPLOAD_CAUSE_KERNEL_PANIC);

	pr_err("(%s) checksum_sched_log: %x\n", __func__, checksum_sched_log());

	/*
	dump_rte(NULL, NULL, 0, 0, NULL, NULL);
	*/

	sec_debug_dump_stack();

	sec_gaf_dump_all_task_info();

	sec_gaf_dump_cpu_stat();

	sec_debug_hw_reset();

	return 0;
}

static unsigned int crash_key_code[] = {
	KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_POWER
};

static struct sec_crash_key dflt_crash_key = {
	.keycode	= crash_key_code,
	.size		= ARRAY_SIZE(crash_key_code),
	.timeout	= 1000,	/* 1 sec */
};

static struct sec_crash_key *__sec_crash_key = &dflt_crash_key;

static unsigned int sec_debug_unlock_crash_key(unsigned int value,
		unsigned int *unlock)
{
	unsigned int i = __sec_crash_key->size -
		__sec_crash_key->knock - 1;	/* except triggers */
	unsigned int ret = 0;

	do {
		if (value == __sec_crash_key->keycode[i]) {
			ret = 1;
			*unlock |= 1 << i;
		}
	} while (i-- != 0);

	return ret;
}

static void sec_debug_check_crash_key(unsigned int value, int down)
{
	static unsigned long unlock_jiffies;
	static unsigned long trigger_jiffies;
	static bool other_key_pressed;
	static unsigned int unlock;
	static unsigned int knock;
	unsigned int timeout;

	if (!down) {
		if (unlock == __sec_crash_key->unlock &&
		    value == __sec_crash_key->trigger)
			return;
		else
			goto __clear_all;
	}

	if (sec_debug_unlock_crash_key(value, &unlock)) {
		if (unlock == __sec_crash_key->unlock && !unlock_jiffies)
			unlock_jiffies = jiffies;
	} else if (value == __sec_crash_key->trigger) {
		trigger_jiffies = jiffies;
		knock++;
	} else {
		other_key_pressed = true;
		goto __clear_timeout;
	}

	if (unlock_jiffies && trigger_jiffies && !other_key_pressed &&
	    time_after(trigger_jiffies, unlock_jiffies)) {
		timeout = jiffies_to_msecs(trigger_jiffies - unlock_jiffies);
		if (timeout < __sec_crash_key->timeout) {
			if (knock >= __sec_crash_key->knock)
				panic("Crash Key");
			return;
		} else
			goto __clear_all;
	}

	return;

__clear_all:
	other_key_pressed = false;
__clear_timeout:
	unlock_jiffies = 0;
	trigger_jiffies = 0;
	unlock = 0;
	knock = 0;
}

static int sec_debug_keyboard_call(struct notifier_block *this,
				   unsigned long type, void *data)
{
	struct keyboard_notifier_param *param = data;

	if (likely(type != KBD_KEYCODE && type != KBD_UNBOUND_KEYCODE))
		return NOTIFY_DONE;

	sec_debug_check_crash_key(param->value, param->down);

	return NOTIFY_DONE;
}

static struct notifier_block sec_debug_keyboard_notifier = {
	.notifier_call = sec_debug_keyboard_call,
};

void __init sec_debug_init_crash_key(struct sec_crash_key *crash_key)
{
	int i;

	if (!sec_debug_level.en.kernel_fault)
		return;

	if (crash_key) {
		__sec_crash_key = crash_key;
		if (__sec_crash_key->timeout == 0)
			__sec_crash_key->timeout = dflt_crash_key.timeout;
	}

	__sec_crash_key->trigger =
		__sec_crash_key->keycode[__sec_crash_key->size - 1];
	__sec_crash_key->knock = 1;
	for (i = __sec_crash_key->size - 2; i >= 0; i--) {
		if (__sec_crash_key->keycode[i] != __sec_crash_key->trigger)
			break;
		__sec_crash_key->knock++;
	}

	__sec_crash_key->unlock = 0;
	for (i = 0; i < __sec_crash_key->size - __sec_crash_key->knock; i++)
		__sec_crash_key->unlock |= 1 << i;

	register_keyboard_notifier(&sec_debug_keyboard_notifier);
}

static struct notifier_block nb_reboot_block = {
	.notifier_call = sec_debug_normal_reboot_handler
};

static struct notifier_block nb_panic_block = {
	.notifier_call = sec_debug_panic_handler,
};

static void sec_debug_set_build_info(void)
{
	char *p = gkernel_sec_build_info;

	sprintf(p, "Kernel Build Info : ");
	strcat(p, " Date:");
	strncat(p, gkernel_sec_build_info_date_time[0], 12);
	strcat(p, " Time:");
	strncat(p, gkernel_sec_build_info_date_time[1], 9);
}

static int __init sec_debug_init(void)
{

	sec_debug_set_build_info();
	/*
	   sec_debug_set_upload_magic(SEC_DEBUG_MAGIC_DUMP);
	   sec_debug_set_upload_cause(UPLOAD_CAUSE_INIT);
	 */

	register_reboot_notifier(&nb_reboot_block);

	atomic_notifier_chain_register(&panic_notifier_list, &nb_panic_block);

	/*alloc to store cp crash string*/
	cp_crashstr = (char *)kzalloc(SZ_1K, GFP_KERNEL);
	if (!cp_crashstr)
		pr_err("[%s] fail to allocate buffer for cp_crash string!!\n", __func__);

	pr_info("[%s] cp_crashstr=0x%p(PA:0x%p)\n", __func__,
			cp_crashstr, (void *)(unsigned long)virt_to_phys(cp_crashstr));

	sec_debug_init_oemnib();

	return 0;
}

device_initcall(sec_debug_init);

int sec_debug_get_level(void)
{
	return sec_debug_level.uint_val;
}

/* Exynos compatibility */
int get_sec_debug_level(void)
    __attribute__ ((weak, alias("sec_debug_get_level")));

/* klaatu - schedule log */
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
void __sec_debug_task_log(int cpu, struct task_struct *task)
{
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&task_log_idx[cpu]) & (SCHED_LOG_MAX - 1);
	psec_debug_log->task[cpu][i].time = cpu_clock(cpu);
	strcpy(psec_debug_log->task[cpu][i].comm, task->comm);
	psec_debug_log->task[cpu][i].pid = task->pid;
}

void __sec_debug_irq_log(unsigned int irq, void *fn, int en)
{
	int cpu = raw_smp_processor_id();
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&irq_log_idx[cpu]) & (SCHED_LOG_MAX - 1);
	psec_debug_log->irq[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->irq[cpu][i].irq = irq;
	psec_debug_log->irq[cpu][i].fn = (void *)fn;
	psec_debug_log->irq[cpu][i].en = en;
}

void __sec_debug_work_log(struct worker *worker,
		struct work_struct *work, work_func_t f, int en)
{
	int cpu = raw_smp_processor_id();
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&work_log_idx[cpu]) & (SCHED_LOG_MAX - 1);
	psec_debug_log->work[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->work[cpu][i].worker = worker;
	psec_debug_log->work[cpu][i].work = work;
	psec_debug_log->work[cpu][i].f = f;
	psec_debug_log->work[cpu][i].en = en;
}

void __sec_debug_hrtimer_log(struct hrtimer *timer,
		enum hrtimer_restart (*fn) (struct hrtimer *), int en)
{
	int cpu = raw_smp_processor_id();
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&hrtimer_log_idx[cpu]) &
	    (ARRAY_SIZE(psec_debug_log->hrtimers[0]) - 1);
	psec_debug_log->hrtimers[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->hrtimers[cpu][i].timer = timer;
	psec_debug_log->hrtimers[cpu][i].fn = fn;
	psec_debug_log->hrtimers[cpu][i].en = en;
}

void __sec_debug_cpuidle_log(int state, int en)
{
	int cpu = raw_smp_processor_id();
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&cpuidle_log_idx[cpu]) &
	    (ARRAY_SIZE(psec_debug_log->cpuidles[0]) - 1);
	psec_debug_log->cpuidles[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->cpuidles[cpu][i].state = state;
	psec_debug_log->cpuidles[cpu][i].en = en;
}


void __sec_debug_hypercall_log(int op, int en, int argc,
		unsigned long arg1, unsigned long arg2, unsigned long arg3,
		unsigned long arg4, unsigned long arg5)
{
	int cpu = raw_smp_processor_id();
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&hypercall_log_idx[cpu]) &
	    (ARRAY_SIZE(psec_debug_log->hypercalls[0]) - 1);

	psec_debug_log->hypercalls[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->hypercalls[cpu][i].op = op;
	psec_debug_log->hypercalls[cpu][i].en = en;
	psec_debug_log->hypercalls[cpu][i].argc = argc;
	psec_debug_log->hypercalls[cpu][i].arg[0] = arg1;
	psec_debug_log->hypercalls[cpu][i].arg[1] = arg2;
	psec_debug_log->hypercalls[cpu][i].arg[2] = arg3;
	psec_debug_log->hypercalls[cpu][i].arg[3] = arg4;
	psec_debug_log->hypercalls[cpu][i].arg[4] = arg5;
}
EXPORT_SYMBOL(__sec_debug_hypercall_log);
inline void sec_debug_hypercall_log(int op, int en, int size,
		unsigned long arg1, unsigned long arg2, unsigned long arg3,
		unsigned long arg4, unsigned long arg5)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_hypercall_log(op, en, size, arg1, arg2, arg3, arg4, arg5);
}
EXPORT_SYMBOL(sec_debug_hypercall_log);

void __sec_debug_xen_ipi_function_call_log(void *func, int en,
									struct call_single_data *data)
{
	int cpu = raw_smp_processor_id();
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&xen_ipi_function_call_log_idx[cpu]) &
	    (ARRAY_SIZE(psec_debug_log->xen_ipi_function_calls[0]) - 1);

	psec_debug_log->xen_ipi_function_calls[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->xen_ipi_function_calls[cpu][i].func = func;
	psec_debug_log->xen_ipi_function_calls[cpu][i].en = en;
	psec_debug_log->xen_ipi_function_calls[cpu][i].data = data;
}

void __sec_debug_wdtkick_regs_log(struct pt_regs *regs)
{
	int cpu = raw_smp_processor_id();
	struct pt_regs dummy_regs = {
		.ip = 0,
		.bp = 0,
		.sp = 0,
	};

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (!regs)		/* regs can be NULL */
		regs = &dummy_regs;

	if (unlikely(!psec_debug_log))
		return;

	psec_debug_log->last_wdtkick_regs[cpu].time = cpu_clock(cpu);
	psec_debug_log->last_wdtkick_regs[cpu].pc = regs->ip;
	psec_debug_log->last_wdtkick_regs[cpu].lr = regs->bp;
	psec_debug_log->last_wdtkick_regs[cpu].sp = regs->sp;
}

void __sec_debug_irq_regs_log(int cpu, struct pt_regs *regs)
{
	unsigned int i;

	if (likely(!sec_debug_level.en.kernel_fault))
		return;

	if (unlikely(!psec_debug_log))
		return;

	i = atomic_inc_return(&irq_regs_idx[cpu]) &
	    (ARRAY_SIZE(psec_debug_log->last_irq_regs[0]) - 1);
	psec_debug_log->last_irq_regs[cpu][i].time = cpu_clock(cpu);
	psec_debug_log->last_irq_regs[cpu][i].pc = regs->ip;
	psec_debug_log->last_irq_regs[cpu][i].lr = regs->bp;
	psec_debug_log->last_irq_regs[cpu][i].sp = regs->sp;
}

#ifdef CONFIG_SEC_DEBUG_IRQ_EXIT_LOG
static unsigned long long excp_irq_exit_time[NR_CPUS];

void sec_debug_irq_last_exit_log(void)
{
	int cpu = raw_smp_processor_id();

	excp_irq_exit_time[cpu] = cpu_clock(cpu);
}
#endif /* CONFIG_SEC_DEBUG_IRQ_EXIT_LOG */

static int __init sec_debug_log_alloc(void)
{
	dma_addr_t dma;
	unsigned int order;
	unsigned long want_flags = 0x0;
	order = get_order(sizeof(struct sched_log));
	psec_debug_log = dma_alloc_coherent(NULL,
			PAGE_SIZE << order, &dma, GFP_KERNEL);

	if (!psec_debug_log)
		return 0;

	want_flags = pgprot_val(PAGE_KERNEL_IO_NOCACHE);
	kernel_map_sync_memtype(dma, sizeof(struct sched_log), want_flags);

	pr_info("%s : sec debug log vaddr:0x%p paddr:0x%p\n", __func__,
			(void *)(unsigned long)psec_debug_log,
			(void *)(unsigned long)dma);
	return 0;
}

device_initcall(sec_debug_log_alloc);
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

/* klaatu - semaphore log */
#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
void debug_semaphore_init(void)
{
	unsigned int i;
	struct sem_debug *sem_debug = NULL;

	spin_lock_init(&sem_debug_lock);
	sem_debug_free_head_cnt = 0;
	sem_debug_done_head_cnt = 0;

	/* initialize list head of sem_debug */
	INIT_LIST_HEAD(&sem_debug_free_head.list);
	INIT_LIST_HEAD(&sem_debug_done_head.list);

	for (i = 0; i < SEMAPHORE_LOG_MAX; i++) {
		/* malloc semaphore */
		sem_debug = kmalloc(sizeof(struct sem_debug), GFP_KERNEL);
		/* add list */
		list_add(&sem_debug->list, &sem_debug_free_head.list);
		sem_debug_free_head_cnt++;
	}

	sem_debug_init = 1;
}

void debug_semaphore_down_log(struct semaphore *sem)
{
	struct list_head *tmp;
	struct sem_debug *sem_dbg;
	unsigned long flags;

	if (!sem_debug_init)
		return;

	spin_lock_irqsave(&sem_debug_lock, flags);
	list_for_each(tmp, &sem_debug_free_head.list) {
		sem_dbg = list_entry(tmp, struct sem_debug, list);
		sem_dbg->task = current;
		sem_dbg->sem = sem;
		/* strcpy(sem_dbg->comm,current->group_leader->comm); */
		sem_dbg->pid = current->pid;
		sem_dbg->cpu = smp_processor_id();
		list_del(&sem_dbg->list);
		list_add(&sem_dbg->list, &sem_debug_done_head.list);
		sem_debug_free_head_cnt--;
		sem_debug_done_head_cnt++;
		break;
	}
	spin_unlock_irqrestore(&sem_debug_lock, flags);
}

void debug_semaphore_up_log(struct semaphore *sem)
{
	struct list_head *tmp;
	struct sem_debug *sem_dbg;
	unsigned long flags;

	if (!sem_debug_init)
		return;

	spin_lock_irqsave(&sem_debug_lock, flags);
	list_for_each(tmp, &sem_debug_done_head.list) {
		sem_dbg = list_entry(tmp, struct sem_debug, list);
		if (sem_dbg->sem == sem && sem_dbg->pid == current->pid) {
			list_del(&sem_dbg->list);
			list_add(&sem_dbg->list, &sem_debug_free_head.list);
			sem_debug_free_head_cnt++;
			sem_debug_done_head_cnt--;
			break;
		}
	}
	spin_unlock_irqrestore(&sem_debug_lock, flags);
}

/* rwsemaphore logging */
void debug_rwsemaphore_init(void)
{
	unsigned int i;
	struct rwsem_debug *rwsem_debug = NULL;

	spin_lock_init(&rwsem_debug_lock);
	rwsem_debug_free_head_cnt = 0;
	rwsem_debug_done_head_cnt = 0;

	/* initialize list head of sem_debug */
	INIT_LIST_HEAD(&rwsem_debug_free_head.list);
	INIT_LIST_HEAD(&rwsem_debug_done_head.list);

	for (i = 0; i < RWSEMAPHORE_LOG_MAX; i++) {
		/* malloc semaphore */
		rwsem_debug = kmalloc(sizeof(struct rwsem_debug), GFP_KERNEL);
		/* add list */
		list_add(&rwsem_debug->list, &rwsem_debug_free_head.list);
		rwsem_debug_free_head_cnt++;
	}

	rwsem_debug_init = 1;
}

void debug_rwsemaphore_down_log(struct rw_semaphore *sem, int dir)
{
	struct list_head *tmp;
	struct rwsem_debug *sem_dbg;
	unsigned long flags;

	if (!rwsem_debug_init)
		return;

	spin_lock_irqsave(&rwsem_debug_lock, flags);
	list_for_each(tmp, &rwsem_debug_free_head.list) {
		sem_dbg = list_entry(tmp, struct rwsem_debug, list);
		sem_dbg->task = current;
		sem_dbg->sem = sem;
		/* strcpy(sem_dbg->comm,current->group_leader->comm); */
		sem_dbg->pid = current->pid;
		sem_dbg->cpu = smp_processor_id();
		sem_dbg->direction = dir;
		list_del(&sem_dbg->list);
		list_add(&sem_dbg->list, &rwsem_debug_done_head.list);
		rwsem_debug_free_head_cnt--;
		rwsem_debug_done_head_cnt++;
		break;
	}
	spin_unlock_irqrestore(&rwsem_debug_lock, flags);
}

void debug_rwsemaphore_up_log(struct rw_semaphore *sem)
{
	struct list_head *tmp;
	struct rwsem_debug *sem_dbg;
	unsigned long flags;

	if (!rwsem_debug_init)
		return;

	spin_lock_irqsave(&rwsem_debug_lock, flags);
	list_for_each(tmp, &rwsem_debug_done_head.list) {
		sem_dbg = list_entry(tmp, struct rwsem_debug, list);
		if (sem_dbg->sem == sem && sem_dbg->pid == current->pid) {
			list_del(&sem_dbg->list);
			list_add(&sem_dbg->list, &rwsem_debug_free_head.list);
			rwsem_debug_free_head_cnt++;
			rwsem_debug_done_head_cnt--;
			break;
		}
	}
	spin_unlock_irqrestore(&rwsem_debug_lock, flags);
}
#endif /* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

#ifdef CONFIG_SEC_DEBUG_USER
void sec_user_fault_dump(void)
{
	if (sec_debug_level.en.kernel_fault == 1 &&
	    sec_debug_level.en.user_fault == 1)
		panic("User Fault");
}

static int sec_user_fault_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';

	if (strncmp(buf, "dump_user_fault", 15) == 0)
		sec_user_fault_dump();

	return count;
}

static const struct file_operations sec_user_fault_proc_fops = {
	.write = sec_user_fault_write,
};

static int __init sec_debug_user_fault_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("user_fault", S_IWUGO, NULL,
			    &sec_user_fault_proc_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}

rootfs_initcall(sec_debug_user_fault_init);
#endif /* CONFIG_SEC_DEBUG_USER */
