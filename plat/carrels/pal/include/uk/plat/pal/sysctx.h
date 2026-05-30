/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_SYSCTX_H__
#define __UK_PLAT_PAL_SYSCTX_H__

#include <uk/arch.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>

#define UK_PAL_SYSCTX_OFFSETOF_TLSP	0
#define UK_PAL_SYSCTX_SIZE		16

#define UK_PAL_SYSCTX_LOAD_FNSYM	uk_carrels_sysctx_load
#define UK_PAL_SYSCTX_STORE_FNSYM	uk_carrels_sysctx_store

#if !__ASSEMBLY__

#ifdef __cplusplus
extern "C" {
#endif

struct uk_pal_sysctx {
	__uptr tpidr_el0;
	__u8 pad[8];
};

UK_CTASSERT(sizeof(struct uk_pal_sysctx) == UK_PAL_SYSCTX_SIZE);
UK_CTASSERT(__offsetof(struct uk_pal_sysctx, tpidr_el0) ==
	    UK_PAL_SYSCTX_OFFSETOF_TLSP);

__isr static inline __u64
uk_pal_sysctx_get(const struct uk_pal_sysctx *sc, __sz offset)
{
	return *(__u64 *)((const char *)sc + offset);
}

__isr static inline void
uk_pal_sysctx_set(struct uk_pal_sysctx *sc, __sz offset, __u64 val)
{
	*(__u64 *)((char *)sc + offset) = val;
}

__isr static inline __uptr uk_pal_tlsp_get(void)
{
	return UK_ARCH_ARM64_SYSREG_READ(tpidr_el0);
}

__isr static inline void uk_pal_tlsp_set(__uptr tlsp)
{
	UK_ARCH_ARM64_SYSREG_WRITE(tpidr_el0, tlsp);
}

__isr static inline void
uk_carrels_sysctx_store(struct uk_pal_sysctx *sysctx)
{
	sysctx->tpidr_el0 = uk_pal_tlsp_get();
}

__isr static inline void
uk_carrels_sysctx_load(struct uk_pal_sysctx *sysctx)
{
	uk_pal_tlsp_set(sysctx->tpidr_el0);
}

__isr static inline void
uk_pal_sysctx_store(struct uk_pal_sysctx *sysctx)
{
	uk_carrels_sysctx_store(sysctx);
}

__isr static inline void
uk_pal_sysctx_load(struct uk_pal_sysctx *sysctx)
{
	uk_carrels_sysctx_load(sysctx);
}

#ifdef __cplusplus
}
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_SYSCTX_H__ */