/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/smp.h
 * Description:   x86 SMP definitions and inlines.
 *                
 * @LICENSE@
 *                
 * $Id: smp.h,v 1.15 2001/12/04 20:16:26 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __X86_SMP_H__
#define __X86_SMP_H__

// if we have smp we __ALWAYS__ need APIC handling 
// so include <apic.h> right here.
#include <x86/apic.h>

INLINE dword_t get_apic_cpu_id()
{
    return ((get_local_apic(X86_APIC_ID) >> 24) & 0xf);
}

INLINE dword_t get_cpu_id()
{
    return get_idle_tcb()->cpu;
}

extern dword_t boot_cpu;
INLINE dword_t get_boot_cpu()
{
    return boot_cpu;
}

INLINE int is_boot_cpu()
{
    return (get_apic_cpu_id() == get_boot_cpu());
}

INLINE void send_command_ipi(dword_t cpu)
{
    set_local_apic(X86_APIC_INTR_CMD2, cpu << (56 - 32));
    set_local_apic(X86_APIC_INTR_CMD1, APIC_SMP_COMMAND_IPI);
}

INLINE void broadcast_command_ipi()
{
    /* send to all cpu's excl. myself */
    set_local_apic(X86_APIC_INTR_CMD2, 0xff << (56 - 32));
    set_local_apic(X86_APIC_INTR_CMD1, (APIC_SMP_COMMAND_IPI + get_cpu_id()) |
		   0xc0000);
}

void set_cpu_online(dword_t cpu) L4_SECT_INIT;
int is_cpu_online(dword_t cpu) L4_SECT_INIT;

/* kdebug control between cpu's */
void smp_enter_kdebug();
void smp_leave_kdebug();

void smp_startup_processors() L4_SECT_INIT;

/**********************************************************************
 *
 *          command definitions for inter-cpu communication
 *
 **********************************************************************/

#define SMP_CMD_FLUSH_TLB		1

#define SMP_CMD_IPC_SHORT		10
#define SMP_CMD_IPC_START		11
#define SMP_CMD_IPC_END			12
#define SMP_CMD_IPC_RECEIVE		13

#define SMP_CMD_THREAD_EX_REGS		20
#define SMP_CMD_THREAD_GET		21
#define SMP_CMD_THREAD_PUT		22
#define SMP_CMD_DELETE_ALL_THREADS	23
#define SMP_CMD_UNWIND			24

/*
 * mailbox status messages
 */
#define MAILBOX_NULL			0
#define MAILBOX_OK			1
#define MAILBOX_ERROR			2
#define MAILBOX_DONE			3

#define MAILBOX_UNWIND_REMOTE		10

#define MAX_MAILBOX_PARAM		5

#define MAILBOX_TIMEOUT			0x100000

DECLARE_SPINLOCK(ipi_lock);

class cpu_mailbox_t 
{
private:
    dword_t m_command;
    volatile dword_t m_status;

public:
    tcb_t * tcb;
    dword_t param[MAX_MAILBOX_PARAM];
    l4_threadid_t tid;
    volatile dword_t pending_requests;

    /* handling of incoming requests */
    inline void set_request(dword_t from_cpu) 
    {
	asm volatile("lock;orl %0, %1\n"
		     :
		     : "r"(1 << from_cpu), "m"(pending_requests));
    }

    inline void clear_request(dword_t from_cpu)
    {
	asm volatile("lock;andl %0, %1\n"
		     :
		     : "r"(~(1 << from_cpu)), "m"(pending_requests));
    };

    inline void handle_pending_requests() {
	extern void smp_handle_requests();
	if (pending_requests)
	    smp_handle_requests();
    };

    /* command handling */
    dword_t send_command(dword_t cpu, dword_t command, 
			 dword_t value = MAILBOX_NULL);
    
    inline void broadcast_command(dword_t command,
				  dword_t value = MAILBOX_NULL)
    {
	m_status = value;
	m_command = command;
	broadcast_command_ipi();
    }

    inline dword_t wait_for_status(dword_t value)
    {
	extern int printf(const char *, ...);

	int count = MAILBOX_TIMEOUT;
	m_status = value;
	while (m_status == value) {
	    if (!count--)
	    {
		printf("cpu %d: mailbox::wait_for_status timeout\n", get_cpu_id());
		count = MAILBOX_TIMEOUT;
		asm("int3\n");
	    }

	    /* allow incoming requests from other cpus */
	    if (pending_requests)
	    {
		printf("cpu %d: cpu_mailbox::wait_for_status incoming request\n", get_cpu_id());
		handle_pending_requests();
	    }

	    __asm__ __volatile__ (".byte 0xf3; .byte 0x90\n"); 
	}
	return m_status;
    }

    inline void switch_to_idle(tcb_t * current, const dword_t val)
    {
	dword_t dummy;
	
	if (current->resources)
	    free_resources(current);

	__asm__ __volatile__(
	    "/* switch_to_idle */	\n"
	    "pushl	%%ebp		\n"
	    "pushl	$1f		\n"
	    "movl	%%esp, %2	\n"
	    "movl	%3, %%esp	\n"
	    "movl	%5, %4		\n"
	    "ret			\n"
	    "1:			\n"
	    "popl	%%ebp		\n"
	    "/* switch_to_idle */ \n"
	    : "=a" (dummy), "=d"(dummy)
	    : "m"(current->stack), "m"(get_idle_tcb()->stack), 
	    "m"(m_status), "gi"(val)
	    : "ebx", "ecx", "esi", "edi", "memory");
    }

    inline dword_t get_command() { return m_command; }

    inline void set_status(dword_t status) { m_status = status; }

    inline dword_t get_status() { return m_status; }
};

/* XCPU operations */

void smp_move_thread(tcb_t * tcb, dword_t cpu);
dword_t smp_start_short_ipc(tcb_t * to_tcb, tcb_t * current);
void smp_end_short_ipc(tcb_t * to_tcb);
dword_t smp_start_ipc(tcb_t * to_tcb, tcb_t * current);
dword_t smp_end_ipc(tcb_t * to_tcb, tcb_t * current);

dword_t smp_start_receive_ipc(tcb_t * from_tcb, tcb_t * current);

int smp_delete_all_threads(space_t * space);
int smp_ex_regs(tcb_t * tcb, dword_t * uip, dword_t * usp, 
		l4_threadid_t * pager, dword_t * flags);

void smp_flush_tlb();

/**********************************************************************
 *
 *                          thread handling
 *
 **********************************************************************/

INLINE void thread_adapt_pagetable(tcb_t * tcb, dword_t cpu)
{
    tcb->pagedir_cache = tcb->space->pagedir_phys(cpu);
}

#endif /* __X86_SMP_H__ */
