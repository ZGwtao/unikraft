/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef UK_USE_SECTION_SEGMENTS
#define UK_USE_SECTION_SEGMENTS 1
#endif

#include <uk/arch/limits.h>
#include <uk/plat/common/common.lds.h>

#if CONFIG_ARCH_X86_64
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
#else
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
#endif
ENTRY(_start)

PHDRS
{
#if CONFIG_ELF_PHDRS_IN_PT_LOAD
	phdrs PT_LOAD FILEHDR PHDRS FLAGS(PHDRS_PF_R);
#endif
	text     PT_LOAD FLAGS(PHDRS_PF_RX);
	rodata   PT_LOAD FLAGS(PHDRS_PF_R);
	data     PT_LOAD FLAGS(PHDRS_PF_RW);
	tls      PT_TLS;
	tls_load PT_LOAD FLAGS(PHDRS_PF_R);
	stack    PT_GNU_STACK FLAGS(PHDRS_PF_RW);
}

SECTIONS
{
	. = 0x2800000;

	_base_addr = .;
	_start_ram_addr = .;

#if CONFIG_ELF_PHDRS_IN_PT_LOAD
	PROVIDE(__executable_start = _base_addr);
	. = _base_addr + SIZEOF_HEADERS;
	PROVIDE(__phdr = .);
#endif

	_text = .;
	.text :
	{
		KEEP(*(.text.start))
		KEEP(*(.text.start.*))
		KEEP(*(.text._start))
		KEEP(*(.text._start.*))
		*(.text)
		*(.text.*)
	} :text

	. = ALIGN(__PAGE_SIZE);
	_etext = .;

	_rodata = .;

	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
	} :rodata

	EXCEPTION_SECTIONS

	CTORTAB_SECTION

	INITTAB_SECTION

	EVENTTAB_SECTION

	_erodata = .;

	. = ALIGN(8);
	_ctors = .;

	.preinit_array :
	{
		PROVIDE_HIDDEN(__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN(__preinit_array_end = .);
	} :rodata

	. = ALIGN(8);

	.init_array :
	{
		PROVIDE_HIDDEN(__init_array_start = .);
		KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)
		       SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP(*(.init_array .ctors))
		PROVIDE_HIDDEN(__init_array_end = .);
	} :rodata

	_ectors = .;

	. = ALIGN(__PAGE_SIZE);

	TLS_SECTIONS

	DATA_SECTIONS

	. = ALIGN(__PAGE_SIZE);
	.ipc_buffer (NOLOAD) :
	{
		PROVIDE(__sel4_ipc_buffer_obj = .);
	} :NONE

	_end = .;

	.comment 0 :
	{
		*(.comment)
	}

	DEBUG_SYMBOLS

	DISCARDS
}
