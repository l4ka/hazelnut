/*********************************************************************
 *                
 * Copyright (C) 1999-2002,  Karlsruhe University
 *                
 * File path:     sigma0/crt0-arm.S
 * Description:   Startup code for ARM family processors.
 *                
 * @LICENSE@
 *                
 * $Id: crt0-arm.S,v 1.7 2002/05/06 13:52:51 ud3 Exp $
 *                
 ********************************************************************/
#include <config.h>

	.globl	_start
_start:
	ldr	sp, 2f
	bl	sigma0_main

1:	b	1b
2:	.word	_sigma0_stack_top

	.bss
	.p2align 12
_sigma0_stack_bottom:
	.space	8192
_sigma0_stack_top:
