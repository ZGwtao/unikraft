
#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

#include <uk/plat/pal/auxsp.h>
#include <uk/plat/time.h>

__uk_pcpuvar __uptr uk_carrels_auxsp;

__uk_pcpuvar __uptr uk_carrels_sentry;
__uk_pcpuvar __uptr uk_carrels_sstackp;
__uk_pcpuvar __uptr uk_carrels_sarg;

#include <uk/boot.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/memory.h>
#include <uk/paging.h>
#include <uk/assert.h>

#include <stdint.h>
#include <stddef.h>

#include <uk/sddf.h>

#include <carrels/events.h>
#include <uk/driver/sddf/serial.h>

__attribute__((__section__(".timer_client_config")))
timer_client_config_t timer_config;

#if CONFIG_LIBUKNETDEV
__attribute__((__section__(".net_client_config")))
net_client_config_t net_config;
#endif


/* Populated by Microkit from the heap map's setvar attributes. */
uintptr_t carrels_heap_base;
size_t carrels_heap_size;

/* Provide additional static heap memory region */
static void carrels_add_heap_mrd(struct ukplat_bootinfo *bi)
{
	// @gt ?? use setvar_vaddr & setvar_size to help you initalise
	// these two variables.
	// e.g., MAP(
    //     uk_heap, 0x200000000, perms="rw", cached="true",
    //     setvar_vaddr="carrels_heap_base", setvar_size="carrels_heap_size",
    // )
	if (!carrels_heap_base || !carrels_heap_size)
		UK_CRASH("CARRELS: heap map was not patched\n");

	if (!UK_PAGING_PAGE_ALIGNED(carrels_heap_base) ||
	    !UK_PAGING_PAGE_ALIGNED(carrels_heap_size))
		UK_CRASH("CARRELS: heap map is not page-aligned\n");

	if (carrels_heap_base > (uintptr_t)-1 - carrels_heap_size)
		UK_CRASH("CARRELS: heap map address overflows\n");

	// @gt ?? pbase is useless here, and unikraft in a PD
	// will not manage pagetable itself (making physical
	// address not even needed)
	struct ukplat_memregion_desc heap = {
		.pbase = carrels_heap_base,
		.vbase = carrels_heap_base,
		.pg_off = 0,
		.len = carrels_heap_size,
		.pg_count = UK_PAGING_PAGE_COUNT(carrels_heap_size),
		.type = UKPLAT_MEMRT_FREE,
		.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE,
	};
	int idx;

	idx = ukplat_memregion_list_insert(&bi->mrds, &heap);
	if (idx < 0) {
		sddf_printf("Failed to add heap MRD: rc=%d\n", idx);
		assert(0);
	}
}

__weak const char carrels_cmdline[] = "";

void init(void)
{
    assert(uk_sddf_serial_init() == 0);

	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	assert(bi != NULL);

	bi->cmdline = (__u64)(uintptr_t)carrels_cmdline;
	bi->cmdline_len = strlen(carrels_cmdline);

    carrels_add_heap_mrd(bi);

	/* This is the boot process provided by ukboot, which
	 * allows you to register a set of functions that should
	 * be executed earlier than the 'boot_entry' below (which
	 * contains process for initialising allocator/IRQ control/
	 * time/rootserver/everything)
	 * 
	 * Common things required here:
	 * - microkit events dispatcher
	 * - lcpu pm (halting mechanism)
	 * - serial
	 * - etc.
	 * 
	 * You can find out what's been initialised by searching
	 * the functions registered via 'uk_plat_initcall_prio'
	 */
	uk_boot_early_init(bi);

	uk_boot_entry();
}

// @gt ??
// Required by libmicrokit's link process
// Carrels never returns from init() and receives
// events through its scheduler-aware event path.
//
void notified(microkit_channel ch)
{
	(void)ch;
}
