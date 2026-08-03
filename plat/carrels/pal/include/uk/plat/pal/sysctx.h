/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_SYSCTX_H__
#define __UK_PLAT_PAL_SYSCTX_H__

#include <uk/plat/carrels/arch/sysctx.h>

#define UK_PAL_SYSCTX_LOAD_FNSYM  uk_plat_carrels_sysctx_load
#define UK_PAL_SYSCTX_STORE_FNSYM uk_plat_carrels_sysctx_store

#define UK_PAL_SYSCTX_OFFSETOF_TLSP \
	UK_PAL_CARRELS_ARCH_SYSCTX_OFFSETOF_TLSP
#define UK_PAL_SYSCTX_SIZE \
	UK_PAL_CARRELS_ARCH_SYSCTX_SIZE

#if !__ASSEMBLY__

static inline __u64
uk_pal_sysctx_get(const struct uk_pal_sysctx *sc, __sz offset)
{
	return *(__u64 *)((const char *)sc + offset);
}

static inline void
uk_pal_sysctx_set(struct uk_pal_sysctx *sc, __sz offset, __u64 val)
{
	*(__u64 *)((char *)sc + offset) = val;
}

static inline __uptr uk_pal_tlsp_get(void)
{
	return uk_plat_carrels_tlsp_get();
}

static inline void uk_pal_tlsp_set(__uptr tlsp)
{
	uk_plat_carrels_tlsp_set(tlsp);
}

static inline void
uk_pal_sysctx_store(struct uk_pal_sysctx *sysctx)
{
	uk_plat_carrels_sysctx_store(sysctx);
}

static inline void
uk_pal_sysctx_load(struct uk_pal_sysctx *sysctx)
{
	uk_plat_carrels_sysctx_load(sysctx);
}

#endif /* !__ASSEMBLY__ */

#endif