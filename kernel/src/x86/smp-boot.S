/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/smp-boot.S
 * Description:   Assembly glue for booting SMP.
 *                
 * @LICENSE@
 *                
 * $Id: smp-boot.S,v 1.3 2001/11/22 13:39:07 skoglund Exp $
 *                
 ********************************************************************/
#include <config.h>
#include <x86/config.h>
#include <x86/cpu.h>


#if (__GNUC__ >= 3)
# define INIT_SMP		_Z8init_smpv
#else
# define INIT_SMP		init_smp__Fv
#endif

#if defined(CONFIG_SMP)
	.section	.init
/* startup of application processor after switching to protected mode */
_start_ap:
	mov	$X86_KDS, %ax
	mov	%ax, %ds
	mov	%ax, %ss
	mov	%ax, %es
	mov	%ax, %fs
	jmp	1f
1:

	/* load temporary stack pointer */
	lea	_mini_stack-4, %esp
	/* set up paging */
	call	INIT_SMP
	

.code16
	
.globl	_start_smp
.globl  _end_smp
.align	4
_start_smp:
_base = .
	/* disable ints */
	cli	

	/* equal code and data segment */
	mov	%cs, %ax
	mov	%ax, %ds

	/* setup a boot gdt, (ptr - _base) for .code16 relocation */
	lgdt	_smp_gdt - _base

	/* enable protected mode */
	xor	%ax, %ax
	inc	%ax
	lmsw	%ax
	jmp	1f
1:
	nop
	nop
	
	ljmpl	$X86_KCS, $_start_ap
	
	/* does not return */

.align	16
_smp_gdt:
	.word	3*8	/* this GDT has 3 entries: NULL, KCS, KDS */
	.long	smp_boot_gdt - KERNEL_OFFSET
	.word	0	
	
_end_smp:
#endif /* CONFIG_SMP */
