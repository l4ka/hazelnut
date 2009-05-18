/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/pr31700/startup.S
 * Description:   Startup code for the MIPS PR31700.
 *                
 * @LICENSE@
 *                
 * $Id: startup.S,v 1.2 2001/11/22 13:27:54 skoglund Exp $
 *                
 ********************************************************************/
#include <mips/cpu.h>
	
.section .init
.globl	_start

	.type	_start, @function
_start:
	//j	_start
	la	sp, _mini_stack
	jal	init_platform__Fv

	jal	init__Fv
	nop
	/* we should not return!!! */
1:	
	j	1b
	nop

.section .init
	.space	1024
_mini_stack:
	
