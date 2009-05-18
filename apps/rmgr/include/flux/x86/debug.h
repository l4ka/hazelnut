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
#ifndef _FLUX_X86_DEBUG_H_
#define _FLUX_X86_DEBUG_H_

#define PRINT_STACK_TRACE(num_frames) {			\
	int i = (num_frames);				\
	unsigned *fp;					\
	printf("stack trace:\n");			\
	asm volatile ("movl %%ebp,%0" : "=r" (fp));	\
	while (fp && --i >= 0) {			\
		printf("\t0x%08x\n", *(fp + 1));	\
		fp = (unsigned *)(*fp);			\
	}						\
}

#ifdef ASSEMBLER
#ifdef DEBUG


#define A(cond,a,b)			\
	a,b				;\
	j##cond	8f			;\
	int	$0xda			;\
8:


#else /* !DEBUG */


#define A(cond,a,b)


#endif /* !DEBUG */
#else /* !ASSEMBLER */


#endif /* !ASSEMBLER */

#endif /* _FLUX_X86_DEBUG_H_ */
