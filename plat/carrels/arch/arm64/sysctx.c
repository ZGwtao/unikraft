/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/plat/carrels/arch/sysctx.h>

void uk_plat_carrels_sysctx_store(struct uk_pal_sysctx *sysctx)
{
	sysctx->tpidr_el0 = uk_plat_carrels_tlsp_get();
}

void uk_plat_carrels_sysctx_load(struct uk_pal_sysctx *sysctx)
{
	uk_plat_carrels_tlsp_set(sysctx->tpidr_el0);
}
