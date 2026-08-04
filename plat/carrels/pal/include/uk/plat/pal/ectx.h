/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_ECTX_H__
#define __UK_PLAT_PAL_ECTX_H__

#include <uk/essentials.h>

#define UK_PAL_ECTX_SIZE	16
#define UK_PAL_ECTX_ALIGN	16

#define UK_PAL_ECTX_LOAD_FNSYM		uk_pal_ectx_load
#define UK_PAL_ECTX_STORE_FNSYM		uk_pal_ectx_store
#define UK_PAL_ECTX_SANITIZE_FNSYM	uk_pal_ectx_sanitize

#if !__ASSEMBLY__

struct uk_pal_ectx {
	__u8 dummy[UK_PAL_ECTX_SIZE];
} __attribute__((aligned(UK_PAL_ECTX_ALIGN)));

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_ECTX_H__ */