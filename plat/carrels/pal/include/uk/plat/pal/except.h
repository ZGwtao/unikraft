/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_EXCEPT_H__
#define __UK_PLAT_PAL_EXCEPT_H__

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <uk/compiler.h>

/*
 * Event identifiers required by uk/pal/except.h.
 * They are software event IDs, not hardware IRQ numbers.
 */
#define UK_PAL_EXCEPT_EVENT_DEBUG		0
#define UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP	1
#define UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT	2
#define UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR	3
#define UK_PAL_EXCEPT_EVENT_ERR_MATH		4
#define UK_PAL_EXCEPT_EVENT_ERR_SECURITY	5
#define UK_PAL_EXCEPT_EVENT_SYSCALL		6
#define UK_PAL_EXCEPT_EVENT_IRQ			7
#define UK_PAL_EXCEPT_EVENT_UNHANDLED		8

/*
 * Definitions required before uk/pal/arch/except.h is included by
 * the generic PAL header.
 */
#define UK_PAL_ARM64_EXCEPT_ID_INVALID_OP \
	UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP
#define UK_PAL_ARM64_EXCEPT_ID_DEBUG \
	UK_PAL_EXCEPT_EVENT_DEBUG
#define UK_PAL_ARM64_EXCEPT_ID_PAGE_FAULT \
	UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT
#define UK_PAL_ARM64_EXCEPT_ID_BUS_ERROR \
	UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR
#define UK_PAL_ARM64_EXCEPT_ID_MATH \
	UK_PAL_EXCEPT_EVENT_ERR_MATH
#define UK_PAL_ARM64_EXCEPT_ID_SECURITY \
	UK_PAL_EXCEPT_EVENT_ERR_SECURITY
#define UK_PAL_ARM64_EXCEPT_ID_SYSCALL \
	UK_PAL_EXCEPT_EVENT_SYSCALL

#if !__ASSEMBLY__

extern volatile __u8 uk_carrels_irqs_disabled;
/*
 * Keep contexts opaque here. Their concrete representation is not needed
 * for the initial Microkit boot-only platform.
 */
struct uk_pal_except_err_ctx;
struct uk_pal_except_irq_ctx;
struct uk_pal_regs;

__isr static inline const char *
uk_pal_except_err_ctx_get_str(const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr static inline void
uk_pal_except_err_ctx_set_str(struct uk_pal_except_err_ctx *ctx,
			      const char *str)
{
	(void)ctx;
	(void)str;
}

__isr static inline int
uk_pal_except_err_ctx_get_handler_err(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr static inline void
uk_pal_except_err_ctx_set_handler_err(
	struct uk_pal_except_err_ctx *ctx,
	int handler_err)
{
	(void)ctx;
	(void)handler_err;
}

__isr static inline struct uk_pal_regs *
uk_pal_except_err_ctx_get_regs(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr static inline void
uk_pal_except_err_ctx_set_regs(
	struct uk_pal_except_err_ctx *ctx,
	struct uk_pal_regs *regs)
{
	(void)ctx;
	(void)regs;
}

__isr static inline __u64
uk_pal_except_err_ctx_get_fault_addr(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr static inline void
uk_pal_except_err_ctx_set_fault_addr(
	struct uk_pal_except_err_ctx *ctx,
	__u64 fault_addr)
{
	(void)ctx;
	(void)fault_addr;
}

__isr static inline struct uk_pal_regs *
uk_pal_except_irq_ctx_get_regs(
	const struct uk_pal_except_irq_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr static inline void
uk_pal_except_irq_ctx_set_regs(
	struct uk_pal_except_irq_ctx *ctx,
	struct uk_pal_regs *regs)
{
	(void)ctx;
	(void)regs;
}

__isr static inline __u64
uk_pal_except_irq_ctx_get_irq(
	const struct uk_pal_except_irq_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr static inline void
uk_pal_except_irq_ctx_set_irq(
	struct uk_pal_except_irq_ctx *ctx,
	__u32 irq)
{
	(void)ctx;
	(void)irq;
}

/*
 * Microkit PDs execute at EL0 and cannot manipulate the architectural
 * interrupt mask directly. For the current single-CPU CARRELS platform,
 * maintain the logical IRQ state expected by Unikraft in software.
 *
 * A non-zero value means logically disabled.
 */

__isr static inline void uk_pal_disable_irq(void)
{
	uk_carrels_irqs_disabled = 1;

	/*
	 * Prevent compiler reordering of accesses from after the logical
	 * interrupt-disable operation to before it.
	 */
	__asm__ volatile ("" ::: "memory");
}

__isr static inline void uk_pal_enable_irq(void)
{
	/*
	 * Complete accesses in the logical critical section before marking
	 * interrupts enabled.
	 */
	__asm__ volatile ("" ::: "memory");

	uk_carrels_irqs_disabled = 0;
}

__isr static inline int uk_pal_irqs_disabled(void)
{
	__asm__ volatile ("" ::: "memory");

	return uk_carrels_irqs_disabled != 0;
}

__isr static inline unsigned long uk_pal_save_irqf(void)
{
	unsigned long flags;

	flags = uk_carrels_irqs_disabled != 0;

	/*
	 * save_irqf() has save-and-disable semantics, not merely save
	 * semantics.
	 */
	uk_carrels_irqs_disabled = 1;

	__asm__ volatile ("" ::: "memory");

	return flags;
}

__isr static inline void uk_pal_restore_irqf(unsigned long flags)
{
	__asm__ volatile ("" ::: "memory");

	uk_carrels_irqs_disabled = flags ? 1 : 0;

	__asm__ volatile ("" ::: "memory");
}

__isr static inline void uk_pal_irqs_handle_pending(void)
{
}

__isr static inline void uk_pal_except_push_nested(void)
{
}

__isr static inline void uk_pal_except_pop_nested(void)
{
}

__isr static inline __uptr uk_pal_except_get_except_stack_base(void)
{
	return 0;
}

__isr static inline int uk_pal_except_init(void)
{
	return 0;
}

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_EXCEPT_H__ */