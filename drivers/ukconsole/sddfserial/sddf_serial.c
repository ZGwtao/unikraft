/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdbool.h>

#include <uk/assert.h>
#include <uk/compiler.h>
#include <uk/console/driver.h>
#include <uk/errptr.h>
#include <uk/init.h>
#include <uk/sddf.h>

#if CONFIG_LIBSDDFSERIAL_EARLY_CONSOLE
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#endif /* CONFIG_LIBSDDFSERIAL_EARLY_CONSOLE */


extern serial_client_config_t serial_config;

static struct uk_console sddfserial_console;

/*
 * Provided by the sDDF serial putchar library.
 *
 * _sddf_putchar() is buffered: it updates the shared tail and notifies the
 * serial virtualiser on '\n' or when the queue is full.
 *
 * sddf_putchar_unbuffered() updates the shared tail and notifies immediately.
 */
void _sddf_putchar(char character);
void sddf_putchar_unbuffered(char character);

static int sddfserial_setup(void)
{
	if (unlikely(!serial_config_check_magic(&serial_config)))
		return -EINVAL;

	/* the initialisation work is finished at entry.c ... */
	return 0;
}

__isr static __ssz sddfserial_out(struct uk_console *dev __unused,
				  const char *buf, __sz len)
{
	__sz l = len;

	UK_ASSERT(buf);

	while (l--)
		_sddf_putchar(*buf++);

	return len;
}

__isr static __ssz sddfserial_emerg_out(struct uk_console *dev __unused,
					const char *buf, __sz len)
{
	__sz l = len;

	UK_ASSERT(buf);

	while (l--)
		sddf_putchar_unbuffered(*buf++);

	return len;
}

static __ssz sddfserial_in(struct uk_console *dev __unused,
			   char *buf __unused, __sz len __unused)
{
	/*
	 * Input is intentionally not implemented.  The old backend polled PL011
	 * MMIO registers directly; this backend only exposes the sDDF serial TX
	 * path as a Unikraft console.
	 */
	return 0;
}

static const struct uk_console_ops sddfserial_ops = {
	.out = sddfserial_out,
	.in = sddfserial_in,
	.emerg_out = sddfserial_emerg_out,
};

static int sddfserial_register_console(void)
{
	int rc;

	rc = sddfserial_setup();
	if (unlikely(rc))
		return rc;

	uk_console_init(&sddfserial_console, "SDDFSERIAL", &sddfserial_ops,
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_EMERG_STDOUT,
			UK_CONSOLE_CLASS_UART);
	uk_console_register(&sddfserial_console);

	return 0;
}

#if CONFIG_LIBSDDFSERIAL_EARLY_CONSOLE
static int early_init(struct ukplat_bootinfo *bi __unused)
{
	return sddfserial_register_console();
}

UK_BOOT_EARLYTAB_ENTRY(early_init, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#else
static int sddfserial_init(struct uk_init_ctx *ictx __unused)
{
	return sddfserial_register_console();
}

/* UK_PRIO_EARLIEST reserved for cmdline */
uk_plat_initcall_prio(sddfserial_init, 0, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* CONFIG_LIBSDDFSERIAL_EARLY_CONSOLE */
