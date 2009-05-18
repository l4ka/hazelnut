/* 
 * Copyright (c) 1996-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
/*
 * This file defines a basic global descriptor table that can be used
 * for simple protected-mode execution in both kernel and user mode.
 */
#ifndef _FLUX_X86_BASE_GDT_H_
#define _FLUX_X86_BASE_GDT_H_

#include <flux/x86/seg.h>


#define BASE_TSS	0x08
#define KERNEL_CS	0x10	/* Kernel's PL0 code segment */
#define KERNEL_DS	0x18	/* Kernel's PL0 data segment */
#define KERNEL_16_CS	0x20	/* 16-bit version of KERNEL_CS */
#define KERNEL_16_DS	0x28	/* 16-bit version of KERNEL_DS */
#define LINEAR_CS	0x30	/* PL0 code segment mapping to linear space */
#define LINEAR_DS	0x38	/* PL0 data segment mapping to linear space */
#define USER_CS		0x43	/* User-defined descriptor, RPL=3 */
#define USER_DS		0x4b	/* User-defined descriptor, RPL=3 */

#define GDTSZ		(0x50/8)


#ifndef ASSEMBLER

#include <flux/c/common.h>

extern struct x86_desc base_gdt[GDTSZ];

__FLUX_BEGIN_DECLS
/* Initialize the base GDT descriptors with sensible defaults.  */
extern void base_gdt_init(void);

/* Load the base GDT into the CPU.  */
extern void base_gdt_load(void);

/* 16-bit versions of the above functions,
   which can be executed in real mode.  */
extern void i16_base_gdt_init(void);
extern void i16_base_gdt_load(void);
__FLUX_END_DECLS

#endif /* not ASSEMBLER */

#endif _FLUX_X86_BASE_GDT_H_
