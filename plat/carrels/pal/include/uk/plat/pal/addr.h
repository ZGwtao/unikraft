/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_ADDR_H__
#define __UK_PLAT_PAL_ADDR_H__

/* stolen from Xen */
#include <uk/plat/native/addr.h>

#if !__ASSEMBLY__

#define UK_PAL_VADDR_INV UK_PLAT_NATIVE_VADDR_INV
#define UK_PAL_PADDR_INV UK_PLAT_NATIVE_PADDR_INV

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_ADDR_H__ */