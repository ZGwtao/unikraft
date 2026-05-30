/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_PLAT_PAL_REGS_H__
#define __UK_PLAT_PAL_REGS_H__

#include <uk/arch/types.h>
#include <uk/essentials.h>

/*
 * AArch64 register-frame layout expected by libukpal.
 */
#define UK_PAL_ARM64_REGS_OFFSETOF_X0		0
#define UK_PAL_ARM64_REGS_OFFSETOF_X1		8
#define UK_PAL_ARM64_REGS_OFFSETOF_X2		16
#define UK_PAL_ARM64_REGS_OFFSETOF_X3		24
#define UK_PAL_ARM64_REGS_OFFSETOF_X4		32
#define UK_PAL_ARM64_REGS_OFFSETOF_X5		40
#define UK_PAL_ARM64_REGS_OFFSETOF_X6		48
#define UK_PAL_ARM64_REGS_OFFSETOF_X7		56
#define UK_PAL_ARM64_REGS_OFFSETOF_X8		64
#define UK_PAL_ARM64_REGS_OFFSETOF_X9		72
#define UK_PAL_ARM64_REGS_OFFSETOF_X10		80
#define UK_PAL_ARM64_REGS_OFFSETOF_X11		88
#define UK_PAL_ARM64_REGS_OFFSETOF_X12		96
#define UK_PAL_ARM64_REGS_OFFSETOF_X13		104
#define UK_PAL_ARM64_REGS_OFFSETOF_X14		112
#define UK_PAL_ARM64_REGS_OFFSETOF_X15		120
#define UK_PAL_ARM64_REGS_OFFSETOF_X16		128
#define UK_PAL_ARM64_REGS_OFFSETOF_X17		136
#define UK_PAL_ARM64_REGS_OFFSETOF_X18		144
#define UK_PAL_ARM64_REGS_OFFSETOF_X19		152
#define UK_PAL_ARM64_REGS_OFFSETOF_X20		160
#define UK_PAL_ARM64_REGS_OFFSETOF_X21		168
#define UK_PAL_ARM64_REGS_OFFSETOF_X22		176
#define UK_PAL_ARM64_REGS_OFFSETOF_X23		184
#define UK_PAL_ARM64_REGS_OFFSETOF_X24		192
#define UK_PAL_ARM64_REGS_OFFSETOF_X25		200
#define UK_PAL_ARM64_REGS_OFFSETOF_X26		208
#define UK_PAL_ARM64_REGS_OFFSETOF_X27		216
#define UK_PAL_ARM64_REGS_OFFSETOF_X28		224
#define UK_PAL_ARM64_REGS_OFFSETOF_X29		232
#define UK_PAL_ARM64_REGS_OFFSETOF_LR		240
#define UK_PAL_ARM64_REGS_OFFSETOF_ELR_EL1	248
#define UK_PAL_ARM64_REGS_OFFSETOF_SPSR_EL1	256
#define UK_PAL_ARM64_REGS_OFFSETOF_ESR_EL1	264
#define UK_PAL_ARM64_REGS_OFFSETOF_SP		272

#define UK_PAL_REGS_PAD_SIZE			8
#define UK_PAL_REGS_SIZE			288

#define UK_PAL_REGS_OFFSETOF_SP \
	UK_PAL_ARM64_REGS_OFFSETOF_SP

#define UK_PAL_REGS_OFFSETOF_PC \
	UK_PAL_ARM64_REGS_OFFSETOF_ELR_EL1

#if !__ASSEMBLY__

#ifdef __cplusplus
extern "C" {
#endif

struct uk_pal_regs {
	__u64 x0;
	__u64 x1;
	__u64 x2;
	__u64 x3;
	__u64 x4;
	__u64 x5;
	__u64 x6;
	__u64 x7;
	__u64 x8;
	__u64 x9;
	__u64 x10;
	__u64 x11;
	__u64 x12;
	__u64 x13;
	__u64 x14;
	__u64 x15;
	__u64 x16;
	__u64 x17;
	__u64 x18;
	__u64 x19;
	__u64 x20;
	__u64 x21;
	__u64 x22;
	__u64 x23;
	__u64 x24;
	__u64 x25;
	__u64 x26;
	__u64 x27;
	__u64 x28;
	__u64 x29;

	__u64 lr;
	__u64 elr_el1;
	__u64 spsr_el1;
	__u64 esr_el1;
	__u64 sp;

	__u8 pad[UK_PAL_REGS_PAD_SIZE];
};

UK_CTASSERT(sizeof(struct uk_pal_regs) == UK_PAL_REGS_SIZE);

UK_CTASSERT(__offsetof(struct uk_pal_regs, x0) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_X0);
UK_CTASSERT(__offsetof(struct uk_pal_regs, x29) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_X29);
UK_CTASSERT(__offsetof(struct uk_pal_regs, lr) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_LR);
UK_CTASSERT(__offsetof(struct uk_pal_regs, elr_el1) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_ELR_EL1);
UK_CTASSERT(__offsetof(struct uk_pal_regs, spsr_el1) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_SPSR_EL1);
UK_CTASSERT(__offsetof(struct uk_pal_regs, esr_el1) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_ESR_EL1);
UK_CTASSERT(__offsetof(struct uk_pal_regs, sp) ==
	    UK_PAL_ARM64_REGS_OFFSETOF_SP);

__isr static inline __u64
uk_pal_regs_get(const struct uk_pal_regs *regs, __sz offset)
{
	return *(__u64 *)((const char *)regs + offset);
}

__isr static inline void
uk_pal_regs_set(struct uk_pal_regs *regs, __sz offset, __u64 val)
{
	*(__u64 *)((char *)regs + offset) = val;
}

#ifdef __cplusplus
}
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __UK_PLAT_PAL_REGS_H__ */