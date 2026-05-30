/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __CARRELS_ARM64_IMAGE_H__
#define __CARRELS_ARM64_IMAGE_H__

#if CONFIG_CARRELS_VMM_DEFAULT
#define RAM_BASE_ADDR 0x2800000
#endif /* CONFIG_CARRELS_VMM_DEAULT  */

#define DTB_RESERVED_SIZE 0x100000

#endif /*  __CARRELS_ARM64_IMAGE_H__ */
