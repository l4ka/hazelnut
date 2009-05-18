/* 
 * Copyright (c) 1995-1994 The University of Utah and
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
 * Definitions for a basic x86 32-bit Task State Segment (TSS)
 * which is set up and and used by the base environment as a "default" TSS.
 */
#ifndef _FLUX_X86_BASE_TSS_H_
#define _FLUX_X86_BASE_TSS_H_

#include <flux/c/common.h>
#include <flux/x86/tss.h>

extern struct x86_tss base_tss;

__FLUX_BEGIN_DECLS
/* Initialize the base TSS with sensible default values.
   The current stack pointer is used as the TSS's ring 0 stack pointer.  */
extern void base_tss_init(void);

/* Load the base TSS into the current CPU.
   Assumes that the base GDT is already loaded,
   and that the BASE_TSS descriptor in the GDT refers to the base TSS.  */
extern void base_tss_load(void);
__FLUX_END_DECLS

#endif _FLUX_X86_BASE_TSS_H_
