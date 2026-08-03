/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __UK_PLAT_CARRELS_ARCH_SYSCTX_H__
#define __UK_PLAT_CARRELS_ARCH_SYSCTX_H__

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

#define UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_TLSP 0
#define UK_PAL_CARRELS_ARCH_SYSCTX_SIZE          16

#if !__ASSEMBLY__

struct uk_pal_sysctx {
	__uptr tpidr_el0;
	__u8 pad[8];
};

UK_CTASSERT(sizeof(struct uk_pal_sysctx) ==
	    UK_PAL_CARRELS_ARCH_SYSCTX_SIZE);

UK_CTASSERT(__offsetof(struct uk_pal_sysctx, tpidr_el0) ==
	    UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_TLSP);

static inline __uptr uk_plat_carrels_tlsp_get(void)
{
	return UK_ARCH_ARM64_SYSREG_READ(tpidr_el0);
}

static inline void uk_plat_carrels_tlsp_set(__uptr tlsp)
{
	UK_ARCH_ARM64_SYSREG_WRITE(tpidr_el0, tlsp);
}

void uk_plat_carrels_sysctx_store(struct uk_pal_sysctx *sysctx);
void uk_plat_carrels_sysctx_load(struct uk_pal_sysctx *sysctx);

#endif /* !__ASSEMBLY__ */

#endif