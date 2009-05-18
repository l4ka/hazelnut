/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     crt0-arm.S
 * Description:   Startup code for ARM family processors.
 *                
 * @LICENSE@
 *                
 * $Id: crt0-arm.S,v 1.1 2001/12/10 17:04:37 skoglund Exp $
 *                
 ********************************************************************/
#include <config.h>

	.globl	_start
_start:
	ldr	sp, 2f
	bl	main
1:	b	1b
2:	.word	_stack_top

	.globl	atexit
atexit:	
	mov	pc, lr

	.bss
_stack_bottom:
	.space	8192
_stack_top:
