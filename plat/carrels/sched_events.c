/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/init.h>
#include <uk/print.h>
#include <uk/sched.h>

#include <carrels/events.h>

static int carrels_sched_poll_init(void)
{
	int rc;

	rc = uk_sched_platform_poll_register(
		uk_carrels_microkit_poll
	);
	if (rc < 0)
		uk_pr_err("CARRELS: failed to register scheduler event poll: %d\n",
			  rc);

	return rc;
}

uk_plat_initcall_prio(carrels_sched_poll_init, 0, UK_PRIO_LATEST);