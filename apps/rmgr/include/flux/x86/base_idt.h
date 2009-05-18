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
#ifndef _FLUX_X86_BASE_IDT_H_
#define _FLUX_X86_BASE_IDT_H_

#include <flux/c/common.h>
#include <flux/x86/seg.h>


/* The base environment provides a full-size 256-entry IDT.
   This is needed, for example, under VCPI or Intel MP Standard PCs.  */
#define IDTSZ	256

extern struct x86_gate base_idt[IDTSZ];


/* Note that there is no base_idt_init() function,
   because the IDT is used for both trap and interrupt vectors.
   To initialize the processor trap vectors, call base_trap_init().
   Inititalizing hardware interrupt vectors is platform-specific.  */


__FLUX_BEGIN_DECLS
/* Load the base IDT into the CPU.  */
extern void base_idt_load(void);
__FLUX_END_DECLS


#endif /* _FLUX_X86_BASE_IDT_H_ */
