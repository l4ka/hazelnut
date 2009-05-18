/* 
 * Copyright (c) 1996-1995 The University of Utah and
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
 * This file defines a simple, clean method provided by the OS toolkit
 * for initializing x86 gate descriptors:
 * call gates, interrupt gates, trap gates, and task gates.
 * Simply create a table of gate_init_entry structures
 * (or use the corresponding assembly-language macros below),
 * and then call the x86_gate_init() routine.
 */
#ifndef _FLUX_X86_KERN_GATE_INIT_H_
#define _FLUX_X86_KERN_GATE_INIT_H_

#ifndef ASSEMBLER

#include <flux/c/common.h>

/* One entry in the list of gates to initialized.
   Terminate with an entry with a null entrypoint.  */
struct gate_init_entry
{
	unsigned entrypoint;
	unsigned short vector;
	unsigned short type;
};

struct x86_gate;

__FLUX_BEGIN_DECLS
/* Initialize a set of gates in a descriptor table.
   All gates will use the same code segment selector, 'entry_cs'.  */
void gate_init(struct x86_gate *dest, const struct gate_init_entry *src,
	       unsigned entry_cs);
__FLUX_END_DECLS

#else /* ASSEMBLER */

/*
 * We'll be using macros to fill in a table in data hunk 2
 * while writing trap entrypoint routines at the same time.
 * Here's the header that comes before everything else.
 */
#define GATE_INITTAB_BEGIN(name)	\
	.data	2			;\
ENTRY(name)				;\
	.text

/*
 * Interrupt descriptor table and code vectors for it.
 */
#define	GATE_ENTRY(n,entry,type) \
	.data	2			;\
	.long	entry			;\
	.word	n			;\
	.word	type			;\
	.text

/*
 * Terminator for the end of the table.
 */
#define GATE_INITTAB_END		\
	.data	2			;\
	.long	0			;\
	.text


#endif /* ASSEMBLER */

#endif /* _FLUX_X86_KERN_GATE_INIT_H_ */
