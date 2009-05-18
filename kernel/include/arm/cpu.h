/*********************************************************************
 *                
 * Copyright (C) 1999 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/cpu.h
 * Description:   ARM specific declarations.
 *                
 * @LICENSE@
 *                
 * $Id: cpu.h,v 1.19 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__CPU_H__
#define __ARM__CPU_H__

/* CP15 Control Register */

#define CTL_MMU_ENABLED         0x0001
#define CTL_ADDR_FAULT          0x0002
#define CTL_DCACHE              0x0004
#define CTL_WRITE_BUF           0x0008
#define CTL_BIG_ENDIAN          0x0080
#define CTL_SYS_PROT            0x0100
#define CTL_ROM_PROT            0x0200
#define CTL_ICACHE              0x1000
#define CTL_IVECTOR             0x2000

/* Control Bits */

#define IRQ_MASK	(1 << 7)	/* IRQ Disable Bit */
#define FIQ_MASK	(1 << 6)	/* FIQ Disable Bit */
#define THUMB_CODE	(1 << 5)	/* thumb code mode */
#define USER_MODE	(0x10)		/* USER Mode */
#define FIQ_MODE	(0x11)		/* FIQ Mode */
#define IRQ_MODE	(0x12)		/* IRQ Mode */
#define SVC_MODE	(0x13)		/* SVC Mode */
#define ABORT_MODE	(0x17)		/* ABORT Mode */
#define UNDEF_MODE	(0x1B)		/* UNDEF Mode */
#define SYSTEM_MODE	(0x1F)		/* SYSTEM Mode */

/* the mode all kernel activities are executed in */
#define KERNEL_MODE	(SVC_MODE)

#define MODE_MASK 0x0000000F 

/* kernel identification - we are on ARM */
#define KERNEL_VERSION_VER	KERNEL_VERSION_CPU_ARM

#ifndef ASSEMBLY

#include INC_PLATFORM(cpu.h)

/*********
* Macros *
*********/


/* CP15 Flush Instruction TLB & Data TLB */ 
INLINE void flush_tlb()
{
    __asm__ __volatile__
	("mcr     p15, 0, r0, c8, c7, 0");
}


INLINE void flush_tlbent(ptr_t addr)
{
    __asm__ __volatile__
	("mcr     p15, 0, r0, c8, c7, 0");
}


INLINE void enable_interrupts()
{
    register dword_t foo;
    __asm__ __volatile__ (
	"/* enable_interrupts() */	\n"
	"	mrs	%0, cpsr	\n"
	"	and	%0, %0, %1	\n"
	"	msr	cpsr, %0	\n"
	: "=r" (foo)
	: "i" (~(IRQ_MASK|FIQ_MASK)));
}

INLINE void disable_interrupts()
{
    register dword_t foo;
    __asm__ __volatile__ (
	"/* disable_interrupts() */	\n"
	"	mrs	%0, cpsr	\n"
	"	or	%0, %0, %1	\n"
	"	msr	cpsr, %0	\n"
	: "=r" (foo)
	: "i" (IRQ_MASK|FIQ_MASK));
}


INLINE void system_sleep()
{
    /* we have no sleep, haven't we? */
    register dword_t foo, foo2;
    __asm__ __volatile__ (
	"/* enable_interrupts(start) */	\n"
	"	mrs	%0, cpsr	\n"
	"	mov	%1, %2		\n"
	"	msr	cpsr, %1	\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	nop			\n"
	"	msr	cpsr, %0	\n"
	: "=r" (foo), "=r" (foo2)
	: "i" (KERNEL_MODE));
}


typedef struct {
    dword_t	r0, r1, r2 , r3 , r4 , r5, r6, r7;
    dword_t	r8, r9, r10, r11, r12, r13, r14;
    dword_t	sp, lr, cpsr, spsr;
    dword_t	excp_no;
} exception_frame_t;	

#endif /* ASSEMBLY */

#endif /* __ARM__CPU_H__ */
