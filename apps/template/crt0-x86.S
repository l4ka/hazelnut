/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     template/crt0-x86.S
 * Description:   example server startup code for x86 processors
 *                
 * @LICENSE@
 *                
 * $Id: crt0-x86.S,v 1.5 2001/11/30 14:20:06 ud3 Exp $
 *                
 ********************************************************************/
	.text
	.global _start
	.global _stext
_stext:	
_start:
	leal	__stack, %esp
	pushl	%eax
	pushl	%ebx
	pushl	$___return_from_main
	jmp	main

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
__stack:
