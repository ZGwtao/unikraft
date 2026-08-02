/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <uk/plat/common/_time.h>
#include <uk/plat/time.h>

#include <carrels/events.h>
#include <carrels/timer.h>

volatile __u64 uk_carrels_timer_event_count;

static void carrels_timer_event_handler(microkit_channel ch, void *arg)
{
	(void)arg;

	UK_ASSERT(ch == uk_carrels_timer_channel);

	uk_carrels_timer_event_count++;
}

int uk_carrels_timer_event_init(void)
{
	int rc;

	UK_ASSERT(uk_carrels_timer_ready);

	rc = carrels_event_register(uk_carrels_timer_channel,
				    carrels_timer_event_handler,
				    NULL);
	if (rc < 0)
		uk_pr_err("CARRELS: failed to register timer channel %u: %d\n",
			  (unsigned int)uk_carrels_timer_channel, rc);

	return rc;
}

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
