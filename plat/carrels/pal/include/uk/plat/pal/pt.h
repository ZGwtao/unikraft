/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_PT_H__
#define __UK_PLAT_PAL_PT_H__

#include <uk/arch/types.h>

/*
 * AArch64 4 KiB translation granule.
 *
 * CARRELS does not own or modify the active page tables. These definitions
 * only describe their conventional shape so that generic paging headers and
 * page-based allocator helpers can compile.
 */
#define UK_PAL_PT_LEVELS		4
#define UK_PAL_PTES_PER_LEVEL		512
#define UK_PAL_PT_LEVEL_SHIFT		9

#if !__ASSEMBLY__

/*
 * Unikraft numbers levels in reverse:
 *   0: 4 KiB
 *   1: 2 MiB
 *   2: 1 GiB
 *   3: 512 GiB
 */
#define UK_PAL_PT_Lx_IDX(vaddr, lvl)				\
	(((__uptr)(vaddr) >> UK_PAL_PAGE_Lx_SHIFT(lvl))	\
	 & (UK_PAL_PTES_PER_LEVEL - 1UL))

#define UK_PAL_PT_Lx_PTES(lvl)					\
	((void)(lvl), UK_PAL_PTES_PER_LEVEL)

/*
 * Minimal AArch64 descriptor semantics.
 * Bit 0 is the valid/present bit.
 */
#define UK_PAL_PT_Lx_PTE_PRESENT(pte, lvl)			\
	((void)(lvl), (((__u64)(pte) & 0x1UL) != 0))

#define UK_PAL_PT_Lx_PTE_CLEAR_PRESENT(pte, lvl)		\
	((void)(lvl), ((__u64)(pte) & ~0x1UL))

#define UK_PAL_PT_Lx_PTE_INVALID(lvl)				\
	((void)(lvl), (__u64)0)

/*
 * AArch64 4 KiB descriptors store the output address in bits [47:12].
 * This is descriptive only; CARRELS does not traverse or write seL4 page
 * tables.
 */
#define UK_PAL_PT_PADDR_MASK		0x0000fffffffff000UL

#define UK_PAL_PT_Lx_PTE_PADDR(pte, lvl)			\
	((void)(lvl),						\
	 (__paddr_t)((__u64)(pte) & UK_PAL_PT_PADDR_MASK))

#define UK_PAL_PT_Lx_PTE_SET_PADDR(pte, lvl, paddr)		\
	((void)(lvl),						\
	 ((__u64)(pte) & ~UK_PAL_PT_PADDR_MASK) |		\
	 ((__u64)(paddr) & UK_PAL_PT_PADDR_MASK))

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_PT_H__ */