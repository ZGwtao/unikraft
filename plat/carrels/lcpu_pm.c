/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/init.h>
#include <uk/lcpu/pm.h>
#include <uk/microkit.h>

#include <carrels/events.h>


static void carrels_halt_irq(void)
{
	uk_carrels_microkit_wait();
}

static const struct uk_lcpu_pm_ops carrels_lcpu_pm_ops = {
	.halt_irq = carrels_halt_irq,
};

static int carrels_lcpu_pm_init(void)
{
	int rc;

	rc = uk_lcpu_pm_ops_register(&carrels_lcpu_pm_ops);
	if (rc < 0)
		sddf_printf("CARRELS: failed to register lcpu PM ops: %d\n",
			    rc);

	return rc;
}

uk_plat_initcall_prio(carrels_lcpu_pm_init, 0, UK_PRIO_LATEST);