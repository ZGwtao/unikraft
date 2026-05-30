/* SPDX-License-Identifier: BSD-3-Clause */

int _ukplat_mem_mappings_init(void)
{
	return 0;
}

/*
 * The Microkit protection domain address space is already constructed by
 * seL4/Microkit. CARRELS has no boot page table to trim or replace.
 */
int ukplat_mem_init(void)
{
	return 0;
}