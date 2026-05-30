/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <uk/asm/lcpu.h>
#include <uk/arch/limits.h>
#include <uk/asm.h>
#include <uk/plat/common/sections.h>
#include <uk/config.h>
#include <uk/reloc.h>

#define BOOTSTACK_SIZE	4096

/* Prefer using in-image stack, to avoid conflicts when unmapping is done by
 * paging_init (it invalidates our stack)
 */
.section .bss
.space BOOTSTACK_SIZE
lcpu_bootstack:

.text
ENTRY(_lib_carrels_plat_entry)
	b	_ukplat_entry
END(_lib_carrels_plat_entry)

.align 4
.global bpt_unmap_mrd
bpt_unmap_mrd:
	.quad	0x0000000000000000		/* 0 GiB */
	.quad	0x0000000000000000		/* 0 GiB */
	.quad	0x0000000000000000		/* Page-aligned */
	.quad	0x0000000100000000		/* 4 GiB */
	.quad	0x0000000000100000		/* Page count */
	.short	0x0000000000000000
	.short	0x0000000000000010		/* UKPLAT_MEMRF_UNMAP */
	.fill	36, 1, 0
