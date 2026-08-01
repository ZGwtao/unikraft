/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <uk/plat/common/_time.h>
#include <uk/plat/time.h>

#include <carrels/events.h>
#include <carrels/timer.h>

__nsec ukplat_monotonic_clock(void)
{
	if (!uk_carrels_timer_ready)
		return 0;

	return (__nsec)sddf_timer_time_now(
		uk_carrels_timer_channel
	);
}

__nsec ukplat_wall_clock(void)
{
	return ukplat_monotonic_clock();
}

void ukplat_time_init(void)
{
	UK_ASSERT(uk_carrels_timer_ready);
}

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
