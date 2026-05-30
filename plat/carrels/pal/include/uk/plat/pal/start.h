/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_START_H__
#define __UK_PLAT_PAL_START_H__

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

#define UK_PAL_SENTRY_SYM	uk_carrels_sentry
#define UK_PAL_SSTACKP_SYM	uk_carrels_sstackp
#define UK_PAL_SARG_SYM		uk_carrels_sarg

#if !__ASSEMBLY__

#ifdef __cplusplus
extern "C" {
#endif

extern __uk_pcpuvar __uptr uk_carrels_sentry;
extern __uk_pcpuvar __uptr uk_carrels_sstackp;
extern __uk_pcpuvar __uptr uk_carrels_sarg;

#ifdef __cplusplus
}
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_START_H__ */