	.text
	.global _start
	.global _stext
_stext:	
_start:
	int3
	nop
	leal	__stack, %esp
	pushl	%ebx	/* mbi ptr	*/
	pushl	%eax	/* mb magic	*/
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
