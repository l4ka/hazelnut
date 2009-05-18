/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     asm-x86.S
 * Description:   X86-specifc glue for the kernel debugger.
 *                
 * @LICENSE@
 *                
 * $Id: asm-x86.S,v 1.11 2001/12/04 21:18:22 uhlig Exp $
 *                
 ********************************************************************/
#include <config.h>
#include <x86/config.h>
#include <x86/cpu.h>
#include <tcb_layout.h>

#if (__GNUC__ >= 3)
# define KDEBUG_INIT   _Z11kdebug_initv
# define KMEM_ALLOC    _Z10kmem_allocj
# define TRACE_PF_HANDLER	_Z16tracs_pf_handlerjjjjj
# define TRACE_IPC_HANDLER	_Z17trace_ipc_handllerjjjjjjj
# define PMC_OVERFLOW_HANDLER	_Z20pmc_overflow_handlerv
# define TRACE_EXCEPTION_HANDLER	_Z17exception_handlerP17exception_frame_t
#else
# define KDEBUG_INIT   kdebug_init__Fv
# define KMEM_ALLOC    kmem_alloc__FUi
# define TRACE_PF_HANDLER	trace_pf_handler__FUiUiUiUiUi
# define TRACE_IPC_HANDLER	trace_ipc_handler__FUiUiUiUiUiUiUi
# define PMC_OVERFLOW_HANDLER	pmc_overflow_handler__Fv
# define TRACE_EXCEPTION_HANDLER exception_handler__FP17exception_frame_t
#endif

/*
 * Define to keep the old copy of the stack contents.  Useful if one 
 * catches faults inside KDB.
 */
//#define KEEP_OLD_STACK_CONTENTS

#if defined(CONFIG_X86_P4_BTS)
#  define DEBUGCTL_MASK	(~0x0d)
#else
#  define DEBUGCTL_MASK	(~0x01)
#endif

#if defined(CONFIG_KEEP_LAST_BRANCHES) || defined(CONFIG_X86_P4_BTS)
#define clr_dbgctl()				  \
	mov	$IA32_DEBUGCTL, %ecx		; \
	rdmsr					; \
	andl	$DEBUGCTL_MASK, %eax		; \
	wrmsr
#define set_dbgctl()				  \
	mov	$IA32_DEBUGCTL, %ecx		; \
	rdmsr					; \
	movl	x86_debugctl_state, %ebx	; \
	orl	%ebx, %eax			; \
	wrmsr
#else
#define clr_dbgctl()
#define set_dbgctl()
#endif

#define ENTRY(name) \
	.globl name; \
	.align 4; \
	name##:

#if defined(CONFIG_DEBUGGER_NEW_KDB)
#define EXCEPTION_EC(num)\
	pusha;\
	pushl	%ds;\
	pushl	%es;\
	pushl	$##num;\
	pushl	%esp;\
	call	TRACE_EXCEPTION_HANDLER;\
	addl	$8, %esp;\
	popl	%es;\
	popl	%ds;\
	popa;\
	addl	$4, %esp;\
	iret 
	
	
#define DEF_EXC_EC(num, entry)	\
	ENTRY(entry) EXCEPTION_EC(num)

#define EXCEPTION(num)\
	pushl	$0;\
	pusha;\
	pushl	%ds;\
	pushl	%es;\
	pushl	$##num;\
	pushl	%esp;\
	call	TRACE_EXCEPTION_HANDLER; \
	addl	$8, %esp;\
	popl	%es;\
	popl	%ds;\
	popa;\
	addl	$4, %esp;\
	iret 
	
	
#define DEF_EXC(num, entry)	\
	ENTRY(entry) EXCEPTION(num)
#endif /*new debugger*/
	
#define ke(text) \
	int	$3		; \
	jmp	9f		; \
	.ascii	text		; \
9:

	.section .kdebug

ENTRY(int_1)
	pushl	$0x0
	pusha
	pushl	%ds
	pushl	%es
	pushl	$1
	jmp	0f

ENTRY(int_2)
	pushl	$0x0
	pusha
	pushl	%ds
	pushl	%es
	pushl	$2
	jmp	0f


ENTRY(int_3)
	pushl	$0x0
	pusha
	pushl	%ds
	pushl	%es
	pushl	$3

0:
#if defined(CONFIG_ENABLE_SMALL_AS)
	mov	$X86_KDS, %eax
	mov	%eax, %ds
	mov	%eax, %es
#endif

