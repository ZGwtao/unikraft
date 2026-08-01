
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

#include <uk/microkit.h>

__attribute__((__section__(".serial_client_config")))
serial_client_config_t serial_config;
__attribute__((__section__(".timer_client_config")))
timer_client_config_t timer_config;

serial_queue_handle_t serial_rx_queue_handle;
serial_queue_handle_t serial_tx_queue_handle;

sddf_channel uk_carrels_timer_channel;
bool uk_carrels_timer_ready
;

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

	uk_carrels_timer_channel = timer_config.driver_id;
	uk_carrels_timer_ready = true;

	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	assert(bi != NULL);

    carrels_add_heap_mrd(bi);

	uk_boot_early_init(bi);

	uk_boot_entry();
}

void notified(microkit_channel ch)
{
	(void)ch;
}