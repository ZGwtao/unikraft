/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_CARRELS_ARCH_AUXSP_H__
#define __UK_PLAT_CARRELS_ARCH_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

#define UK_PLAT_CARRELS_AUXSP_SYM	uk_carrels_auxsp

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

/* defined in entry.c */
extern __uk_pcpuvar __uptr uk_carrels_auxsp;


static inline void uk_plat_carrels_set_auxsp(__uptr auxsp)
{
	uk_carrels_auxsp = auxsp;
}


#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_CARRELS_ARCH_AUXSP_H__ */
