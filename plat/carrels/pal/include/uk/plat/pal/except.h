/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_EXCEPT_H__
#define __UK_PLAT_PAL_EXCEPT_H__

/* this file is a dummy file because PDs have no except */

#include <uk/plat/pal/arch/except.h>

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <uk/compiler.h>

#define UK_PAL_EXCEPT_EVENT_DEBUG \
	UK_PAL_ARCH_EXCEPT_EVENT_DEBUG
#define UK_PAL_EXCEPT_EVENT_ERR_INVALID_OP	\
	UK_PAL_ARCH_EXCEPT_EVENT_ERR_INVALID_OP
#define UK_PAL_EXCEPT_EVENT_ERR_PAGE_FAULT \
	UK_PAL_ARCH_EXCEPT_EVENT_ERR_PAGE_FAULT
#define UK_PAL_EXCEPT_EVENT_ERR_BUS_ERROR \
	UK_PAL_ARCH_EXCEPT_EVENT_ERR_BUS_ERROR
#define UK_PAL_EXCEPT_EVENT_ERR_MATH \
	UK_PAL_ARCH_EXCEPT_EVENT_ERR_MATH
#define UK_PAL_EXCEPT_EVENT_ERR_SECURITY \
	UK_PAL_ARCH_EXCEPT_EVENT_ERR_SECURITY
#define UK_PAL_EXCEPT_EVENT_SYSCALL	\
	UK_PAL_ARCH_EXCEPT_EVENT_SYSCALL
#define UK_PAL_EXCEPT_EVENT_IRQ	\
	UK_PAL_ARCH_EXCEPT_EVENT_IRQ
#define UK_PAL_EXCEPT_EVENT_UNHANDLED \
	UK_PAL_ARCH_EXCEPT_EVENT_UNHANDLED

#if !__ASSEMBLY__

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

__isr static inline void uk_pal_disable_irq(void) {}
__isr static inline void uk_pal_enable_irq(void) {}
__isr static inline void uk_pal_restore_irqf(unsigned long flags) {}
__isr static inline void uk_pal_irqs_handle_pending(void) {}
__isr static inline void uk_pal_except_push_nested(void) {}
__isr static inline void uk_pal_except_pop_nested(void) {}

__isr static inline int uk_pal_irqs_disabled(void)
{
	/* the only usage is assert(irqs_disabled()); */
	return 1;
}

__isr static inline unsigned long uk_pal_save_irqf(void)
{
	/* dummy value */
	return 0;
}


__isr static inline __uptr uk_pal_except_get_except_stack_base(void)
{
	/* dummy value */
	return 0;
}

__isr static inline int uk_pal_except_init(void)
{
	/* dummy value */
	return 0;
}

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_EXCEPT_H__ */