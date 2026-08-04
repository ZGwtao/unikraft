/* SPDX-License-Identifier: BSD-3-Clause */
/* Stolen from plat/xen/pal/include/uk/plat/pal/pt.h */
/*
 * The pt.h means nothing for Carrels, as demand paging is not supported.
 * The purpose of this file is to supporting the compilation of libukalloc
 */

#ifndef __UK_PLAT_PAL_PT_H__
#define __UK_PLAT_PAL_PT_H__

#include <uk/arch/types.h>
#include <carrels/pt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UK_PAL_PT_LEVELS		UK_PLAT_CARRELS_PT_LEVELS
#define UK_PAL_PTES_PER_LEVEL	UK_PLAT_CARRELS_PTES_PER_LEVEL
#define UK_PAL_PT_LEVEL_SHIFT	UK_PLAT_CARRELS_PT_LEVEL_SHIFT

#if !__ASSEMBLY__

#define UK_PAL_PT_Lx_IDX(vaddr, lvl)			    \
	UK_PLAT_CARRELS_PT_Lx_IDX(vaddr, lvl)

#define UK_PAL_PT_Lx_PTES(lvl)				    \
	UK_PLAT_CARRELS_PT_Lx_PTES(lvl)

#define UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)		    \
	UK_PLAT_CARRELS_PT_Lx_PTE_PRESENT(pte, lvl)

#define UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)	    \
	UK_PLAT_CARRELS_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)

#define UK_PAL_PT_Lx_PTE_INVALID(lvl)			    \
	UK_PLAT_CARRELS_PT_Lx_PTE_INVALID(lvl)

#define UK_PAL_PT_Lx_PTE_PADDR(pte, lvl)		    \
	UK_PLAT_CARRELS_PT_Lx_PTE_PADDR(pte, lvl)

#define UK_PAL_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)	    \
	UK_PLAT_CARRELS_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)

/* Carrels platform does not currently support paging, do not define pt(e) ops */

#endif /* !__ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __UK_PLAT_PAL_PT_H__ */
