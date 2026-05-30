/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_ADDR_H__
#define __UK_PLAT_PAL_ADDR_H__

#include <uk/arch/types.h>

#if !__ASSEMBLY__

/*
 * CARRELS does not expose or manage physical addresses.
 * Use page-aligned sentinel values for unsupported/unspecified addresses.
 */
#define UK_PAL_VADDR_INV ((__vaddr_t)~0xfffUL)
#define UK_PAL_PADDR_INV ((__paddr_t)~0xfffUL)

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_ADDR_H__ */