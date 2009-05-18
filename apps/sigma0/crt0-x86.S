/*********************************************************************
 *                
 * Copyright (C) 1999-2002,  Karlsruhe University
 *                
 * File path:     sigma0/crt0-x86.S
 * Description:   Startup code for x86 family processors.
 *                
 * @LICENSE@
 *                
 * $Id: crt0-x86.S,v 1.7 2002/05/06 13:52:51 ud3 Exp $
 *                
 ********************************************************************/
#include <config.h>

	.text
	.global _start
	.global _stext
_stext:	
_start:
	/* do we have a reasonable KIP pointer on the stack ???
	   Jochen/Fiasco */
	testl	$0x00000FFF, (%esp)	
	jne	0f
	/* if not, it may already be in eax - L4/KA */
	popl	%eax
0:	leal	stack, %esp
	pushl	%eax
	pushl	$___return_from_main
	jmp	sigma0_main

#if 1
	.align 16, 0x90
__mb_header:
	.long   0x1BADB002;
	.long   0x00010000;
	.long   - 0x00010000 - 0x1BADB002;
	.long   __mb_header;
	.long   _start;
	.long   _edata;
	.long   _end;
	.long   _start;
#endif
	
___return_from_main:
	int $3
	jmp 1f
	.ascii "System stopped."
1:	jmp ___return_from_main
		
	.bss

	.space	1024
stack:
