/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/assert.h>
#include <uk/print.h>
#include <uk/sddf.h>

#include <carrels/events.h>

static sddf_channel timer_channel;
static bool timer_initialized;
static volatile __u64 timer_event_count;

static void sddf_timer_event_handler(microkit_channel ch, void *arg)
{
	(void)arg;

	UK_ASSERT(timer_initialized);
	UK_ASSERT(ch == timer_channel);

	timer_event_count++;
}

int uk_sddf_timer_init(sddf_channel channel)
{
	int rc;

	if (timer_initialized) {
		UK_ASSERT(timer_channel == channel);
		return 0;
	}

	rc = carrels_event_register(channel,
				    sddf_timer_event_handler,
				    NULL);
	if (rc < 0) {
		uk_pr_err("sDDF timer: failed to register channel %u: %d\n",
			  (unsigned int)channel, rc);
		return rc;
	}

	timer_channel = channel;
	timer_initialized = true;

	return 0;
}

void uk_sddf_timer_set_timeout(__nsec duration)
{
	UK_ASSERT(timer_initialized);

	sddf_timer_set_timeout(timer_channel, duration);
}

__u64 uk_sddf_timer_event_count(void)
{
	return timer_event_count;
}