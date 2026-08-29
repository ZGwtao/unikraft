/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdbool.h>

#include <uk/assert.h>
#include <uk/compiler.h>
#include <uk/console/driver.h>
#include <uk/errptr.h>
#include <uk/init.h>
#include <uk/driver/sddf/serial.h>

#if CONFIG_LIBSDDFSERIAL_EARLY_CONSOLE
#include <uk/boot/earlytab.h>
#include <uk/plat/common/bootinfo.h>
#endif /* CONFIG_LIBSDDFSERIAL_EARLY_CONSOLE */


static struct uk_console sddfserial_console;

static int sddfserial_setup(void)
{
	return uk_sddf_serial_init();
}

__isr static __ssz sddfserial_out(struct uk_console *dev __unused,
				  const char *buf, __sz len)
{
	return uk_sddf_serial_write(buf, len);
}

__isr static __ssz sddfserial_emerg_out(struct uk_console *dev __unused,
					const char *buf, __sz len)
{
	return uk_sddf_serial_emerg_write(buf, len);
}

static __ssz sddfserial_in(struct uk_console *dev __unused,
			   char *buf, __sz len)
{
	/* Feed the tty one character at a time.  ukfile-console performs line
	 * discipline after each console read; draining a whole RX batch here
	 * would hide embedded CR/LF boundaries when multiple lines are pasted. */
	return uk_sddf_serial_read_blocking(buf, len ? 1 : 0);
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
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_STDIN |
			UK_CONSOLE_FLAG_EMERG_STDOUT,
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
