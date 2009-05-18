/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-booter/crt0-arm-ep7211.S
 * Description:   startup assembly code for EP7211
 *                
 * @LICENSE@
 *                
 * $Id: crt0-arm-ep7211.S,v 1.3 2001/12/09 04:06:30 ud3 Exp $
 *                
 ********************************************************************/

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

	mov	r0, #0
	mcr	p15, 0, r0, c7, c7	/* flush I,D caches on v4	*/
	mcr	p15, 0, r0, c7, c10, 4	/* drain write buffer on v4	*/
	mcr	p15, 0, r0, c8, c7	/* flush I,D TLBs on v4		*/

	ldr	r2, =1f

2:	bic	r3, r3, #0x1		/* MMU				*/
	mcr	p15, 0, r3, c1, c0	/* disabled			*/

	mov	pc, r2			/* jump to the next instruction	*/
1:
	
#define SLEEP \
	mov r1, #0x10000 ; 0: subs r1, r1, #1; bne 0b


#define HwBaseAddress2		0x80001000
#define HwControl		0x00000100
#define HwControlUartEnable	0x00000100
#define HwStatus		0x00000140
#define CLKMOD			0x00000040
#define UartValue		(0x00000001 | 0x00070000)
#define UartValue_13		(0x00000000 | 0x00070000)
#define HwUartControl		0x000004C0
#define HwUartData		0x00000480

	/* LED Flasher		*/
	ldr	r0, =0x800022c0
	mov	r1, #((1 << 6) | (0x7 << 2) | 0)
//	str	r1, [r0]
			
	ldr	r12, =HwBaseAddress2
	mov	r0, #HwControlUartEnable	/* UART enable		*/
	str	r0, [r12, #HwControl]		/* write HwControl	*/

	ldr	r1, =HwStatus
	add	r1, r1, r12
	ldr	r2, [r1]
	tst	r2, #CLKMOD			/* figure out which	*/
	ldreq	r0, =UartValue			/* rate setting to use	*/
	ldrne	r0, =UartValue_13
	str	r0, [r12, #HwUartControl]

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


