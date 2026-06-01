/* SPDX-License-Identifier: ISC */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Sergiu Moga <sergiu.moga@protonmail.com>
 *
 * Copyright (c) 2018 Arm Ltd.
 * Copyright (c) 2023 University Politehnica of Bucharest.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <uk/config.h>
#include <libfdt.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>
#if CONFIG_LIBUKACPI
#include <uk/acpi.h>
#endif /* CONFIG_LIBUKACPI */
#include <uk/lcpu.h>
#include <uk/assert.h>
#include <uk/boot.h>
#include <uk/intctlr.h>
#include <uk/arch/limits.h>
#include <uk/arch/util.h>
#include <uk/microkit.h>

#if CONFIG_ENFORCE_W_XOR_X && CONFIG_LIBUKPAGING
#include <uk/plat/common/w_xor_x.h>
#endif /* CONFIG_ENFORCE_W_XOR_X && CONFIG_LIBUKPAGING */

#ifdef CONFIG_ARM64_FEAT_PAUTH
#include <arm/arm64/pauth.h>
#endif /* CONFIG_ARM64_FEAT_PAUTH */

#ifdef CONFIG_HAVE_MEMTAG
#include <uk/arch/memtag.h>
#endif /* CONFIG_HAVE_MEMTAG */

#include <carrels/ossvc.h>

extern serial_client_config_t serial_config;

// __attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;

// serial_queue_handle_t serial_rx_queue_handle;
// serial_queue_handle_t serial_tx_queue_handle;

// interface per client payload
__attribute__((__section__(".pc_svc_desc"))) const protocon_svc_desc_t ciface = {
    /* numbers of each interface type */
    .t3_num = 1,
    /* type identifiers */
    .type3 = SERIAL_IFACE,
    /* pointer array of each interface type */
    .t3_iface = { (uintptr_t)&serial_config, 0, 0, 0, 0, 0, 0, 0 }
};


/* At this point we expect that the C runtime is configured and that
 * bootcode has enabled all CPU features used by compiled code.
 */
void __no_pauth _ukplat_entry(void)
{

	struct ukplat_bootinfo *bi;
	void *bstack;
	int rc;

	// FIXME(1): make sure the bootinfo for template PD is correct
	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Could not retrieve bootinfo\n");

	// FIXME(2): initialise uk_boot_earlytab_start?
#if 1
	uk_boot_early_init(bi);
#endif

#if 0
    assert(serial_config_check_magic(&serial_config));
    if (serial_config.rx.queue.vaddr != NULL) {
        serial_queue_init(&serial_rx_queue_handle, serial_config.rx.queue.vaddr, serial_config.rx.data.size, serial_config.rx.data.vaddr);
    }
    serial_queue_init(&serial_tx_queue_handle, serial_config.tx.queue.vaddr, serial_config.tx.data.size, serial_config.tx.data.vaddr);
    serial_putchar_init(serial_config.tx.id, &serial_tx_queue_handle);
#endif
    sddf_printf(
		"Powered by\n"
		"o.   .o       _ _               __ _\n"
		"Oo   Oo  ___ (_) | __ __  __ _ ' _) :_\n"
		"oO   oO ' _ `| | |/ /  _)' _` | |_|  _)\n"
		"oOo oOO| | | | |   (| | | (_) |  _) :_\n"
		" OoOoO ._, ._:_:_,\\_._,  .__,_:_, \\___)\n"
		"                 Ijiraq 0.21.0~f6f00b2d\n"
		"Hello from Unikraft!\n"
	);

	// while (1);

	// FIXME(4)
	// Stuck here

	// this is for some reason not working?
	uk_pr_info("Boot info retrieved successfully\n");

	sddf_printf("From sddf_printf: Boot info retrieved successfully\n");

	/* Allocate boot stack */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");
	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	// TODO: reach here
	// also get uk_pr_info working to print something here
	// *(int *)(0x90) = 1;
	while (1);

	/* Initialize paging */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize paging (%d)\n", rc);

#if CONFIG_ENFORCE_W_XOR_X && CONFIG_LIBUKPAGING
	enforce_w_xor_x();
#endif /* CONFIG_ENFORCE_W_XOR_X && CONFIG_LIBUKPAGING */

#ifdef CONFIG_ARM64_FEAT_PAUTH
	rc = ukplat_pauth_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize PAuth (%d)\n", rc);
#endif /* CONFIG_ARM64_FEAT_PAUTH */

#ifdef CONFIG_HAVE_MEMTAG
	rc = ukarch_memtag_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize MTE (%d)\n", rc);
#endif /* CONFIG_HAVE_MEMTAG */

#if CONFIG_LIBUKACPI
	rc = uk_acpi_init();
	if (unlikely(rc < 0))
		uk_pr_err("ACPI init failed: %d\n", rc);
#endif /* CONFIG_LIBUKACPI */

	/* Initialize interrupt controller */
	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Could not initialize the IRQ controller: %d\n", rc);

	/* Initialize logical boot CPU */
	rc = uk_lcpu_init(uk_lcpu_get_bsp());
	if (unlikely(rc))
		UK_CRASH("Failed to initialize bootstrapping CPU: %d\n", rc);

#ifdef CONFIG_HAVE_SMP
	rc = uk_lcpu_mp_init(CONFIG_LIBUKLCPU_RUN_IRQ,
			     CONFIG_LIBUKLCPU_WAKEUP_IRQ);
	if (unlikely(rc))
		UK_CRASH("SMP initialization failed: %d.\n", rc);
#endif /* CONFIG_HAVE_SMP */

	/*
	 * Switch away from the bootstrap stack as early as possible.
	 */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n", bstack);

	uk_arch_arm64_jump_to((__u64)bstack, (__u64)uk_boot_entry);
	uk_lcpu_halt();
}
