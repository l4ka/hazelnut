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
_stack_bottom:
	.space	8192
_stack_top:
