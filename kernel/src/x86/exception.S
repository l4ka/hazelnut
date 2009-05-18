/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     x86/exception.S
 * Description:   Syscall and interrupt/exception entry points for the
 *                x86.
 *                
 * @LICENSE@
 *                
 * $Id: exception.S,v 1.67 2002/05/13 15:52:30 skoglund Exp $
 *                
 ********************************************************************/
#include <macros.h>
#include <config.h>
#include <x86/memory.h>
#include <x86/config.h>
#include <x86/cpu.h>
#include <tcb_layout.h>
	
#if (__GNUC__ >= 3)
# define SYS_IPC		_Z7sys_ipc13l4_threadid_tjj
# define SYS_ID_NEAREST		_Z14sys_id_nearest13l4_threadid_t
# define SYS_LTHREAD_EX_REGS	_Z19sys_lthread_ex_regsjjj13l4_threadid_t
# define SYS_SCHEDULE		_Z12sys_schedule16schedule_param_t13l4_threadid_t
# define SYS_THREAD_SWITCH	_Z17sys_thread_switch13l4_threadid_t
# define SYS_TASK_NEW		_Z12sys_task_new13l4_threadid_tjS_jj
# define SYS_FPAGE_UNMAP	_Z15sys_fpage_unmap7fpage_tj

# define EXCEPTION_HANDLER	_Z17exception_handlerP17exception_frame_t
# define IRQ_HANDLER		_Z11irq_handlerP17exception_frame_t
# define TIMER_IRQ_HANDLER	_Z17timer_irq_handlerP17exception_frame_t
# define PAGEFAULT_HANDLER	_Z9pagefaultjjjjj

# define ABORT_IPC		_Z9abort_ipcv
# define SWITCH_TO_ROOTTASK	_Z18switch_to_roottaskv
# define SWITCH_TO_SIGMA0	_Z16switch_to_sigma0v
# define SWITCH_TO_USER		_Z14switch_to_userv

#define APIC_HANDLER_LINT0	_Z18apic_handler_lint0v
#define APIC_HANDLER_LINT1	_Z18apic_handler_lint1v
#define APIC_HANDLER_ERROR	_Z18apic_handler_errorv
#define APIC_HANDLER_SPURIOUS	_Z25apic_handler_spurious_intv
#define APIC_HANDLER_PERFCTR	_Z20apic_handler_perfctrjPj
#define HANDLE_PERFCTR_BOUNCE	_Z26handle_perfctr_bounce_backP17exception_frame_t
#define SMP_HANDLER_COMMAND_IPI	_Z23smp_handler_command_ipiv

#else /* __GNUC__ < 3 */

# define SYS_IPC		sys_ipc__FG13l4_threadid_tUiUi
# define SYS_ID_NEAREST		sys_id_nearest__FG13l4_threadid_t
# define SYS_LTHREAD_EX_REGS	sys_lthread_ex_regs__FUiUiUiG13l4_threadid_t
# define SYS_SCHEDULE		sys_schedule__FG16schedule_param_tG13l4_threadid_t
# define SYS_THREAD_SWITCH	sys_thread_switch__FG13l4_threadid_t
# define SYS_TASK_NEW		sys_task_new__FG13l4_threadid_tUiT0UiUi
# define SYS_FPAGE_UNMAP	sys_fpage_unmap__FG7fpage_tUi		

# define EXCEPTION_HANDLER	exception_handler__FP17exception_frame_t
# define IRQ_HANDLER		irq_handler__FP17exception_frame_t
# define TIMER_IRQ_HANDLER	timer_irq_handler__FP17exception_frame_t
# define PAGEFAULT_HANDLER	pagefault__FUiUiUiUiUi

# define ABORT_IPC		abort_ipc__Fv
# define SWITCH_TO_ROOTTASK	switch_to_roottask__Fv
# define SWITCH_TO_SIGMA0	switch_to_sigma0__Fv
# define SWITCH_TO_USER		switch_to_user__Fv

#define APIC_HANDLER_LINT0	apic_handler_lint0__Fv
#define APIC_HANDLER_LINT1	apic_handler_lint1__Fv
#define APIC_HANDLER_ERROR	apic_handler_error__Fv
#define APIC_HANDLER_SPURIOUS	apic_handler_spurious_int__Fv
#define APIC_HANDLER_PERFCTR	apic_handler_perfctr__FUiPUi
#define HANDLE_PERFCTR_BOUNCE	handle_perfctr_bounce_back__FP17exception_frame_t
#define SMP_HANDLER_COMMAND_IPI	smp_handler_command_ipi__Fv
#endif

