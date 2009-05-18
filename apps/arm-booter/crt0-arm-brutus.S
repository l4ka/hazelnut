/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-booter/crt0-arm-brutus.S
 * Description:   startup assembly code for Brutus (AngelBoot)
 *                
 * @LICENSE@
 *                
 * $Id: crt0-arm-brutus.S,v 1.3 2001/12/09 04:06:30 ud3 Exp $
 *                
 ********************************************************************/

	.globl	_start
_start:
	/* Request SVC mode from Angel */
        mov     r0, #0x17
        swi     #0x123456

	/* Turn off interrupts to keep control */
        mrs     r0, cpsr
        orr     r0, r0, #0xC0
        msr     cpsr, r0

	/* Clean out the DCache */
        mov     r2, pc
        bic     r2, r2, #0x1f
        add     r3, r2, #0x10000        @ 64 kb is quite enough...
1:	ldr     r0, [r2], #32
        teq     r2, r3
        bne     1b

	mcr p15, 0, r0, c7, c10, 4	@ Drain writebuf

	/* Disable MMU and Caches */
        mrc     p15, 0, r0, c1, c0
        ldr     r1, =0x100d
        bic     r0, r0, r1
        mcr     p15, 0, r0, c1, c0

	/* Clean out caches and TLBs */
	mcr p15, 0, r0, c7, c7, 0	@ Flush I+D cache
	mcr p15, 0, r0, c8, c7, 0	@ Flush I+D TLBs on v4

	ldr     r1, =0x90040000		@ Load GPIO PBase
        ldr     r0, =0x00100000		@ Set relevant output GPIOs
        str     r0, [r1, #0xc]

#define SLEEP			; \
	mov	r1, #0x800000	; \
0:	subs	r1, r1, #1	; \
	bne	0b

	ldr     r1, =0x80050000

	/* Disable UART */
        mov     r0, #0x00		@ disable ints/RX/TX
        str     r0, [r1, #0x0c]		@ Set Cntr Reg 3

	/* Clear Serial Interrupts */
        ldr     r0, =0xFF
        str     r0, [r1, #0x1c]		@ Clear Stat Reg 0

	/* Set to (N81) No Parity, 8bit data, 1 stop bit, internal clock */
        ldr     r0, =0x8
        str     r0, [r1, #0x00]		@ Set Cntr Reg 0

	/* Set to Baud Rate of 115200 */
        mov     r0, #0x00
        str     r0, [r1, #0x04]		@ Set Cntr Reg 1
        ldr     r0, =0x001
        str     r0, [r1, #0x08]		@ Set Cntr Reg 2

	/* Enable Transmit and Receive, Interrupts Masked */
        ldr     r0, =0x3
        str     r0, [r1, #0x0c]		@ Set Cntr Reg 3

	SLEEP

	/* here we actually start */
2:	ldr	sp, =__stack_top
	bl	main
1:	b	1b

	.globl	__gccmain
__gccmain:
	mov	pc,lr

	.bss
	.align 2			
__stack_bottom:
	.space	1024
__stack_top:





