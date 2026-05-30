#include <microkit.h>

#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

#include <uk/plat/pal/auxsp.h>
#include <uk/plat/time.h>

__uk_pcpuvar __uptr uk_carrels_auxsp;

__uk_pcpuvar __uptr uk_carrels_sentry;
__uk_pcpuvar __uptr uk_carrels_sstackp;
__uk_pcpuvar __uptr uk_carrels_sarg;

void uk_pal_set_auxsp(__uptr auxsp)
{
	uk_carrels_auxsp = auxsp;
}

volatile __u8 uk_carrels_irqs_disabled = 1;

#include <uk/boot.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/memory.h>
#include <uk/paging.h>

#include <sddf/timer/client.h>
#include <sddf/timer/config.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>

#include <sddf/util/printf.h>

// __attribute__((__section__(".serial_client_config")))
extern serial_client_config_t serial_config;
__attribute__((__section__(".timer_client_config")))
timer_client_config_t timer_config;

serial_queue_handle_t serial_rx_queue_handle;
serial_queue_handle_t serial_tx_queue_handle;

sddf_channel timer_channel;
static int timer_ready;

__nsec ukplat_monotonic_clock(void)
{
	if (!timer_ready)
		return 0;

	return (__nsec)sddf_timer_time_now(timer_channel);
}

#define CARRELS_HEAP_BASE 0x500000UL
#define CARRELS_HEAP_SIZE 0x200000UL

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

	sddf_printf("Heap MRD[%d]: vbase=%lx len=%lx\n",
		    idx,
		    (unsigned long)heap.vbase,
		    (unsigned long)heap.len);
}

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    if (serial_config.rx.queue.vaddr != NULL) {
        serial_queue_init(&serial_rx_queue_handle,
                          serial_config.rx.queue.vaddr,
                          serial_config.rx.data.size,
                          serial_config.rx.data.vaddr);
    }
    serial_queue_init(&serial_tx_queue_handle,
                      serial_config.tx.queue.vaddr,
                      serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);
    serial_putchar_init(serial_config.tx.id, &serial_tx_queue_handle);

    assert(timer_config_check_magic(&timer_config));

    timer_channel = timer_config.driver_id;
    timer_ready = 1;

	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	assert(bi != NULL);

    carrels_add_heap_mrd(bi);

	sddf_printf("CARRELS: uk_boot_early_init\n");
	uk_boot_early_init(bi);

	sddf_printf("CARRELS: uk_boot_entry\n");
	uk_boot_entry();
}

void notified(microkit_channel ch)
{
    if (ch == timer_channel) {
        sddf_timer_set_timeout(timer_channel, NS_IN_S * 2);
        uint64_t time = sddf_timer_time_now(timer_channel);
        sddf_printf("CLIENT|INFO: timer: %lu ns\n", time);
    }
}