#if defined(CONFIG_USERMODE_NOIRQ)
# define MAYBE_STI
#else
# define MAYBE_STI	sti
#endif

#if defined(CONFIG_MEASURE_INT_LATENCY)
# define STORE_TIME			 \
	mov	0xf00f0390, %eax	;\
	mov	%eax, interrupted_time	;\
	rdtsc				;\
	mov	%eax, interrupted_rdtsc

	.data
	.align 16
	.globl interrupted_rdtsc
	.globl interrupted_time
interrupted_rdtsc:	.long 0
interrupted_time:	.long 0
	.text

#else
# define STORE_TIME
#endif

#if defined(CONFIG_ENABLE_SMALL_AS)
#define set_kds(_reg)		 \
	mov	$X86_KDS, %_reg	;\
	mov	%_reg, %ds

#define reset_ds(_num)			 \
	mov	_num(%esp), %eax	;\
	add	$0x8, %eax		;\
	mov	%eax, %ds

#define set_uds(_reg)		 \
	mov	$X86_UDS, %_reg	;\
	mov	%_reg, %ds
#else
#define set_kds(_reg)
#define reset_ds(_num)
#define set_uds(_reg)
#endif

#if defined(CONFIG_KEEP_LAST_BRANCHES)
#define clr_dbgctl_lbr()		  \
	mov	$0x1d9, %ecx		; \
	rdmsr				; \
	andl	$0xfffffffe, %eax	; \
	wrmsr

#define set_dbgctl_lbr()		  \
	mov	$0x1d9, %ecx		; \
	rdmsr				; \
	orl	$0x1, %eax		; \
	wrmsr
#else
#define clr_dbgctl_lbr()
#define set_dbgctl_lbr()
#endif

	
#define ke(text) \
	int	$3		; \
	jmp	1f		; \
	.ascii	text		; \
1:
	
#define ENTRY(name) \
	.globl name; \
	.p2align 4; \
	name##:

#define EXCEPTION_EC(num)\
	pusha;\
	pushl	%ds;\
	pushl	%es;\
	set_kds(eax);\
	pushl	$##num;\
	pushl	%esp;\
	clr_dbgctl_lbr();\
	call	EXCEPTION_HANDLER;\
	set_dbgctl_lbr();\
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
	set_kds(eax);\
	pushl	$##num;\
	pushl	%esp;\
	clr_dbgctl_lbr();\
	call	EXCEPTION_HANDLER;\
	set_dbgctl_lbr();\
	addl	$8, %esp;\
	popl	%es;\
	popl	%ds;\
	popa;\
	addl	$4, %esp;\
	iret 
	
	
#define DEF_EXC(num, entry)	\
	ENTRY(entry) EXCEPTION(num)

#define APIC_SMP_CMD_ENTRY	\
	ENTRY(apic_smp_command_ipi)\
	pusha;\
	set_kds(eax);\
	call	SMP_HANDLER_COMMAND_IPI;\
	reset_ds(36);\
	popa;\
	iret
	
