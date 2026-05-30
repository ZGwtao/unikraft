/* SPDX-License-Identifier: BSD-3-Clause */

	.text
	.align 2

	.global uk_pal_ectx_load
	.type uk_pal_ectx_load, %function
uk_pal_ectx_load:
	ret
	.size uk_pal_ectx_load, . - uk_pal_ectx_load

	.global uk_pal_ectx_store
	.type uk_pal_ectx_store, %function
uk_pal_ectx_store:
	ret
	.size uk_pal_ectx_store, . - uk_pal_ectx_store

	.global uk_pal_ectx_sanitize
	.type uk_pal_ectx_sanitize, %function
uk_pal_ectx_sanitize:
	ret
	.size uk_pal_ectx_sanitize, . - uk_pal_ectx_sanitize


	.global uk_carrels_sysctx_load
	.type uk_carrels_sysctx_load, %function
uk_carrels_sysctx_load:
	/* x0 = struct uk_pal_sysctx * */
	ldr	x1, [x0, #0]
	msr	tpidr_el0, x1
	ret
	.size uk_carrels_sysctx_load, . - uk_carrels_sysctx_load

	.global uk_carrels_sysctx_store
	.type uk_carrels_sysctx_store, %function
uk_carrels_sysctx_store:
	/* x0 = struct uk_pal_sysctx * */
	mrs	x1, tpidr_el0
	str	x1, [x0, #0]
	ret
	.size uk_carrels_sysctx_store, . - uk_carrels_sysctx_store