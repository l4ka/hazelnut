/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
#ifndef _FLUX_X86_ASM_H_
#define _FLUX_X86_ASM_H_

#include <flux/config.h>

/* This is a more reliable delay than a few short jmps. */
#define IODELAY	pushl %eax; inb $0x80,%al; inb $0x80,%al; popl %eax

#define S_ARG0	 4(%esp)
#define S_ARG1	 8(%esp)
#define S_ARG2	12(%esp)
#define S_ARG3	16(%esp)

#define FRAME	pushl %ebp; movl %esp, %ebp
#define EMARF	leave

#define B_ARG0	 8(%ebp)
#define B_ARG1	12(%ebp)
#define B_ARG2	16(%ebp)
#define B_ARG3	20(%ebp)

#ifdef i486
#define TEXT_ALIGN	4
#else
#define TEXT_ALIGN	2
#endif
#define DATA_ALIGN	2
#define ALIGN		TEXT_ALIGN

/*
 * The .align directive has different meaning on the x86 when using ELF
 * than when using a.out.
 * Newer GNU as remedies this problem by providing .p2align and .balign.
 */
#if defined(HAVE_P2ALIGN)
# define P2ALIGN(p2)	.p2align p2
#elif defined(__ELF__)
# define P2ALIGN(p2)	.align	(1<<(p2))
#else
# define P2ALIGN(p2)	.align	p2
#endif

#define	LCL(x)	x

#define LB(x,n) n
#ifndef __ELF__
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#define SEXT(x) "_"#x
#else
#define EXT(x) x
#define LEXT(x) x ## :
#define SEXT(x) #x
#endif
#define GLEXT(x) .globl EXT(x); LEXT(x)
#define LCLL(x) x ## :
#define gLB(n)  n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f


/* Symbol types */
#ifdef __ELF__
#define FUNCSYM(x)	.type    x,@function
#else
#define FUNCSYM(x)	/* nothing */
#endif


#ifdef GPROF

#define MCOUNT		.data; gLB(9) .long 0; .text; lea LBb(x, 9),%edx; call mcount
#define	ENTRY(x)	.globl EXT(x); P2ALIGN(TEXT_ALIGN); LEXT(x) ; \
			pushl %ebp; movl %esp, %ebp; MCOUNT; popl %ebp;
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			P2ALIGN(TEXT_ALIGN); LEXT(x) LEXT(y)
#define	ASENTRY(x) 	.globl x; P2ALIGN(TEXT_ALIGN); gLB(x) ; \
  			pushl %ebp; movl %esp, %ebp; MCOUNT; popl %ebp;

#else	/* ndef GPROF */

#define MCOUNT
#define	ENTRY(x)	FUNCSYM(x); .globl EXT(x); P2ALIGN(TEXT_ALIGN); LEXT(x)
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			P2ALIGN(TEXT_ALIGN); LEXT(x) LEXT(y)
#define	ASENTRY(x)	.globl x; P2ALIGN(TEXT_ALIGN); gLB(x)

#endif	/* ndef GPROF */


#define	Entry(x)	P2ALIGN(TEXT_ALIGN); GLEXT(x)
#define	DATA(x)		P2ALIGN(DATA_ALIGN); GLEXT(x)

#endif /* _FLUX_X86_ASM_H_ */
