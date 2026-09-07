/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_CARRELS_ARCH_SYSCTX_H__
#define __UK_PLAT_CARRELS_ARCH_SYSCTX_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

#define UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_GSBASE 0
#define UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_FSBASE 8
#define UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_TLSP \
	UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_FSBASE
#define UK_PAL_CARRELS_ARCH_SYSCTX_SIZE 16

#if !__ASSEMBLY__
struct uk_pal_sysctx {
	__u64 gsbase;
	__u64 fsbase;
};

UK_CTASSERT(sizeof(struct uk_pal_sysctx) == UK_PAL_CARRELS_ARCH_SYSCTX_SIZE);
UK_CTASSERT(__offsetof(struct uk_pal_sysctx, gsbase) ==
	    UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_GSBASE);
UK_CTASSERT(__offsetof(struct uk_pal_sysctx, fsbase) ==
	    UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_FSBASE);

/*
 * seL4 exposes FSGSBASE to x86_64 protection domains.  These instructions
 * are unprivileged when the kernel enables CR4.FSGSBASE, unlike rdmsr/wrmsr.
 */
static inline __uptr uk_plat_carrels_tlsp_get(void)
{
	__uptr fsbase;

	__asm__ volatile ("rdfsbase %0" : "=r" (fsbase));
	return fsbase;
}

static inline void uk_plat_carrels_tlsp_set(__uptr tlsp)
{
	__asm__ volatile ("wrfsbase %0" : : "r" (tlsp));
}

void uk_plat_carrels_sysctx_store(struct uk_pal_sysctx *sysctx);
void uk_plat_carrels_sysctx_load(struct uk_pal_sysctx *sysctx);
#endif

#endif /* __UK_PLAT_CARRELS_ARCH_SYSCTX_H__ */
