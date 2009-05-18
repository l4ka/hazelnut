/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/cpu.h
 * Description:   MIPS CPU spcific definitions.
 *                
 * @LICENSE@
 *                
 * $Id: cpu.h,v 1.3 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_CPU_H__
#define __MIPS_CPU_H__

#define zero    $0      /* wired zero */
#define AT      $1      /* assembler temp  - uppercase because of ".set at" */
#define v0      $2      /* return value */
#define v1      $3
#define a0      $4      /* argument registers */
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8      /* caller saved */
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16     /* callee saved */
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24     /* caller saved */
#define t9      $25
#define jp      $25     
#define k0      $26     /* kernel scratch */
#define k1      $27
#define gp      $28     /* global pointer */
#define sp      $29     /* stack pointer */
#define fp      $30     /* frame pointer */
#define s8	$30	/* same as fp */
#define ra      $31     /* return address */

/*
 * Coprocessor 0 
 */
#define CP0_INDEX	$0
#define CP0_RANDOM	$1
#define CP0_ENTRYLO0	$2
#define CP0_ENTRYLO1	$3
#define CP0_CONTEXT	$4
#define CP0_PAGEMASK	$5
#define CP0_WIRED	$6
#define CP0_BADVADDR	$8
#define CP0_COUNT	$9
#define CP0_ENTRYHI	$10
#define CP0_COMPARE	$11
#define CP0_STATUS	$12
#define CP0_CAUSE	$13
#define CP0_EPC		$14
#define CP0_PRID	$15
#define CP0_CONFIG	$16
#define CP0_LLADDR	$17
#define CP0_WATCHLO	$18
#define CP0_WATCHHI	$19
#define CP0_XCONTEXT	$20
#define CP0_FRAMEMASK	$21
#define CP0_DIAGNOSTIC	$22
#define CP0_PERFORMANCE	$25
#define CP0_ECC		$26
#define CP0_CACHEERR	$27
#define CP0_TAGLO	$28
#define CP0_TAGHI	$29
#define CP0_ERROREPC	$30

#define ST0_KUP		0x18

#ifndef ASSEMBLY


typedef struct {
    dword_t dummy;
} exception_frame_t;    


INLINE void disable_interrupts()
{
}

INLINE void enable_interrupts()
{
}

INLINE void system_sleep()
{
}

INLINE void flush_tlb()
{
}

INLINE void flush_tlbent(ptr_t addr)
{
}


#endif 

#endif
