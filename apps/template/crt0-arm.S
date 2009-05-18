/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     template/crt0-arm.S
 * Description:   example server startup code for StrongARM processors
 *                
 * @LICENSE@
 *                
 * $Id: crt0-arm.S,v 1.3 2001/12/13 03:25:43 ud3 Exp $
 *                
 ********************************************************************/
	.globl	_start
_start:
	ldr	sp, 2f
	bl	main
1:	b	1b
2:	.word	_stack_top

	.globl	__gccmain
__gccmain:
	mov	pc,lr

	.bss
	.p2align 12
_stack_bottom:
	.space	8192
_stack_top:
