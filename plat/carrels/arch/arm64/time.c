/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <uk/plat/common/_time.h>
#include <uk/plat/time.h>
#include <uk/sddf.h>
#include <uk/sddf/timer.h>

#include <carrels/events.h>

extern timer_client_config_t timer_config;

static sddf_channel timer_channel;
static bool timer_ready;

void ukplat_time_init(void)
{
	int rc;

	UK_ASSERT(timer_config_check_magic(&timer_config));

	timer_channel = timer_config.driver_id;

	rc = uk_sddf_timer_init(timer_channel);
	if (rc < 0)
		UK_CRASH("CARRELS: timer initialization failed: %d\n", rc);

	timer_ready = true;
}

__nsec ukplat_monotonic_clock(void)
{
	UK_ASSERT(timer_ready);

	return (__nsec)sddf_timer_time_now(timer_channel);
}

__nsec ukplat_wall_clock(void)
{
	return ukplat_monotonic_clock();
}

void time_block_until(__snsec until)
{
	__nsec now;
	__nsec remaining;

	UK_ASSERT(timer_ready);

	if (until <= 0)
		return;

	for (;;) {
		now = ukplat_monotonic_clock();
		if (now >= (__nsec)until)
			return;

		remaining = (__nsec)until - now;

		uk_sddf_timer_set_timeout(remaining);
		uk_carrels_microkit_wait();
	}
}