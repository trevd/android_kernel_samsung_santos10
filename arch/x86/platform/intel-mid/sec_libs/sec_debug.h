/* arch/x86/platform/intel-mid/sec_libs/sec_debug.h
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

#ifndef SEC_DEBUG_H
#define SEC_DEBUG_H

#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/smp.h>

#define SEC_DEBUG_UPLOAD_CAUSE_ADDR	(0x0)
#define SEC_DEBUG_MAGIC_ADDR		0x4
#define SEC_REBOOT_CMD_ADDR			0x8
#define SEC_DEBUG_CP_CRASHSTR_ADDR	0xC

#define SEC_DEBUG_MAGIC_DUMP		0x66262564

#ifdef CONFIG_SEC_DEBUG

union sec_debug_level_t {
	struct {
		u16 kernel_fault;
		u16 user_fault;
	} en __aligned(sizeof(u16));
	u32 uint_val;
};

extern union sec_debug_level_t sec_debug_level;

struct sec_crash_key {
	unsigned int *keycode;	/* keycode array */
	unsigned int size;	/* number of used keycode */
	unsigned int timeout;	/* msec timeout */
	unsigned int unlock;	/* unlocking mask value */
	unsigned int trigger;	/* trigger key code */
	unsigned int knock;	/* number of triggered */
};

extern void sec_debug_init_crash_key(struct sec_crash_key *crash_key);

extern int sec_debug_get_level(void);
/* Exynos compatiable */
extern int get_sec_debug_level(void);

extern void sec_store_cp_crash_info(char *info, size_t len);
#else

#define sec_debug_init_crash_key(crash_key)
#define sec_debug_level()		0
#define sec_store_cp_crash_info(info, len)

#endif /* CONFIG_SEC_DEBUG */

#define IRQ_HANDLER_NONE		0
#define IRQ_HANDLER_ENTRY		1
#define IRQ_HANDLER_EXIT		2

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG

struct worker;
struct work_struct;

extern void __sec_debug_task_log(int cpu, struct task_struct *task);

extern void __sec_debug_irq_log(unsigned int irq, void *fn, int en);

extern void __sec_debug_ipc_log(u32 cmd, u32 sub, u8 *in, u32 inlen, u32 *out,
			u32 outlen, u32 dptr, u32 sptr, int en);

extern void __sec_debug_work_log(struct worker *worker,
		struct work_struct *work, work_func_t f, int en);

extern void __sec_debug_hrtimer_log(struct hrtimer *timer,
		enum hrtimer_restart (*fn) (struct hrtimer *), int en);

extern void __sec_debug_cpuidle_log(int state, int en);

extern void __sec_debug_wdtkick_regs_log(struct pt_regs *regs);

extern void __sec_debug_irq_regs_log(int cpu, struct pt_regs *regs);

#if defined(CONFIG_XEN)
extern void __sec_debug_xen_ipi_function_call_log(void *func, int en,
									struct call_single_data *data);
#endif
static inline void sec_debug_task_log(int cpu, struct task_struct *task)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_task_log(cpu, task);
}

static inline void sec_debug_irq_log(unsigned int irq, void *fn, int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_irq_log(irq, fn, en);
}

static inline void sec_debug_ipc_log(u32 cmd, u32 sub, u8 *in, u32 inlen, u32 *out,
				u32 outlen, u32 dptr, u32 sptr, int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_ipc_log(cmd, sub, in, inlen, out, outlen, dptr, sptr, en);
}


static inline void sec_debug_work_log(struct worker *worker,
		struct work_struct *work, work_func_t f, int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_work_log(worker, work, f, en);
}

static inline void sec_debug_hrtimer_log(struct hrtimer *timer,
		enum hrtimer_restart (*fn) (struct hrtimer *), int en)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_hrtimer_log(timer, fn, en);
}

static inline void sec_debug_cpuidle_log(int state, int en)
{

	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_cpuidle_log(state,en);
}


static inline void sec_debug_wdtkick_regs_log(struct pt_regs *regs)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_wdtkick_regs_log(regs);
}

static inline void sec_debug_irq_regs_log(int cpu, struct pt_regs *regs)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_irq_regs_log(cpu, regs);
}

#ifdef CONFIG_SEC_DEBUG_XEN
static inline void sec_debug_xen_ipi_function_call_log(void *func, int en,
									struct call_single_data *data)
{
	if (unlikely(sec_debug_level.en.kernel_fault))
		__sec_debug_xen_ipi_function_call_log(func, en, data);

}
extern inline void sec_debug_hypercall_log(int op, int en, int size,
		unsigned long arg1, unsigned long arg2, unsigned long arg3,
		unsigned long arg4, unsigned long arg5);
#endif

#ifdef CONFIG_SEC_DEBUG_IRQ_EXIT_LOG
extern void sec_debug_irq_last_exit_log(void);
#endif /* CONFIG_SEC_DEBUG_IRQ_EXIT_LOG */

#else

#define sec_debug_task_log(cpu, task)
#define sec_debug_irq_log(irq, fn, en)
#define sec_debug_work_log(worker, work, f, en)
#define sec_debug_hrtimer_log(timer, fn, en)
#define sec_debug_cpuidle_log(state, en)
#ifdef CONFIG_SEC_DEBUG_XEN
#define sec_debug_hypercall_log(type, argnum, arg1, arg2, arg3, arg4, arg5)
#define sec_debug_xen_ipi_function_call_log(func, en, data)
#endif
#define sec_debug_wdtkick_regs_log(regs)
#define sec_debug_irq_regs_log(cpu, regs)
#define sec_debug_ipc_log(cmd, sub, in, inlen, out, outlen, dptr, sptr, en)
#define sec_debug_irq_last_exit_log()

#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG

extern void debug_semaphore_init(void);

extern void debug_semaphore_down_log(struct semaphore *sem);

extern void debug_semaphore_up_log(struct semaphore *sem);

extern void debug_rwsemaphore_init(void);

extern void debug_rwsemaphore_down_log(struct rw_semaphore *sem, int dir);

extern void debug_rwsemaphore_up_log(struct rw_semaphore *sem);

#else

#define debug_semaphore_init()
#define debug_semaphore_down_log(sem)
#define debug_semaphore_up_log(sem)
#define debug_rwsemaphore_init()
#define debug_rwsemaphore_down_log(sem, dir)
#define debug_rwsemaphore_up_log(sem)

#endif /* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

#define debug_rwsemaphore_down_read_log(x)	\
	debug_rwsemaphore_down_log(x, READ_SEM)
#define debug_rwsemaphore_down_write_log(x)	\
	debug_rwsemaphore_down_log(x, WRITE_SEM)


#if defined(CONFIG_INTEL_SCU_IPC_UTIL_EMERG_OEMNIB)
extern void *intel_scu_ipc_get_oshob(void);

int intel_scu_ipc_emergency_write_oemnib(u8 *oemnib, int len, int offset);
#else
#define intel_scu_ipc_emergency_write_oemnib(oemnib, len, offset)
#endif

#endif /* SEC_DEBUG_H */