#if defined(CONFIG_SMP)
enter_smp:	
	mov	%esp, %eax
	andl	$L4_TCB_MASK, %eax
	movl	OFS_TCB_INTR_PENDING+4(%eax), %ebx
	movl	$-1, %eax		/* -1 no one in lock */
	lock	cmpxchg %ebx, %ss:_kdebug_spinlock
	jz	2f
1:
	cmp	$-1, %ss:_kdebug_spinlock
	je	exit_smp
	
	/* check if our cpu id is in the spinlock */
	cmp	%ebx, %ss:_kdebug_spinlock
	jne	1b
2:		
	lock	incl %ss:_kdebug_counter
#endif

	clr_dbgctl()

	cmpl	$KDEBUG_INIT, %ss:(kernel_info_page + 0x10)
	je	2f
	call	*%ss:(kernel_info_page + 0x14)
2:	
	mov	%esp, %ss:_kdebug_saved_esp
	mov	$_kdebug_stack_top, %esp
	pushl	_kdebug_saved_esp
#if defined(KEEP_OLD_STACK_CONTENTS)
	cmpl	$_kdebug_stack_bottom, (%esp)
	jl	4f
	movl	$_kdebug_stack_bottom, %esi
	movl	$_kdebug_old_stack, %edi
	movl	$256, %ecx
	rep
	movsl	%ds:(%esi), %es:(%edi)
	addl	$(_kdebug_old_stack-_kdebug_stack_bottom), (%esp)
4:
#endif
	call	*%ss:(kernel_info_page + 0x14)
	mov	%ss:_kdebug_saved_esp, %esp

#if defined(CONFIG_SMP)
	movl	%eax, %ss:(_kdebug_spinlock)
	/*lock	decl %ss:_kdebug_counter*/
	cmp	$-1, %eax
	jne	enter_smp
exit_smp:

/*
	lock	btrl $0, %ss:_kdebug_spinlock
3:	cmpl	$0, %ss:_kdebug_counter
	jne	3b */
#endif

	set_dbgctl()
	addl	$4, %esp
	popl	%es
	popl	%ds
	popa
	addl	$4,%esp
	iret 

	

ENTRY(grabpage__Fv)
	/* save the registers the callee doesn't save itself */
	pushl	%ecx
	pushl	%edx
	pushl	$4096
	call	KMEM_ALLOC
	subl	$KERNEL_OFFSET, %eax
	addl	$4,%esp
	popl	%edx
	popl	%ecx
	ret

_kdebug_spinlock:
	.long	-1
_kdebug_counter:
	.long	0

	.section .kdebug-bss
	.global _kdebug_stack_bottom
	.global _kdebug_stack_top
	.global _kdebug_saved_esp
	.align 16
_kdebug_stack_bottom:
	.space	1024
_kdebug_stack_top:

#if defined(KEEP_OLD_STACK_CONTENTS)
_kdebug_old_stack:	
	.space	1024
#endif

#if defined(CONFIG_KEEP_LAST_BRANCHES) || defined(CONFIG_X86_P4_BTS)
	.global x86_debugctl_state
x86_debugctl_state:
	.long	0
#endif

_kdebug_saved_esp:
	.long	0

/* the assembler stubs for the new debugger */

#if defined(CONFIG_DEBUGGER_NEW_KDB)
	.section .kdebug

DEF_EXC(0, monitor_int_0)
DEF_EXC(4, monitor_int_4)
DEF_EXC(5, monitor_int_5)
DEF_EXC(6, monitor_int_6)	
DEF_EXC(7, monitor_int_7)	
DEF_EXC_EC(8, monitor_int_8)
DEF_EXC(9, monitor_int_9)
DEF_EXC_EC(10, monitor_int_10)
DEF_EXC_EC(11, monitor_int_11)
DEF_EXC_EC(12, monitor_int_12)
DEF_EXC(16, monitor_int_16)
DEF_EXC_EC(17, monitor_int_17)
DEF_EXC_EC(18, monitor_int_18)
DEF_EXC(19, monitor_int_19)


ENTRY(trace_pf)
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	call	TRACE_PF_HANDLER
	popl	%edx
	popl	%ecx
	popl	%eax
	jmp	(original_int_14)


ENTRY(trace_ipc)
	pusha
	call	TRACE_IPC_HANDLER
	popa
	jmp	(original_int_48)	


ENTRY(apic_pmc_of_int)
	pusha
	call	PMC_OVERFLOW_HANDLER
	popa
	iret


#endif /* defined(CONFIG_DEBUGGER_NEW_KDB) */
