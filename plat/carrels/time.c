/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/arch/types.h>
#include <uk/plat/time.h>
#include <uk/plat/common/_time.h>

/*
 * Temporary CARRELS implementation.
 *
 * Microkit notifications are not yet integrated into the Unikraft
 * scheduler, so waiting for a deadline currently polls the SDDF-backed
 * monotonic clock.
 *
 * The caller enters with Unikraft's logical IRQ state disabled. This
 * function must not modify that logical state.
 */
void time_block_until(__snsec until)
{
	/*
	 * A negative deadline is already expired.
	 */
	if (until <= 0)
		return;

	while ((__snsec)ukplat_monotonic_clock() < until)
		__asm__ volatile ("" ::: "memory");
}