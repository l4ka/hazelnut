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
#ifndef _FLUX_X86_PC_BASE_IRQ_H_
#define _FLUX_X86_PC_BASE_IRQ_H_


/* On normal PCs, there are always 16 IRQ lines.  */
#define IRQ_COUNT 16


/* Variables storing the master and slave PIC interrupt vector base.  */
extern int irq_master_base, irq_slave_base;


/* Fill an IRQ gate in the base IDT.
   Always uses an interrupt gate; just set `access' to the privilege level.  */
#define fill_irq_gate(irq_num, entry, selector, access)			\
	fill_gate(&base_idt[(irq_num) < 8				\
			    ? irq_master_base+(irq_num)			\
			    : irq_slave_base+(irq_num)-8],		\
		  entry, selector, ACC_INTR_GATE | (access), 0)

#endif /* _FLUX_X86_PC_BASE_IRQ_H_ */
