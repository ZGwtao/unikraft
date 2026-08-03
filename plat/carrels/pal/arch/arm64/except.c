/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/plat/pal/except.h>
#include <uk/pal/except.h>

__isr int
uk_pal_arm64_except_err_ctx_get_eid(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;

	return UK_PAL_EXCEPT_EVENT_UNHANDLED;
}

__isr void
uk_pal_arm64_except_err_ctx_set_eid(
	struct uk_pal_except_err_ctx *ctx, int eid)
{
	(void)ctx;
	(void)eid;
}

__isr __u64
uk_pal_arm64_except_err_ctx_get_esr(
	const struct uk_pal_except_err_ctx *ctx)
{
	(void)ctx;

	return 0;
}

__isr void
uk_pal_arm64_except_err_ctx_set_esr(
	struct uk_pal_except_err_ctx *ctx, __u64 esr)
{
	(void)ctx;
	(void)esr;
}