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

static inline void ukplat_carrels_set_timer_state(void)
{
	timer_ready = true;
}

static inline bool ukplat_carrels_check_timer_state(void)
{
	return timer_ready == true;
}

void ukplat_time_init(void)
{
	int rc;

	UK_ASSERT(timer_config_check_magic(&timer_config));

	timer_channel = timer_config.driver_id;

	rc = uk_sddf_timer_init(timer_channel);
	if (rc < 0)
		UK_CRASH("CARRELS: timer initialization failed: %d\n", rc);

	ukplat_carrels_set_timer_state();
}

__nsec ukplat_monotonic_clock(void)
{
	UK_ASSERT(ukplat_carrels_check_timer_state());
	return (__nsec)sddf_timer_time_now(timer_channel);
}

__nsec ukplat_wall_clock(void)
{
	// @gt ?? For features like time(), gettimeofday(), etc.,
	// we don't have a way to provide access to a UTC time.
	// This is just a dummy implementation to make sure the
	// hooks are instantiated.
	return ukplat_monotonic_clock();
}

static bool set_timeout_until(__nsec until)
{
	__nsec now = ukplat_monotonic_clock();
	if (now >= until)
		return false;

	// wrapper for the sddf timer interface
	uk_sddf_timer_set_timeout(until - now);
	return true;
}

/* This function is invoked only within schedcoop.c for simulating
 * a halting cpu, i.e., block the cpu for a period.
 * Ref: lib/ukschedcoop/schedcoop.c, L177 'idle_thread_fn'
 */
void time_block_until(__snsec until)
{
	seL4_Word timer_badge;
	UK_ASSERT(ukplat_carrels_check_timer_state());

	if (until <= 0)
		return;

	if (!set_timeout_until((__nsec)until))
		return;

	// wait specifically for timeout event
	timer_badge = (seL4_Word)1 << timer_channel;

	/* This is the only timeout request on the timer channel, so its
	 * notification marks this deadline.
	 */
	uk_carrels_microkit_wait_on(timer_badge);
}