#define DEF_INTR(num)		  \
	ENTRY(hwintr_##num)	; \
	pushl $0		; \
	pusha			; \
	pushl	%ds		; \
	pushl	%es		; \
	set_kds(eax)		; \
	STORE_TIME		; \
	pushl	$##num		; \
	pushl	%esp		; \
	call	IRQ_HANDLER	; \
	addl	$8, %esp	; \
	popl	%es		; \
	popl	%ds		; \
	popa			; \
	addl	$4, %esp	; \
	iret 

.text

DEF_EXC(0, int_0)
DEF_EXC(4, int_4)
DEF_EXC(5, int_5)
DEF_EXC(6, int_6)	
DEF_EXC(7, int_7)	
DEF_EXC_EC(8, int_8)
DEF_EXC(9, int_9)
DEF_EXC_EC(10, int_10)
DEF_EXC_EC(11, int_11)
DEF_EXC_EC(12, int_12)
DEF_EXC(16, int_16)
DEF_EXC_EC(17, int_17)
DEF_EXC_EC(18, int_18)
DEF_EXC(19, int_19)

DEF_INTR(0)
DEF_INTR(1)
DEF_INTR(2)
DEF_INTR(3)
DEF_INTR(4)
DEF_INTR(5)
DEF_INTR(6)
DEF_INTR(7)
DEF_INTR(8)
DEF_INTR(9)
DEF_INTR(10)
DEF_INTR(11)
DEF_INTR(12)
DEF_INTR(13)
DEF_INTR(14)
DEF_INTR(15)


ENTRY(int_13)
	pusha
	pushl	%ds
	pushl	%es
	set_kds(eax)
	pushl	$13
	pushl	%esp
	clr_dbgctl_lbr()
	call	EXCEPTION_HANDLER
	set_dbgctl_lbr()
	addl	$8, %esp
	popl	%es
	popl	%ds
	popa
	addl	$4, %esp
	iret	

ENTRY(int_14)
	/* At this point we have the standard exception frame with an error
	   code on our stack. We need only the error code and the instruction
	   pointer in the pagefault handler. But we have to save eax, ecx and
	   edx because the called C function would clobber them. Thus we have
	   three more arguments for the pagefault handling function (that are
	   ignored):

	   void pagefault(dword_t edx, dword_t ecx, dword_t eax,
			  dword_t errcode, dword_t ip); 
	 */
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	set_kds(eax)
	clr_dbgctl_lbr()
	call	PAGEFAULT_HANDLER
	set_dbgctl_lbr()
	reset_ds(20)
	popl	%edx
	popl	%ecx
	popl	%eax
	addl	$4, %esp
	iret
ENTRY(timer_irq)
	pushl $0
	pusha
	pushl	%ds
	pushl	%es
	set_kds(eax)
	pushl	$0
	pushl	%esp
	call	TIMER_IRQ_HANDLER
	addl	$8, %esp
	popl	%es
	popl	%ds
	popa
	addl	$4, %esp
	iret

#ifdef CONFIG_X86_APIC
ENTRY(apic_lint0)
	pusha
	set_kds(eax)
	call	APIC_HANDLER_LINT0
	reset_ds(36)
	popa
	iret

ENTRY(apic_lint1)
	pusha
	set_kds(eax)
	call	APIC_HANDLER_LINT1
	reset_ds(36)
	popa
	iret

ENTRY(apic_error)
	pusha
	set_kds(eax)
	call	APIC_HANDLER_ERROR
	reset_ds(36)
	popa
	iret

ENTRY(apic_spurious_int)
	pusha
	set_kds(eax)
	call	APIC_HANDLER_SPURIOUS
	reset_ds(36)
	popa
	iret


#if defined(CONFIG_ENABLE_PROFILING)
ENTRY(apic_perfctr)
	pusha
	pushl	%ds
	pushl	%es
	set_kds(eax)
	lea	48(%esp), %eax			// EFlags
	push	%eax
	push	44(%esp)			// Fault address
	call	APIC_HANDLER_PERFCTR
	addl	$8, %esp
	popl	%es
	popl	%ds
	popa
	iret

ENTRY(perfctr_bounce_back)
	pushl	$0x0 // Error code
	pusha
	pushl	%ds
	pushl	%es
	set_kds(eax)
	pushl	$0x0 // Fault code
	pushl	%esp
	call	HANDLE_PERFCTR_BOUNCE
	addl	$8, %esp
	popl	%es
	popl	%ds
	popa
	addl	$4, %esp
	iret
#endif /* CONFIG_ENABLE_PROFILING */

#endif /* CONFIG_X86_APIC */

#if defined(CONFIG_SMP)
/* entry point for smp command IPIs */
APIC_SMP_CMD_ENTRY
#endif
	
/* syscalls */

	.section .ipc-s
/*
 ipc
 precondition:
	snd_desc:	eax
	timeout:	ecx
	msg_w0:		edx
	msg_w1:		ebx
	msg_w2:		edi
	rcv_desc:	ebp
	dest:		esi

 postcondition
	msg_dope:	eax
	msg_w0:		edx
	msg_w1:		ebx
	msg_w2:		edi
	source:		esi
*/
ENTRY(int_48)
	pushl	%ebp
	pushl	%eax
	pushl	%esi
	set_kds(ebp)

#if 0	
	/* save registers into tcb */
	lea	-L4_TOTAL_TCB_SIZE+12+24+16(%esp), %esp
	pushl	%ecx
	pushl	%edi
	pushl	%ebx
	pushl	%edx
	lea	L4_TOTAL_TCB_SIZE-12-24(%esp), %esp
	call	SYS_IPC
	addl	$12, %esp
#else
	/* save registers into tcb */
	movl	%esp, %eax
	andl	$L4_TCB_MASK, %eax
	movl	%edx, 0(%eax)
	movl	%ebx, 4(%eax)
	movl	%edi, 8(%eax)
	movl	%ecx, 12(%eax)
	call	SYS_IPC
	addl	$12, %esp
#endif

	set_uds(ebp)
	iret

#if defined(CONFIG_IA32_FEATURE_SEP)
/*
 ipc
 precondition:
	snd_desc:	eax
	msg_w0:		edx
	msg_w1:		ebx
	msg_w2:		edi
	dest:		esi
	user sp:	ecx
	timeout:	(ecx+0x0c)
	rcv_desc:	(ecx+0x08)
	user ip:	(ecx+0x00)
	
 postcondition
	msg_dope:	eax
	msg_w0:		edx
	msg_w1:		ebx
	msg_w2:		edi
	source:		esi
*/


/*
 * Define to create a full interrupt stack frame upon sysenter.
*/
//#define CREATE_STACKFRAME

/*
 * Define to enable possibly slightly faster code which is not safe to
 * be preempted (e.g., profiled) due to strange ESP usage.
 */
//#define PREEMPTION_UNSAFE_CODE


#if defined(CONFIG_ENABLE_SMALL_AS)

/*
**
**	==================================================
**	SYSENTER/SYSEXIT IPC CODE FOR SMALL ADDRESS SPACES
**	==================================================
**
*/
	.globl	return_from_sys_ipc_small

ENTRY(ipc_sysenter_small)
	popl	%esp

#if	defined(CREATE_STACKFRAME)
	pushl	$X86_UDS		/* User SS */
	pushl	%ecx			/* User ESP */
	pushl	$0x00			/* User EFlags */
	pushl	$X86_UCS		/* User CS */
	pushl	(%ecx)			/* User EIP */
#else
	subl	$4, %esp
	pushl	%ecx			/* User ESP */
	subl	$8, %esp
	pushl	(%ecx)			/* User EIP */
#endif

	pushl	8(%ecx)			/* rcv desc */
	pushl	%eax			/* snd desc */
	pushl	%esi			/* dest	id */
	movl	12(%ecx), %ecx

#if defined(CONFIG_IPC_FASTPATH)

	/*
	 * Fastpath IPC code
	 */

#define L4_TIDTCB_MASK	0xE0FFFC00
#define L4_TASK_MASK	0x00FF0000
#define to_tcb		%eax
#define current		%ebp
	
	orl	%ebp, %eax	/* eax = snd_desc | rcv_desc */
	orl	%ecx, %eax	/* eax = snd_desc | rcv_desc | timeout */

	/*
	 * If have no snd_desc, no rcv_desc, and no timeout it means that
	 * we have a short send, short receive, timeout never IPC (i.e.,
	 * fastpath IPC).
	 */
	jne	ipc_slowpath_to_small

	/* Calculate TCB address of sender and receiver. */
	movl	%esi, to_tcb
	orl	$TCB_AREA, to_tcb
	andl	$L4_TIDTCB_MASK, to_tcb
	movl	%ss:OFS_TCB_MYSELF(to_tcb), %ecx	// Load in advance
	mov	%esp, current
	andl	$L4_TCB_MASK, current

	/* Check if destination is valid (to_tcb->mysels == dest). */
	cmpl	%ecx, %esi
	jne	ipc_slowpath_small

	/*
	 * Check: - Is destination WAITING?
	 *	  - Is destination handling an IPC copy pagefault?
	 *	  - Does destination require any resources?
	 *	  - Do we have have an interrupt pending?
	 */
	movl	%ss:OFS_TCB_THREAD_STATE(to_tcb), %ecx
	add	$1, %ecx
	orl	%ss:OFS_TCB_UNWIND_IPC_SP(to_tcb), %ecx
	orl	%ss:OFS_TCB_RESOURCES(to_tcb), %ecx
	orl	%ss:OFS_TCB_INTR_PENDING(current), %ecx
	jne	ipc_slowpath_small

	/* Does destination perform an open-wait (to_tcb->partner == 0). */
	orl	%ss:OFS_TCB_PARTNER(to_tcb), %ecx
	je	1f

	/* Does destination wait for us (to_tcb->partner == myself). */
	cmpl	%ss:OFS_TCB_MYSELF(current), %ecx
	jne	ipc_slowpath_small

1:	/*
	 * If we reach this point we're performing the fastpath IPC.
	 */

	/* Set current->partner = dest */
	movl	%esi, %ss:OFS_TCB_PARTNER(current)

	/* Set current->msg_desc = 0 */
	xorl	%esi, %esi
	movl	%esi, %ss:OFS_TCB_MSG_DESC(current)

	/* Set current->state = WAITING, to_tcb->state = RUNNING */
	subl	$1, %esi
	movl	%esi, %ss:OFS_TCB_THREAD_STATE(current)
	movl	$6, %ss:OFS_TCB_THREAD_STATE(to_tcb)
	
	/* Create stack frame to use if slow path IPC is used for reply. */
	pushl	$ipc_fastpath_ret_small
	movl	%esp, %ss:OFS_TCB_STACK(current)

	/* Set the SOURCE return parameter of receiver. */
	movl	%ss:OFS_TCB_MYSELF(current), %esi

	/* Calculate stack top of destination thread. */
	leal	%ss:L4_TOTAL_TCB_SIZE-4(to_tcb), %esp

	/* Check if sender and receiver reside in same task. */
	xorl	to_tcb, current
	testl	$L4_TASK_MASK, current
	je	5f

	/* Check small space ID of target thread (from inside pagedir). */
	movl	%ss:OFS_TCB_SPACE(to_tcb), %ecx
	addl	$(KERNEL_OFFSET + SPACE_ID_IDX*4), %ecx
	movl	%ss:(%ecx), %eax
	movl	%ss:__is_small, %ebp
	cmpb	$0xff, %al
	je	3f

	testl	%ebp, %ebp
	jne	2f
	movl	$1, %ss:__is_small

2:	/* Get segment descriptors from within page directory. */
	movl	%ss:4(%ecx), %eax
	movl	%ss:8(%ecx), %ecx

	/* Reload segment descriptior table. */
#if	!defined(PREEMPTION_UNSAFE_CODE)
	movl	$gdt, %ebp
	movl	%eax, %ss:32(%ebp)
	movl	%ecx, %ss:36(%ebp)
	orl	$0x800, %ecx
	movl	%eax, %ss:24(%ebp)
	movl	%ecx, %ss:28(%ebp)
#else
	movl	%esp, %ebp
	movl	$(gdt+40), %esp
	pushl	%ecx
	pushl	%eax
	orl	$0x800, %ecx
	pushl	%ecx
	pushl	%eax
	movl	%ebp, %esp
#endif

	/* Reload segment selectors. */
	movl	$X86_UDS, %ecx
	mov	%ecx, %ds
	mov	%ecx, %es
	mov	%ecx, %fs
	mov	%ecx, %gs
	jmp	5f

3:	/* Check if we previously ran in a small space. */
	movl	%ss:__is_small, %ebp
	testl	%ebp, %ebp
	je	4f

	/* Reload 3GB segment descriptors. */
#if	!defined(PREEMPTION_UNSAFE_CODE)
	movl	$gdt, %ebp
	movl	$0x0000ffff, %ss:24(%ebp)
	movl	$0x00cbfb00, %ss:28(%ebp)
	movl	$0x0000ffff, %ss:32(%ebp)
	movl	$0x00cbf300, %ss:36(%ebp)
#else
	movl	%esp, %ebp
	movl	$(gdt+40), %esp
	pushl	$0x00cbf300
	pushl	$0x0000ffff
	pushl	$0x00cbfb00
	pushl	$0x0000ffff
	movl	%ebp, %esp
#endif

	/* Reload segment selectors. */
	movl	$X86_UDS, %ecx
	mov	%ecx, %ds
	mov	%ecx, %es
	mov	%ecx, %fs
	mov	%ecx, %gs

	/* Reload page directory pointer. */
4:	movl	OFS_TCB_PAGEDIR_CACHE-L4_TOTAL_TCB_SIZE+4(%esp), %ecx
	movl	%ecx, %cr3
	movl	$0, %ss:__is_small

5:	/* Reload esp0 in TSS. */
	movl	%esp, %ss:__tss + 4

	movl	-8(%esp), %ecx			/* User ESP */
	movl	%edx, %ebp			/* Save msg.w0 */
	lea	sysexit_tramp_keepds, %edx	/* EIP for trampoline */
	xorl	%eax, %eax			/* Return value */
	sysexit
				
ipc_slowpath_small:
	xorl	%ecx, %ecx		/* Restore timeout value. */

ipc_slowpath_to_small:
#endif /* CONFIG_IPC_FASTPATH */

	set_kds(eax)			/* It is now safe to set kernel ds */

	/* Save registers into tcb */
	movl	%esp, %eax
	andl	$L4_TCB_MASK, %eax
	movl	%edx, 0(%eax)		/* dw0 */
	movl	%ebx, 4(%eax)		/* dw1 */
	movl	%edi, 8(%eax)		/* dw2 */
	movl	%ecx, 12(%eax)		/* timeout */

	call	SYS_IPC
return_from_sys_ipc_small:

	movl	24(%esp), %ecx		/* User ESP */
	movl	%edx, %ebp		/* Save msg.w0 */
	lea	sysexit_tramp, %edx	/* EIP for trampoline */
	sysexit

	/* should never be reached */
	ke("ipc_sysenter fell off")
	# does not return

#if defined(CONFIG_IPC_FASTPATH)
ipc_fastpath_ret_small:
	movl	%esp, %ecx
	andl	$L4_TIDTCB_MASK, %ecx
	movl	$6, OFS_TCB_THREAD_STATE(%ecx)		/* set to running */
	movl	OFS_TCB_IPC_BUFFER+0(%ecx), %ebp	/* dw0	*/
	movl	OFS_TCB_IPC_BUFFER+4(%ecx), %ebx	/* dw1	*/
	movl	OFS_TCB_IPC_BUFFER+8(%ecx), %edi	/* dw2	*/
	movl	OFS_TCB_MSG_DESC(%ecx), %eax		/* msg_desc */
	movl	OFS_TCB_PARTNER(%ecx), %esi		/* set sender */
	addl	$L4_TOTAL_TCB_SIZE - 4, %ecx

	movl	24(%esp), %ecx		/* User ESP */
	movl	%edx, %ebp		/* Save msg.w0 */
	lea	sysexit_tramp, %edx	/* EIP for trampoline */
	sysexit
#endif /* CONFIG_IPC_FASTPATH */

	/*
	 * Trampoline for entering userlevel in a safe manner.
	 */
	.section .utramp, "ax", @progbits
	.type sysexit_tramp, @function
	.globl sysexit_tramp
sysexit_tramp:
	movl	%ebp, %edx	/* Restore msg.w0 */
	movl	$X86_UDS, %ebp
	mov	%ebp, %ds
	mov	%ebp, %ss
	MAYBE_STI
	lret	$8

#if defined(CONFIG_IPC_FASTPATH)
	.p2align 4
	.type sysexit_tramp_keepds, @function
	.globl sysexit_tramp_keepds
sysexit_tramp_keepds:
	movl	%ebp, %edx	/* Restore msg.w0 */
	movl	$X86_UDS, %ebp
	mov	%ebp, %ss
	MAYBE_STI
	lret	$8
#endif /* CONFIG_IPC_FASTPATH */


	/*
	 * If we caught a GP doing the LRET instruction (i.e., invalid
	 * code segment) we return through an IRET which sets the proper
	 * code segment.  If we got a pagefault during the LRET, the IRET
	 * back into the trampoline will fail because it is not accessible
	 * with CS=0x1b.  In both cases we reenter user-level through this
	 * stub.
	 */
	.p2align 4
	.globl reenter_sysexit
reenter_sysexit:
	set_uds(ebp)
	movl	(%ecx), %ebp	/* Get EIP */
	addl	$16, %ecx	/* Adjust for user ESP */

	pushl	$X86_UDS	/* User SS */
	pushl	%ecx		/* User ESP */
	pushl	$X86_USER_FLAGS	/* User EFlags */
	pushl	$X86_UCS	/* User CS */
	pushl	%ebp		/* User EIP */
	iret

	.previous
#endif /* CONFIG_ENABLE_SMALL_AS */

	.globl	return_from_sys_ipc




/*
**
**	==================================================
**	SYSENTER/SYSEXIT IPC CODE FOR LARGE ADDRESS SPACES
**	==================================================
**
*/

ENTRY(ipc_sysenter)
	mov	(%esp),%esp

#if	defined(CREATE_STACKFRAME)
	pushl	$X86_UDS		/* User SS */
	pushl	%ecx			/* User ESP */
	pushl	$0x00			/* User EFlags */
	pushl	$X86_UCS		/* User CS */
	pushl	(%ecx)			/* User EIP */
#else
	subl	$4, %esp
	pushl	%ecx			/* User ESP */
	subl	$8, %esp
	pushl	(%ecx)			/* User EIP */
#endif

	pushl	8(%ecx)			/* rcv desc */
	pushl	%eax			/* snd desc */
	pushl	%esi			/* dest	id */
	movl	12(%ecx),%ecx

#if defined(CONFIG_IPC_FASTPATH)

#define L4_TIDTCB_MASK	0xE0FFFC00
#define to_tcb %eax
#define current %ebp
	
	orl	%ebp, %eax	/* eax = snd_desc | rcv_desc */
	orl	%ecx, %eax	/* eax = snd_desc | rcv_desc | timeout	*/
	
	/*
	 * If have no snd_desc, no rcv_desc, and no timeout it means that
	 * we have a short send, short receive, timeout never IPC (i.e.,
	 * fastpath IPC).
	 */
	jne	ipc_slowpath_to

	/* Calculate TCB address of sender and receiver. */
	movl	%esi, to_tcb
	orl	$TCB_AREA, to_tcb
	andl	$L4_TIDTCB_MASK, to_tcb
	mov	%esp, current
	andl	$L4_TCB_MASK, current

	cmpl	OFS_TCB_MYSELF(to_tcb), %esi
	jne	ipc_slowpath			/* to_tcb->myself != dest */
	movl	OFS_TCB_THREAD_STATE(to_tcb), %ecx
	add	$1,%ecx
	orl	OFS_TCB_UNWIND_IPC_SP(to_tcb),%ecx
	orl	OFS_TCB_RESOURCES(to_tcb),%ecx
	orl	OFS_TCB_INTR_PENDING(current), %ecx
	jne	ipc_slowpath			/* ! waiting	*/

	/* COND: ecx == 0 */
	orl	OFS_TCB_PARTNER(to_tcb), %ecx	/* to_tcb->partner -> ecx */
	je	1f				/* open wait	*/
	cmpl	OFS_TCB_MYSELF(current), %ecx
	jne	ipc_slowpath			/* to_tcb->partner != myself */
	/* if we reach that point we perform the fast ipc */
1:
	movl	%esi,OFS_TCB_PARTNER(current)	/* current->partner = dest */
	xorl	%esi,%esi
	movl	%esi,OFS_TCB_MSG_DESC(current)	/* current->msg_desc = 0 */
	subl	$1,%esi
	movl	%esi,OFS_TCB_THREAD_STATE(current) /* cur->state = WAITING */
	movl	$6,OFS_TCB_THREAD_STATE(to_tcb)	/* to_tcb->state = RUNNING */
	
	/* create "nice" stack frame for slow path switch */
	pushl	$ipc_fastpath_ret
	movl	%esp, OFS_TCB_STACK(current)	/* save stack */

	movl	OFS_TCB_MYSELF(current),%esi	/* set sender	*/
	xorl	%eax,%ebp
	testl	$0xFF0000,%ebp			/* same X.0 task ? */
	je	2f
	movl	OFS_TCB_SPACE(to_tcb), %ecx
	/* switch pagetable */
	movl	%ecx, %cr3
2:

	/* we don't care about the other guys stack :) */
	addl	$L4_TOTAL_TCB_SIZE - 4, %eax
	movl	%eax, __tss + 4
	movl	%edx,%ebp			/* save dw0 around sysexit */
	movl	-8(%eax),%ecx			/* user esp */
	movl	-20(%eax),%edx			/* user eip */
	addl	$16, %ecx
	subl	$2,%edx
	subl	%eax,%eax
	sti
	sysexit
				
ipc_slowpath:
	xorl	%ecx,%ecx			/* restore timeout */
ipc_slowpath_to:
#endif /* CONFIG_IPC_FASTPATH */

	/* Save registers into tcb */
	movl	%esp, %eax
	andl	$L4_TCB_MASK, %eax
	movl	%edx, 0(%eax)		/* dw0 */
	movl	%ebx, 4(%eax)		/* dw1 */
	movl	%edi, 8(%eax)		/* dw2 */
	movl	%ecx, 12(%eax)		/* timeout */

	call	SYS_IPC
return_from_sys_ipc:
	mov	%edx,%ebp

	movl	24(%esp), %ecx
	movl	12(%esp), %edx
	addl	$16, %ecx
	subl	$2, %edx	/* Adjust for the ebp->edx instruction. */
	MAYBE_STI		/* Fake POPF.  Interrupts will still be
				   disabled during the SYSEXIT instruction. */

	/*
	 * We usually run on UDS when doing SYSEXIT.  Only when we have a
	 * pagefault during IPC will we run with KDS (int_14 will reset
	 * %ds according to the value of %cs on the iframe).  This will
	 * result in running with CPL=3, RPL=3, and DPL=0 upon returning
	 * to user-level.  Such behavior, however, is not allowed by the
	 * architecture, and is prevented by 1) doing proper privilege
	 * checks upon segment register reloading, and 2) having the IRET
	 * instruction enforce non-valid data segment registers (i.e.,
	 * segment registers with DPL <= than both CPL and RPL) to get
	 * a zero value.
	 *
	 * For the SYSEXIT instruction the story is somewhat different
	 * and is not documented in the Intel specifications.  A SYSEXIT
	 * instruction will not enforce a zero value into any segment
	 * registers.  It will instead magically load the correct value
	 * (the same as %ss) into the register one instruction after
	 * returning to user-level.  The instruction which is executed
	 * with the wrong segment register values will still have internally
	 * correct values (i.e., DPLs set to 3).
	 *
	 * The IPC path here works fine for most cases.  Only when we
	 * run with a small space kernel, no small spaces have yet been
	 * created, we do a sysenter long IPC, catch a pagefault during
	 * IPC copy, return to user-level, and is interrupted on the first
	 * instruction do we have a problem.  This will cause KDS value
	 * to visible in the %ds upon kernel entry, pushed on the stack,
	 * popped off the stack before IRET, and the zero value to be
	 * enforced into %ds.  Next time any data is accessed through %ds,
	 * we will catch a GP(0).  See interrupt.c for the handling of
	 * this exception.
	 */
	sysexit

	/* Should never be reached. */
	ke("ipc_sysenter fell off")
	# does not return

#if defined(CONFIG_IPC_FASTPATH)
ipc_fastpath_ret:
	movl	%esp, %ecx
	andl	$L4_TIDTCB_MASK, %ecx
	movl	$6, OFS_TCB_THREAD_STATE(%ecx)		/* set to running */
	movl	OFS_TCB_IPC_BUFFER+0(%ecx), %ebp	/* dw0	*/
	movl	OFS_TCB_IPC_BUFFER+4(%ecx), %ebx	/* dw1	*/
	movl	OFS_TCB_IPC_BUFFER+8(%ecx), %edi	/* dw2	*/
	movl	OFS_TCB_MSG_DESC(%ecx), %eax		/* msg_desc */
	movl	OFS_TCB_PARTNER(%ecx), %esi		/* set sender */
	addl	$L4_TOTAL_TCB_SIZE - 4, %ecx
	movl	-20(%ecx), %edx		/* user eip		*/
	movl	-8(%ecx), %ecx		/* user esp		*/
	addl	$16, %ecx
	subl	$2, %edx
	sti
	sysexit
#endif /* CONFIG_IPC_FASTPATH */

	/*
	 * Alternate exit stub used if some thread did an ex_regs on us
	 * before we returned to user-level.  Needed because the ex_regs
	 * possibly changed the the user-esp to something not containing
	 * a proper return address.
	 */
	.globl alternate_sys_ipc_return
alternate_sys_ipc_return:
	addl	$12, %esp
	movl	$X86_UCS, 4(%esp)
	movl	$X86_USER_FLAGS, 8(%esp)
	movl	$X86_UDS, 16(%esp)
	set_uds(ebp)
	iret


#endif	/* defined(CONFIG_IA32_FEATURE_SEP) */
	.text


ENTRY(int_49)
	pushl	%esi
	set_kds(eax)
	call	SYS_ID_NEAREST
	ke("sys_id_nearest returned")	
	# does not return

ENTRY(int_50)
	pushl	%ecx
	pushl	%eax
	set_kds(eax)
	call	SYS_FPAGE_UNMAP
	addl	$8, %esp
	set_uds(edx)
	iret

ENTRY(int_51)
	pushl	%esi
	set_kds(eax)
	call	SYS_THREAD_SWITCH
	ke("sys_thread_switch returned")	
	# does not return

ENTRY(int_52)
	pushl	%esi
	pushl	%eax
	set_kds(eax)
	call	 SYS_SCHEDULE
	ke("sys_schedule returned")	
	# does not return

/*
 lthread_ex_regs
 precondition:
	tid:	eax
	esp:	ecx
	eip:	edx
	pager:	esi

 postcondition:
	tid:	eax
	esp:	ecx
	eip:	edx
	pager:	esi
*/
ENTRY(int_53)
	pushl	%esi
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	set_kds(eax)
	call	SYS_LTHREAD_EX_REGS
	# does not return

/*
 create_thread
 precondition:
	master:	eax
	esp:	ecx
	eip:	edx
	pager:	ebx
	tid:	esi

 postcondition:
	none yet
*/
ENTRY(int_54)
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%eax
	pushl	%esi
	set_kds(eax)
	call	SYS_TASK_NEW

	
		
		
ENTRY(ABORT_IPC)
	set_uds(eax)
	/* load the ipc error code into eax */
	popl	%eax
	iret

ENTRY(SWITCH_TO_ROOTTASK)
	mov	$X86_UDS, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %gs
#if defined(CONFIG_TRACEBUFFER)
	mov	$X86_TBS, %eax
#endif
	mov	%ax, %fs

	/* load the address of the multiboot info structure into ebx */
	popl	%ebx
	/* this signals a valid mbi address in eax */
	movl	$0x2BADB002,%eax
	iret

ENTRY(SWITCH_TO_SIGMA0)
	mov	$X86_UDS, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %gs
#if defined(CONFIG_TRACEBUFFER)
	mov	$X86_TBS, %eax
#endif
	mov	%ax, %fs
	/* load the address of the kernel_info_page into eax */
	popl	%eax
	subl	$KERNEL_OFFSET, %eax
	iret

/* this is the general entry point for all user threads */
ENTRY(SWITCH_TO_USER)
	mov	$X86_UDS, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %gs
#if defined(CONFIG_TRACEBUFFER)
	mov	$X86_TBS, %eax
#endif
	mov	%ax, %fs
	addl	$4, %esp
	iret
