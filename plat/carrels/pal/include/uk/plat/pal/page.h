/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_PAGE_H__
#define __UK_PLAT_PAL_PAGE_H__

#include <uk/arch/types.h>

/*
 * AArch64, 4 KiB translation granule.
 *
 * CARRELS does not manage the PD page tables. These definitions only
 * describe page sizes required by ukalloc and the generic paging API.
 *
 * Level numbering follows Unikraft:
 *   level 0: 4 KiB
 *   level 1: 2 MiB
 *   level 2: 1 GiB
 *   level 3: 512 GiB
 */

#define UK_PAL_PAGE_LEVEL		0
#define UK_PAL_PAGE_SHIFT		12
#define UK_PAL_PAGE_SIZE		(1UL << UK_PAL_PAGE_SHIFT)
#define UK_PAL_PAGE_MASK		(~(UK_PAL_PAGE_SIZE - 1UL))

#define UK_PAL_PAGE_LARGE_LEVEL		1
#define UK_PAL_PAGE_LARGE_SHIFT		21
#define UK_PAL_PAGE_LARGE_SIZE		(1UL << UK_PAL_PAGE_LARGE_SHIFT)
#define UK_PAL_PAGE_LARGE_MASK		(~(UK_PAL_PAGE_LARGE_SIZE - 1UL))

#define UK_PAL_PAGE_HUGE_LEVEL		2
#define UK_PAL_PAGE_HUGE_SHIFT		30
#define UK_PAL_PAGE_HUGE_SIZE		(1UL << UK_PAL_PAGE_HUGE_SHIFT)
#define UK_PAL_PAGE_HUGE_MASK		(~(UK_PAL_PAGE_HUGE_SIZE - 1UL))

/*
 * For 4 KiB granules, each page-table level adds nine address bits.
 */
#define UK_PAL_PAGE_Lx_SHIFT(lvl)	\
	(UK_PAL_PAGE_SHIFT + (9UL * (lvl)))

#define UK_PAL_PAGE_SHIFT_Lx(shift)	\
	(((shift) - UK_PAL_PAGE_SHIFT) / 9UL)

#define UK_PAL_PAGE_Lx_SIZE(lvl)	\
	(1UL << UK_PAL_PAGE_Lx_SHIFT(lvl))

#define UK_PAL_PAGE_Lx_MASK(lvl)	\
	(~(UK_PAL_PAGE_Lx_SIZE(lvl) - 1UL))

#define UK_PAL_PAGE_Lx_ALIGN_UP(val, lvl)			\
	((((__uptr)(val)) + UK_PAL_PAGE_Lx_SIZE(lvl) - 1UL)	\
	 & UK_PAL_PAGE_Lx_MASK(lvl))

#define UK_PAL_PAGE_Lx_ALIGN_DOWN(val, lvl)	\
	((__uptr)(val) & UK_PAL_PAGE_Lx_MASK(lvl))

#define UK_PAL_PAGE_Lx_ALIGNED(val, lvl)	\
	((((__uptr)(val)) &			\
	  (UK_PAL_PAGE_Lx_SIZE(lvl) - 1UL)) == 0)

#define UK_PAL_PAGE_ALIGN_UP(val)	\
	UK_PAL_PAGE_Lx_ALIGN_UP(val, UK_PAL_PAGE_LEVEL)

#define UK_PAL_PAGE_ALIGN_DOWN(val)	\
	UK_PAL_PAGE_Lx_ALIGN_DOWN(val, UK_PAL_PAGE_LEVEL)

#define UK_PAL_PAGE_ALIGNED(val)	\
	UK_PAL_PAGE_Lx_ALIGNED(val, UK_PAL_PAGE_LEVEL)

#define UK_PAL_PAGE_LARGE_ALIGN_UP(val)	\
	UK_PAL_PAGE_Lx_ALIGN_UP(val, UK_PAL_PAGE_LARGE_LEVEL)

#define UK_PAL_PAGE_LARGE_ALIGN_DOWN(val)	\
	UK_PAL_PAGE_Lx_ALIGN_DOWN(val, UK_PAL_PAGE_LARGE_LEVEL)

#define UK_PAL_PAGE_LARGE_ALIGNED(val)	\
	UK_PAL_PAGE_Lx_ALIGNED(val, UK_PAL_PAGE_LARGE_LEVEL)

#define UK_PAL_PAGE_HUGE_ALIGN_UP(val)	\
	UK_PAL_PAGE_Lx_ALIGN_UP(val, UK_PAL_PAGE_HUGE_LEVEL)

#define UK_PAL_PAGE_HUGE_ALIGN_DOWN(val)	\
	UK_PAL_PAGE_Lx_ALIGN_DOWN(val, UK_PAL_PAGE_HUGE_LEVEL)

#define UK_PAL_PAGE_HUGE_ALIGNED(val)	\
	UK_PAL_PAGE_Lx_ALIGNED(val, UK_PAL_PAGE_HUGE_LEVEL)

/*
 * Generic protection flags. They describe requested permissions only;
 * CARRELS does not modify Microkit mappings with them.
 */
#define UK_PAL_PAGE_ATTR_PROT_NONE	0x0UL
#define UK_PAL_PAGE_ATTR_PROT_READ	0x1UL
#define UK_PAL_PAGE_ATTR_PROT_WRITE	0x2UL
#define UK_PAL_PAGE_ATTR_PROT_EXEC	0x4UL

#if !__ASSEMBLY__

/*
 * AArch64 with a 4 KiB granule can use block/page descriptors at
 * levels 0, 1 and 2 in Unikraft's reversed level numbering.
 */
#define UK_PAL_PAGE_Lx_HAS(lvl)	\
	((lvl) <= UK_PAL_PAGE_HUGE_LEVEL)

/*
 * CARRELS does not inspect or construct page-table entries.
 * This macro exists only to satisfy the PAL contract while paging is off.
 */
#define UK_PAL_PAGE_Lx_IS(pte, lvl)	\
	((void)(pte), (void)(lvl), 0)

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_PAGE_H__ */