/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_AUXSP_H__
#define __UK_PLAT_PAL_AUXSP_H__

#include <uk/plat/carrels/arch/auxsp.h>

#define UK_PAL_AUXSP_SYM UK_PLAT_CARRELS_AUXSP_SYM

#if !__ASSEMBLY__

static inline void uk_pal_set_auxsp(__uptr auxsp)
{
	uk_plat_carrels_set_auxsp(auxsp);
}

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_AUXSP_H__ */