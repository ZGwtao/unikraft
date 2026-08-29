#include <uk/sddf.h>
#include <uk/errptr.h>
#include <errno.h>
#include <uk/sched.h>
#include <uk/print.h>
#include <uk/driver/sddf/serial.h>

__attribute__((__section__(".serial_client_config")))
serial_client_config_t serial_config;
serial_queue_handle_t serial_rx_queue_handle;
serial_queue_handle_t serial_tx_queue_handle;
static int serial_initialized;

int uk_sddf_serial_init(void)
{
	if (!serial_config_check_magic(&serial_config))
		return -EINVAL;
	if (serial_initialized)
		return 0;
	if (serial_config.rx.queue.vaddr != NULL)
		serial_queue_init(&serial_rx_queue_handle, serial_config.rx.queue.vaddr,
				  serial_config.rx.data.size, serial_config.rx.data.vaddr);
	serial_queue_init(&serial_tx_queue_handle, serial_config.tx.queue.vaddr,
				  serial_config.tx.data.size, serial_config.tx.data.vaddr);
	serial_putchar_init(serial_config.tx.id, &serial_tx_queue_handle);
	uk_pr_info("sDDF serial: rx=%p tx=%p rx-cap=%u\n",
		   serial_config.rx.queue.vaddr, serial_config.tx.queue.vaddr,
		   serial_rx_queue_handle.capacity);
	serial_initialized = 1;
	return 0;
}

__ssz uk_sddf_serial_write(const char *buf, __sz len)
{
	__sz n = len;
	if (!buf) return -EINVAL;
	while (n--) _sddf_putchar(*buf++);
	return (__ssz)len;
}

__ssz uk_sddf_serial_emerg_write(const char *buf, __sz len)
{
	__sz n = len;
	if (!buf) return -EINVAL;
	while (n--) sddf_putchar_unbuffered(*buf++);
	return (__ssz)len;
}

__ssz uk_sddf_serial_read(char *buf __unused, __sz len __unused)
{
	__sz n;
	char c;

	if (!buf)
		return -EINVAL;
	if (!len)
		return 0;
	if (!serial_config.rx.queue.vaddr)
		return -EAGAIN;

	for (n = 0; n < len; ++n) {
		if (serial_dequeue(&serial_rx_queue_handle, &c) < 0)
			break;
		buf[n] = c;
	}
	return n ? (__ssz)n : -EAGAIN;
}

__ssz uk_sddf_serial_read_blocking(char *buf, __sz len)
{
	__ssz rc;
	static int announced;

	if (!buf)
		return -EINVAL;
	if (!len)
		return 0;

	for (;;) {
		rc = uk_sddf_serial_read(buf, len);
		if (rc != -EAGAIN)
			return rc;
		if (!announced) {
			uk_pr_info("sDDF serial: waiting for RX input\n");
			announced = 1;
		}
		uk_sched_yield();
	}
}
