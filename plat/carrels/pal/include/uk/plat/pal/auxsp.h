/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_AUXSP_H__
#define __UK_PLAT_PAL_AUXSP_H__

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

#define UK_PAL_AUXSP_SYM uk_carrels_auxsp

#if !__ASSEMBLY__

extern __uk_pcpuvar __uptr uk_carrels_auxsp;

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_AUXSP_H__ */