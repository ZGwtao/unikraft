/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Keep a translation unit for the architecture PAL, mirroring arm64.  All
 * exception operations are deliberately no-ops: CPU faults belong to
 * seL4/Microkit and are not delivered as Unikraft exception frames.
 */
#include <uk/plat/pal/except.h>

__isr __u32 uk_pal_x86_64_except_err_ctx_get_trapnr(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr void uk_pal_x86_64_except_err_ctx_set_trapnr(
	struct uk_pal_except_err_ctx *ctx, __u32 trapnr)
{
	(void)ctx;
	(void)trapnr;
}

__isr __u64 uk_pal_x86_64_except_err_ctx_get_error_code(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__isr void uk_pal_x86_64_except_err_ctx_set_error_code(
	struct uk_pal_except_err_ctx *ctx, __u64 error_code)
{
	(void)ctx;
	(void)error_code;
}
