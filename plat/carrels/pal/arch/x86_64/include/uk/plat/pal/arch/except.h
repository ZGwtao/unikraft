/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_EXCEPT_H__
#error "Do not include this header directly"
#endif

/* Dummy IDs: a Microkit PD receives faults as IPC, not CPU exceptions. */
#define UK_PAL_X86_64_EXCEPT_EVENT_ERR_GP_FAULT 0
#define UK_PAL_X86_64_EXCEPT_EVENT_NMI          1
#define UK_PAL_X86_64_EXCEPT_EVENT_DEBUG         2
#define UK_PAL_X86_64_EXCEPT_EVENT_ERR_INVALID_OP 3
#define UK_PAL_X86_64_EXCEPT_EVENT_ERR_PAGE_FAULT 4
#define UK_PAL_X86_64_EXCEPT_EVENT_ERR_BUS_ERROR 5
#define UK_PAL_X86_64_EXCEPT_EVENT_ERR_MATH      6
#define UK_PAL_X86_64_EXCEPT_EVENT_ERR_SECURITY  7
#define UK_PAL_X86_64_EXCEPT_EVENT_SYSCALL       8
#define UK_PAL_X86_64_EXCEPT_EVENT_IRQ           9
#define UK_PAL_X86_64_EXCEPT_EVENT_UNHANDLED     10

#define UK_PAL_ARCH_EXCEPT_EVENT_DEBUG \
	UK_PAL_X86_64_EXCEPT_EVENT_DEBUG
#define UK_PAL_ARCH_EXCEPT_EVENT_ERR_INVALID_OP \
	UK_PAL_X86_64_EXCEPT_EVENT_ERR_INVALID_OP
#define UK_PAL_ARCH_EXCEPT_EVENT_ERR_PAGE_FAULT \
	UK_PAL_X86_64_EXCEPT_EVENT_ERR_PAGE_FAULT
#define UK_PAL_ARCH_EXCEPT_EVENT_ERR_BUS_ERROR \
	UK_PAL_X86_64_EXCEPT_EVENT_ERR_BUS_ERROR
#define UK_PAL_ARCH_EXCEPT_EVENT_ERR_MATH \
	UK_PAL_X86_64_EXCEPT_EVENT_ERR_MATH
#define UK_PAL_ARCH_EXCEPT_EVENT_ERR_SECURITY \
	UK_PAL_X86_64_EXCEPT_EVENT_ERR_SECURITY
#define UK_PAL_ARCH_EXCEPT_EVENT_SYSCALL \
	UK_PAL_X86_64_EXCEPT_EVENT_SYSCALL
#define UK_PAL_ARCH_EXCEPT_EVENT_IRQ \
	UK_PAL_X86_64_EXCEPT_EVENT_IRQ
#define UK_PAL_ARCH_EXCEPT_EVENT_UNHANDLED \
	UK_PAL_X86_64_EXCEPT_EVENT_UNHANDLED

#if !__ASSEMBLY__
#include <uk/arch/types.h>

struct uk_pal_except_err_ctx;

__u32 uk_pal_x86_64_except_err_ctx_get_trapnr(
	const struct uk_pal_except_err_ctx *ctx);
void uk_pal_x86_64_except_err_ctx_set_trapnr(
	struct uk_pal_except_err_ctx *ctx, __u32 trapnr);
__u64 uk_pal_x86_64_except_err_ctx_get_error_code(
	const struct uk_pal_except_err_ctx *ctx);
void uk_pal_x86_64_except_err_ctx_set_error_code(
	struct uk_pal_except_err_ctx *ctx, __u64 error_code);
#endif
