/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_TLB_H__
#define __UK_PLAT_PAL_TLB_H__

#include <uk/arch/types.h>

/*
 * CARRELS does not manage the Microkit PD page tables or TLB.
 *
 * The generic PAL declares the TLB API, but no CARRELS code should call
 * these functions while CONFIG_HAVE_PAGING is disabled.
 */

#endif /* __UK_PLAT_PAL_TLB_H__ */