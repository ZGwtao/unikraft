
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

#include <uk/sddf.h>

#include <carrels/events.h>
#include <uk/driver/sddf/serial.h>

__attribute__((__section__(".timer_client_config")))
timer_client_config_t timer_config;

#if CONFIG_LIBUKNETDEV
__attribute__((__section__(".net_client_config")))
net_client_config_t net_config;
#endif

sddf_channel uk_carrels_timer_channel;
bool uk_carrels_timer_ready;

#define CARRELS_HEAP_BASE 0x200000000UL
#define CARRELS_HEAP_SIZE 0x4000000UL

static void carrels_add_heap_mrd(struct ukplat_bootinfo *bi)
{
	struct ukplat_memregion_desc heap = {
		.pbase = CARRELS_HEAP_BASE,
		.vbase = CARRELS_HEAP_BASE,
		.pg_off = 0,
		.len = CARRELS_HEAP_SIZE,
		.pg_count = UK_PAGING_PAGE_COUNT(CARRELS_HEAP_SIZE),
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

	uk_boot_early_init(bi);

	uk_carrels_microkit_poll_enable();

	uk_boot_entry();
}

void notified(microkit_channel ch)
{
	carrels_event_dispatch(ch);
}
