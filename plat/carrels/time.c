/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/arch/types.h>
#include <uk/plat/time.h>
#include <uk/plat/common/_time.h>
#include <uk/microkit.h>

#include <carrels/events.h>
#include <carrels/timer.h>

void time_block_until(__snsec until)
{
	__nsec now;
	__nsec remaining;

	if (until <= 0)
		return;

	for (;;) {
		now = ukplat_monotonic_clock();
		if (now >= (__nsec)until)
			return;

		remaining = (__nsec)until - now;

		sddf_timer_set_timeout(uk_carrels_timer_channel, remaining);

		uk_carrels_microkit_wait();
	}
}

__nsec ukplat_wall_clock(void)
{
	return ukplat_monotonic_clock();
}