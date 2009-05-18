/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-booter/crt0-arm-dnard.S
 * Description:   startup assembly code for DNARD board
 *                
 * @LICENSE@
 *                
 * $Id: crt0-arm-dnard.S,v 1.5 2001/12/11 19:54:22 ud3 Exp $
 *                
 ********************************************************************/

/*
  DNARD initial memory map

  virt		phys		size
  F0000000	0e500000
  F7EFF000	40000000	4k


  We're loaded at f0000000, backed by 0e500000. This code is linked at
  0e500000. Thus the ldr r2,=1f will load 0e5000xy, not f00000xy. By
  jumping to r2 directly after turning off the MMU, we then run 1:1

*/	
	.globl	_start
_start:
	/* Turn off interrupts to keep control */
	mrs     r0, cpsr
	orr     r0, r0, #0xC0		/* IRQs and FIQs disabled	*/
	msr     cpsr, r0

	/* Clean out the DCache */
	mrc	p15, 0, r3, c1, c0
	bic	r3, r3, #0xC		/* Write Buffer and DCache	*/
	bic	r3, r3, #0x1000		/* ICache			*/
	mcr	p15, 0, r3, c1, c0	/* disabled			*/
	bic	r3, r3, #0x1		/* prepare disabling of MMU	*/

	ldr	r2, =1f			/* load target address to jump
					   to when MMU is off		*/
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7	/* flush I,D caches on v4	*/
	mcr	p15, 0, r0, c7, c10, 4	/* drain write buffer on v4	*/
	mcr	p15, 0, r0, c8, c7	/* flush I,D TLBs on v4
					   The next 4 instructions are
					   fetched still using the just
					   flushed TLB entries		*/

	mcr	p15, 0, r3, c1, c0	/* disable MMU			*/
	mov	pc, r2			/* jump to the next instruction	*/
	 nop
	 nop
	 nop
1:
	
#define SLEEP \
	mov r1, #0x10000 ; 0: subs r1, r1, #1; bne 0b

	ldr	r0, =0x400003f8
	mov	r1, #0
	strb	r1, [r0, #0x01]		/* disable serial interrupt	*/
	SLEEP
	mov	r1, #0x80
	strb	r1, [r0, #0x03]		/* switch to divisor mode	*/
	SLEEP
	mov	r1, #1
	strb	r1, [r0, #0x00]		/* DLLO	*/
	SLEEP
	mov	r1, #0
	strb	r1, [r0, #0x01]		/* DLHI	*/
	SLEEP
	mov	r1, #0x03
	strb	r1, [r0, #0x03]		/* switch back	*/
	SLEEP
	mov	r1, #0x06
	strb	r1, [r0, #0x02]		/* FCR	*/
	SLEEP
	mov	r1, #0x00
	strb	r1, [r0, #0x01]		/* FCR	*/

	SLEEP
	
	ldr	r1, [r0, #1]
	ldr	r1, [r0, #2]
	ldr	r1, [r0, #3]
	ldr	r1, [r0, #4]
	ldr	r1, [r0, #5]
	ldr	r1, [r0, #6]

	SLEEP
		
#if 0
	mov	r2, #0x39
1:	ldr	r0, =0x400003f8
	strb	r2, [r0]
	SLEEP
	sub	r2, r2, #1
	cmp	r2, #0x2F
	bne	1b
#endif
	
	/* here we actually start */
2:	ldr	sp, =__stack_top
	bl	main
1:	b	1b

	.globl	__gccmain
__gccmain:
	mov	pc,lr

		
	.bss
	.align 4
__stack_bottom:
	.space	1024
__stack_top:





