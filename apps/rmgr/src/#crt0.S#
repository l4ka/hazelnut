#define __ASSEMBLY__
#include <l4/compiler.h>
	
	.section .init

ENTRY(__crt_dummy__)
ENTRY(_start)
ENTRY(start)
	leal	SYMBOL_NAME(_stack),%esp
	pushl	%eax
	pushl	%ebx
	
	/* initialize vector for exception 6 */
	movl	$trap6_entry,%eax
	movw	%ax, _idt_offset_low
	shrl	$16, %eax
	movw	%ax, _idt_offset_high
	movw	%cs, %ax
	movw	%ax, _idt_selector

	/* load tiny interrupt descriptor table to catch exception 6 */
	lidtl	SYMBOL_NAME(_idtdesc)
	
	pushl	$SYMBOL_NAME(_exit)
	jmp	SYMBOL_NAME(startup)

ENTRY(__main)
	ret

	/* Show an error message and wait for keypress to reboot. */
trap6_entry:
	pusha
	
	cld

	/* print out warning */
	movl	$trap6_warning, %esi
	movl	$(0xb8000 + (24*80*2)), %edi
	movb	$0x0F, %ah
	
tp60:	/* read next character from string */
	lodsb

	/* check for end-of-string */
	cmpb	$0, %al
	je	tp62
	
	/* check for newline */
	cmpb	$'\n', %al
	jne	tp61
	
	pushl	%eax
	pushl	%esi
	
	/* move screen upwards 1 line */
	movl	$((24*80*2)/4), %ecx
	movl	$(0xb8000 + 1*80*2), %esi
	movl	$0xb8000, %edi
	rep	movsl

	/* clear last line of screen */
	movl	$((1*80*2)/4), %ecx
	movl	$0x07200720, %eax
	rep	stosl

	popl	%esi
	popl	%eax
	
	/* jump to new line */
	movl	$(0xb8000 + (24*80*2)), %edi
	jmp	tp60
	
tp61:	/* print character */
	stosw
	jmp	tp60

tp62:	/* wait for keypress */
	inb	$0x64, %al
	testb	$0x01, %al
	je	tp62

	movb	%al, %ah

	/* empty keyboard buffer */
	inb	$0x60, %al
	
	/* ignore PS/2 mouse events */
	testb	$0x20, %ah
	jne	tp62

	jmp	SYMBOL_NAME(my_pc_reset)


	/* the warning text */
trap6_warning:
	.string	"\nRMGR: Invalid opcode detected! Be sure that you don't use any CPU\n      optimization flags like -march=i686 when compiling RMGR and\n      the depending libraries from OSKit 0.6.\n\n      Press any key to reboot..."

        /* MultiBoot header - see multiboot.h.  */
#ifdef __ELF__
        .align  4
#else /* ! __ELF__, that mean a.out assembler */
        .align  2
#endif

ENTRY(_mb_header)
        .long   0x1BADB002              /* magic */
        .long   0x00010000              /* flags: AOUT_KLUDGE */
        .long   - 0x00010000 - 0x1BADB002
        .long   SYMBOL_NAME(_mb_header) /* header_addr */
        .long   SYMBOL_NAME(__crt_dummy__) /* load_addr */
        .long   _edata			/* load_end_addr */
        .long   _end			/* bss_end_addr */
        .long   SYMBOL_NAME(_start)	/* entry */
	
	.word	0
_idtdesc:
	.word	(7*8)-1
	.long	_idt

_idt:
	.word	0,0,0,0			/* trap 0 */
	.word	0,0,0,0			/* trap 1 */
	.word	0,0,0,0			/* trap 2 */
	.word	0,0,0,0			/* trap 3 */
	.word	0,0,0,0			/* trap 4 */
	.word	0,0,0,0			/* trap 5 */

_idt_offset_low:
	.word	0
_idt_selector:
	.word	0
	.byte	0
	.byte	0xee
_idt_offset_high:
	.word	0

	.bss

	.space	8192
ENTRY(_stack)
