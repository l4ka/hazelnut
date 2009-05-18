/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/smp.c
 * Description:   SMP handling code.
 *                
 * @LICENSE@
 *                
 * $Id: smp.c,v 1.19 2001/12/04 21:22:07 uhlig Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <x86/apic.h>
#include <x86/init.h>

#if defined (CONFIG_SMP)

#if defined(CONFIG_DEBUG_TRACE_IPI)
DEFINE_SPINLOCK(ipi_printf_lock);
#define IPI_PRINTF(fmt, args...) do {printf("cpu %d: ", get_cpu_id()); printf(fmt, ## args ); } while(0)
#else
#define IPI_PRINTF(x...) 
#endif

#if 0
#define XIPC_PRINTF IPI_PRINTF
#else
#define XIPC_PRINTF(x...)
#endif

/* 
 * one bit for each processor which is online
 */
#if (CONFIG_SMP_MAX_CPU > 32)
# error review processor_online_map
#endif
volatile dword_t processor_online_map = 0;
volatile dword_t processor_map = 0;
seg_desc_t smp_boot_gdt[3]; /* this GDT has 3 entries: NULL, KCS, KDS */

void init_smp() L4_SECT_INIT;
void send_startup_ipi(int cpu) L4_SECT_INIT;

/* smp startup code */
extern "C" void _start_smp();
extern "C" void _end_smp();

/* mailboxes for inter-cpu communication */
cpu_mailbox_t cpu_mailbox[CONFIG_SMP_MAX_CPU];

/* global lock for all ipi's */
DEFINE_SPINLOCK(ipi_lock);



/**********************************************************************
 * online status of processors
 */

/* the apic id of the boot processor */
dword_t boot_cpu = 0;

void set_cpu_online(dword_t cpu)
{
    processor_online_map |= (1 << cpu);
}

int is_cpu_online(dword_t cpu)
{
    return processor_online_map & (1 << cpu);
}


/**********************************************************************
 * helpers
 */

INLINE cpu_mailbox_t * get_mailbox()
{
    return &cpu_mailbox[get_cpu_id()];
}


static void udelay(int usec)
{
    for (int i=0; i<(100000*usec); i++);
}

void send_startup_ipi(int cpu)
{
    dword_t val;

    //out_rtc(0xf, 0xa);

    /* at this stage we still have our 1:1 mapping at 0 */
    *((volatile unsigned short *) 0x469) = SMP_STARTUP_PAGE >> 4;
    *((volatile unsigned short *) 0x467) = SMP_STARTUP_PAGE & 0xf;


    /* assert init */
#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("APIC INIT assert...\n");
#endif
    val = get_local_apic(X86_APIC_INTR_CMD2);
    val &= 0x00ffffff;
    set_local_apic(X86_APIC_INTR_CMD2, val | cpu << (56 - 32));

    val = get_local_apic(X86_APIC_INTR_CMD1);
    val &= ~0xCDFFF;
    val |= APIC_TRIGGER_LEVEL | APIC_ASSERT | APIC_DEL_INIT;
    set_local_apic(X86_APIC_INTR_CMD1, val);

    udelay(200);

    /* deassert init */
#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("APIC INIT deassert...\n");
#endif
    val = get_local_apic(X86_APIC_INTR_CMD2);
    val &= 0x00ffffff;
    set_local_apic(X86_APIC_INTR_CMD2, val | cpu << (56 - 32));

    val = get_local_apic(X86_APIC_INTR_CMD1);
    val &= ~0xCDFFF;
    val |= APIC_TRIGGER_LEVEL | APIC_DEASSERT | APIC_DEL_INIT;
    set_local_apic(X86_APIC_INTR_CMD1, val);

    /* send startup ipi */
#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("APIC send startup ipi...\n");
#endif

    // send startup ipi 2 times (see: intel mp boot)
    for (int i = 0; i < 2; i++) {
	set_local_apic(X86_APIC_ERR_STATUS, 0);
#if defined(CONFIG_DEBUG_TRACE_SMP)
	printf("APIC: reset status\n");
#endif

	val = get_local_apic(X86_APIC_INTR_CMD2);
	val &= 0x00ffffff;
	set_local_apic(X86_APIC_INTR_CMD2, val | cpu << (56 - 32));

	val = get_local_apic(X86_APIC_INTR_CMD1);
	val &= ~0xCDFFF;
	val |= 0x600 | (SMP_STARTUP_PAGE >> PAGE_BITS);
	set_local_apic(X86_APIC_INTR_CMD1, val);

#if defined(CONFIG_DEBUG_TRACE_SMP)
	printf("APIC wait for completion\n");
#endif
	// wait till message is delivered
	do {
	    udelay(100);
	    val = get_local_apic(X86_APIC_INTR_CMD1) & 0x1000;
	} while(val);

	udelay(200);
	
	// get delivery error
	val = get_local_apic(X86_APIC_ERR_STATUS) & 0xef;
#if defined(CONFIG_DEBUG_TRACE_SMP)
	printf("\ndelivery status: %x\n", val);
#endif
    }

}

void thread_adapt_queue_state(tcb_t * tcb, dword_t queue_state)
{
    /* we always enqueue into ready and present queue */
    thread_enqueue_present(tcb);
    thread_enqueue_ready(tcb);

    /* enqueue into wakeup queue on demand */
    if (queue_state & TS_QUEUE_WAKEUP)
	thread_enqueue_wakeup(tcb);
}


dword_t cpu_mailbox_t::send_command(dword_t cpu, dword_t command, dword_t value)
{
    int count = MAILBOX_TIMEOUT;
    m_status = value;
    m_command = command;

    /* signal the request */
    cpu_mailbox[cpu].set_request(get_cpu_id());

    /* send command IPI to the cpu */
    send_command_ipi(cpu);
    spin1(71);

    /* and now wait for the cpu to handle the reuqest */
    while (m_status == value) {
	spin1(70);

	if (count-- == 0)
	{
	    IPI_PRINTF("mailbox::send_command timeout - resend IPI to %d\n", cpu);
	    enter_kdebug("timeout");
	    send_command_ipi(cpu);
	    count = MAILBOX_TIMEOUT;
	}

	/* allow incoming requests from other cpus */
	if (pending_requests)
	{
	    IPI_PRINTF("mailbox::send_command incoming request\n");
	    handle_pending_requests();
	    continue;
	}

	__asm__ __volatile__ (".byte 0xf3; .byte 0x90\n");
    }
    return m_status;
}


static void do_xcpu_ipc_short(cpu_mailbox_t * mailbox)
{
    tcb_t * to_tcb = mailbox->tcb;
    tcb_t * from_tcb = (tcb_t*)mailbox->param[0];
    XIPC_PRINTF("smp ipi: thread block to_tcb: %p, from_tcb: %p\n", to_tcb, from_tcb);
    
    if (to_tcb->cpu != get_cpu_id())
    {
	XIPC_PRINTF("%s to_tcb->cpu != current_cpu\n", __FUNCTION__);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }
	
    spin_lock(&to_tcb->tcb_spinlock);

    /* check if destination still receives from me */
    if (!( ( to_tcb->thread_state == TS_WAITING ) && 
	   ( to_tcb->partner == L4_NIL_ID || 
	     to_tcb->partner == from_tcb->myself ) ))
    {
	XIPC_PRINTF("destination not waiting (state: %x, partner: %x)\n", 
		    to_tcb->thread_state, to_tcb->partner);
	spin_unlock(&to_tcb->tcb_spinlock);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }
    
    spin_unlock(&to_tcb->tcb_spinlock);


    XIPC_PRINTF("waiting for handshake on mailbox %x\n", &mailbox);
    /* now do the short ipc hand-shaking */
    mailbox->wait_for_status(MAILBOX_OK);

    /* here the transfer is finished - let the baby run... */
    to_tcb->thread_state = TS_LOCKED_RUNNING;
    thread_enqueue_ready(to_tcb);
}

void do_xcpu_ipc_start(cpu_mailbox_t * mailbox)
{
    tcb_t * to_tcb = mailbox->tcb;
    tcb_t * from_tcb = (tcb_t*)mailbox->param[0];
    IPI_PRINTF("ipi: %s from: %p, to: %p\n", __FUNCTION__, from_tcb, to_tcb);

    if (to_tcb->cpu != get_cpu_id())
    {
	IPI_PRINTF("%s to_tcb->cpu != current_cpu\n", __FUNCTION__);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }
	
    spin_lock(&to_tcb->tcb_spinlock);

    /* check if destination still receives from me */
    if (!( ( to_tcb->thread_state == TS_WAITING ) && 
	   ( to_tcb->partner == L4_NIL_ID || 
	     to_tcb->partner == from_tcb->myself ) ))
    {
	XIPC_PRINTF("destination not waiting (state: %x, partner: %x)\n", 
		    to_tcb->thread_state, to_tcb->partner);
	spin_unlock(&to_tcb->tcb_spinlock);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }
    
    /* set into receiving state */
    to_tcb->thread_state = TS_LOCKED_WAITING;
    spin_unlock(&to_tcb->tcb_spinlock);

    /* release partner CPU */
    mailbox->set_status(MAILBOX_OK);
}    

void do_xcpu_ipc_end(cpu_mailbox_t * mailbox)
{
    tcb_t * to_tcb = mailbox->tcb;
    tcb_t * from_tcb = (tcb_t*)mailbox->param[0];
    XIPC_PRINTF("ipi: %s from: %p, to: %p\n", __FUNCTION__, from_tcb, to_tcb);

    if (to_tcb->cpu != get_cpu_id())
    {
	IPI_PRINTF("%s: to_tcb->cpu != current_cpu\n", __FUNCTION__);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }
	
    spin_lock(&to_tcb->tcb_spinlock);
    
    /* check that everything is still all right! */
    if (to_tcb->thread_state != TS_LOCKED_WAITING ||
	to_tcb->partner != from_tcb->myself)
    {
	IPI_PRINTF("%s something whicked happened meanwhile (ex_regs?) (to->state=%x, to->partner=%x\n", __FUNCTION__, to_tcb->thread_state, to_tcb->partner);
	spin_unlock(&to_tcb->tcb_spinlock);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }

    /* ok, everything is fine, we have to explicitly enqueue the thread,
     * since we do not directly switch */
    to_tcb->thread_state = TS_RUNNING;
    thread_enqueue_ready(to_tcb);

    spin_unlock(&to_tcb->tcb_spinlock);
    mailbox->set_status(MAILBOX_OK);
}

void do_xcpu_ipc_receive(cpu_mailbox_t * mailbox)
{
    tcb_t * from_tcb = mailbox->tcb;
    tcb_t * to_tcb = (tcb_t*)mailbox->param[0];
    XIPC_PRINTF("ipi: %s from: %p, to: %p\n", __FUNCTION__, from_tcb, to_tcb);
    if (from_tcb->cpu != get_cpu_id())
    {
	IPI_PRINTF("from_tcb->cpu != current_cpu\n");
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }

    spin_lock(&to_tcb->tcb_spinlock);

    if (!IS_POLLING(from_tcb) || 
	(from_tcb->partner != to_tcb->myself))
    {
	IPI_PRINTF("not receiving from partner anymore\n");
	mailbox->set_status(MAILBOX_ERROR);
	spin_unlock(&to_tcb->tcb_spinlock);
	return;
    }

    /* ok - we are partners and want to do ipc with each other */
    thread_dequeue_send(to_tcb, from_tcb);
    thread_dequeue_wakeup(from_tcb);
    from_tcb->thread_state = TS_XCPU_LOCKED_RUNNING;
    thread_enqueue_ready(from_tcb);
    spin_unlock(&to_tcb->tcb_spinlock);

    /* release other cpu */
    mailbox->set_status(MAILBOX_OK);
}

dword_t smp_start_receive_ipc(tcb_t * from_tcb, tcb_t * current)
{
    cpu_mailbox_t * mailbox = get_mailbox();
    mailbox->tcb = from_tcb;
    mailbox->param[0] = (dword_t)current;
    dword_t status = mailbox->send_command(from_tcb->cpu, SMP_CMD_IPC_RECEIVE);

    if (status == MAILBOX_ERROR) {
	//enter_kdebug("smp_start_receive_ipc failed");
	return 0;
    }

    switch_to_idle(current);
    
    return 1;
}

void do_xcpu_thread_get(cpu_mailbox_t * mailbox)
{
    tcb_t* tcb = mailbox->tcb;
    IPI_PRINTF("%s %p, state=%x\n", __FUNCTION__, tcb, tcb->thread_state);

    /* not on our cpu! */
    if (tcb->cpu != get_cpu_id())
    {
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }

    mailbox->param[0] = tcb->queue_state;
    thread_dequeue_present(tcb);
    thread_dequeue_ready(tcb);
    thread_dequeue_wakeup(tcb);

    /* are we ourself the migrated thread ??? */
    if (tcb == get_current_tcb())
    {
	apic_ack_irq();
	mailbox->switch_to_idle(tcb /* which is current */, MAILBOX_OK);
	return;
    };
	
    mailbox->set_status(MAILBOX_OK);
}

void do_xcpu_thread_put(cpu_mailbox_t * mailbox)
{
    tcb_t * tcb = mailbox->tcb;
    IPI_PRINTF("%s %p, state=%x\n", __FUNCTION__, tcb, tcb->thread_state);

    tcb->cpu = get_cpu_id();
    thread_adapt_queue_state(tcb, mailbox->param[0]);
    mailbox->set_status(MAILBOX_OK);
	
    /* adjust the page directory for this tcb */
    IPI_PRINTF("pgdir for tcb=%x: %x\n", tcb, tcb->pagedir_cache);
    thread_adapt_pagetable(tcb, get_cpu_id());
    IPI_PRINTF("pgdir for tcb=%x: %x\n", tcb, tcb->pagedir_cache);
    //while(1);
}

#warning xcpu_unwind_mailbox should be explicitly initialized
static cpu_mailbox_t * xcpu_unwind_mailbox L4_SECT_CPULOCAL;
void do_xcpu_unwind(cpu_mailbox_t * mailbox)
{
    tcb_t * tcb = mailbox->tcb;
    if (tcb->cpu != get_cpu_id())
    {
	IPI_PRINTF("xcpu unwind on wrong cpu (%x)\n", tcb);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }

    IPI_PRINTF("%s %x (partner=%x, current=%x)\n",
	       __FUNCTION__, tcb, tcb->partner, get_current_tcb());

    xcpu_unwind_mailbox = mailbox;
    
    unwind_ipc(tcb);

    xcpu_unwind_mailbox = NULL;

    /* now that guy is ready to run - enqueue into run queue */
    tcb->thread_state = TS_RUNNING;
    thread_enqueue_ready(tcb);

    /* may be ignored, if we have an unwind */
    mailbox->set_status(MAILBOX_OK);
}

extern void delete_all_threads(space_t *);
void do_xcpu_delete_all_threads(cpu_mailbox_t * mailbox)
{
    space_t* space = (space_t*)mailbox->param[0];

    xcpu_unwind_mailbox = mailbox;

    delete_all_threads(space);

    xcpu_unwind_mailbox = NULL;

    /* ensure that our pagetable does not disappear */
    if (get_current_pagetable() == space->pagedir_phys(get_cpu_id()))
    {
	/* after deleting all threads we must switch the pagetable */
	set_current_pagetable(get_kernel_space()->pagedir_phys(get_cpu_id()));

	if (get_current_tcb() != get_idle_tcb())
	{
	    mailbox->switch_to_idle(get_current_tcb(), MAILBOX_OK);
	    /* not re-activated */
	    panic("mailbox->switch_to_idle reactivated");
	}

    }
    mailbox->set_status(MAILBOX_OK);
}

void do_xcpu_thread_ex_regs(cpu_mailbox_t * mailbox)
{
    tcb_t * tcb = mailbox->tcb;
    if (tcb->cpu != get_cpu_id())
    {
	IPI_PRINTF("xcpu ex regs on wrong cpu (%x)\n", tcb);
	mailbox->set_status(MAILBOX_ERROR);
	return;
    }

    IPI_PRINTF("%s %x, state=%x\n", __FUNCTION__, tcb, tcb->thread_state);
    
    xcpu_unwind_mailbox = mailbox;
    IPI_PRINTF("before unwind_ipc (%x)\n", get_cpu_id(), tcb);
	
    /* unwind ipc expects to have the spinlock! */
    spin_lock(&tcb->tcb_spinlock);
    unwind_ipc(tcb);
    IPI_PRINTF("after unwind_ipc (%x)\n", get_cpu_id(), tcb);

    if (mailbox->param[1] == ~0U)
	mailbox->param[1] = get_user_sp(tcb);
    else
	mailbox->param[1] = set_user_sp(tcb, mailbox->param[0]);

    if (mailbox->param[0] == ~0U)
	mailbox->param[0] = get_user_ip(tcb);
    else
	mailbox->param[0] = set_user_ip(tcb, mailbox->param[0]);

    if (mailbox->tid == L4_INVALID_ID)
	mailbox->tid = tcb->pager;
    else
    {
	l4_threadid_t oldpager;
	oldpager = tcb->pager;
	tcb->pager = mailbox->tid;
	mailbox->tid = oldpager;
    }

    /* 
     * we never allocate xcpu-threads 
     * therefore we can always set them running
     */
    thread_dequeue_wakeup(tcb);
    tcb->thread_state = TS_RUNNING;
    thread_enqueue_ready(tcb);
    spin_unlock(&tcb->tcb_spinlock);
    
    xcpu_unwind_mailbox = NULL;
    mailbox->set_status(MAILBOX_OK);
}

void do_xcpu_flush_tlb(cpu_mailbox_t * mailbox)
{
    flush_tlb();
    mailbox->set_status(MAILBOX_OK);
}

/* central inter-cpu command handler */
void smp_handle_request(dword_t from_cpu)
{
    dword_t current_cpu = get_cpu_id();
    cpu_mailbox_t * mailbox = &cpu_mailbox[from_cpu];
    if (mailbox->get_command() > SMP_CMD_IPC_RECEIVE)
	IPI_PRINTF("smp-ipi command %d from %d\n", mailbox->get_command(), from_cpu);
    spin1(72);

    if (current_cpu == from_cpu) {
	enter_kdebug("self command ipi???");
	return;
    }

    //IPI_PRINTF("smp-ipi command %x from %d\n", mailbox->get_command(), from_cpu);

    switch(mailbox->get_command()) {
    case SMP_CMD_IPC_SHORT: 
	do_xcpu_ipc_short(mailbox);
	break;

    case SMP_CMD_IPC_START:
	do_xcpu_ipc_start(mailbox);
	break;

    case SMP_CMD_IPC_END:
	do_xcpu_ipc_end(mailbox);
	break;

    case SMP_CMD_IPC_RECEIVE:
	do_xcpu_ipc_receive(mailbox);
	break;

    case SMP_CMD_THREAD_GET:
	do_xcpu_thread_get(mailbox);
	break;

    case SMP_CMD_THREAD_PUT:
	do_xcpu_thread_put(mailbox);
	break;
       
    case SMP_CMD_THREAD_EX_REGS:
	do_xcpu_thread_ex_regs(mailbox);
	break;

    case SMP_CMD_UNWIND:
	do_xcpu_unwind(mailbox);
	break;

    case SMP_CMD_DELETE_ALL_THREADS:
	do_xcpu_delete_all_threads(mailbox);
	break;
	
    case SMP_CMD_FLUSH_TLB:
	do_xcpu_flush_tlb(mailbox);
	break;

    default:
	printf("cpu%d: unhandled command from cpu %d: cmd: %x, tcb: %p\n",
	       get_cpu_id(), from_cpu, mailbox->get_command(), mailbox->tcb);
    }
}

void smp_handle_requests()
{
    cpu_mailbox_t * mailbox = get_mailbox();

    while(mailbox->pending_requests)
    {
	for (dword_t i = 0; i < CONFIG_SMP_MAX_CPU; i++)
	    if (mailbox->pending_requests & (1 << i))
	    {
		mailbox->clear_request(i);
		smp_handle_request(i);
	    }
    }
}

void smp_handler_command_ipi()
{
    smp_handle_requests();
    apic_ack_irq();
}


void smp_move_thread(tcb_t * tcb, dword_t cpu)
{
    cpu_mailbox_t * mailbox = get_mailbox();
    //mailbox->command = SMP_CMD_THREAD_MOVE;
    mailbox->tcb = tcb;
    //mailbox->status = MAILBOX_NULL;
    mailbox->param[0] = tcb->queue_state;

    //IPI_PRINTF("smp move thread %p from cpu %d to cpu %d\n", tcb, tcb->cpu, cpu);

    /* do not move thread if already on cpu */
    if (tcb->cpu == cpu)
	return;

    /* do not migrate to inactive cpus */
    if (!is_cpu_online(cpu))
	return;

 retry_migration:

    if (tcb->cpu == get_cpu_id())
    {
	/* we have the thread - so, we can give it away */
	thread_dequeue_present(tcb);
	thread_dequeue_ready(tcb);
	thread_dequeue_wakeup(tcb);
	
	IPI_PRINTF("before thread put (current: %x, pdir=%x, tcb: %x)\n", 
		   get_current_tcb(), get_current_pagetable(), tcb);

	mailbox->send_command(cpu, SMP_CMD_THREAD_PUT);

	IPI_PRINTF("thread_put done (current: %x, pdir=%x, cpu: %d/%d)\n", 
		   get_current_tcb(), get_current_pagetable(), 
		   get_cpu_id(), get_apic_cpu_id());
    }
    else
    {
	/* we don't have the thread - ask the cpu */
	dword_t status;
	status = mailbox->send_command(tcb->cpu, SMP_CMD_THREAD_GET);
	
	/* thread may have moved meanwhile */
	if (status != MAILBOX_OK)
	    goto retry_migration;
	
	if (cpu == get_cpu_id())
	{
	    /* the thread comes to us */
	    tcb->cpu = cpu;
	    thread_adapt_queue_state(tcb, mailbox->param[1]);
	    /* adjust the page directory for this tcb */
	    //printf("pgdir: %x\n", tcb->page_dir);
	    thread_adapt_pagetable(tcb, get_cpu_id());
	    //printf("pgdir: %x\n", tcb->page_dir);
	}
	else
	{
	    status = mailbox->send_command(cpu, SMP_CMD_THREAD_PUT);
	    
	    if (status != MAILBOX_OK)
	    {
		enter_kdebug("3-cpu thread migration failed");
		return;
	    }
	}
    } 
}

/* pre-condition:
 *   tcb is waiting or receiving on another cpu
 */
dword_t smp_start_short_ipc(tcb_t * tcb, tcb_t * current)
{
    XIPC_PRINTF("sending start_short_ipc ipi (current=%p)\n", current);

    cpu_mailbox_t * mailbox = get_mailbox();    
    mailbox->tcb = tcb;
    mailbox->param[0] = (dword_t)current;

    dword_t status = mailbox->send_command(tcb->cpu, SMP_CMD_IPC_SHORT);

    /* 
     * ok - delivery can start now
     * partner cpu spins in mailbox loop and waits for message.
     */
    if (status == MAILBOX_OK)
	return 1;

    IPI_PRINTF("%d smp_start_short_ipc failed (%x (%d, %x) -> %x (%d, %x))\n", 
	       get_cpu_id(), 
	       current, current->cpu, current->thread_state,
	       tcb, tcb->cpu, tcb->thread_state);

    /* ipc failed - check whether we have pending requests */
    IPI_PRINTF("pending requests = %x\n", mailbox->pending_requests);
    mailbox->handle_pending_requests();

    return 0;
}

void smp_end_short_ipc(tcb_t * to_tcb)
{
    cpu_mailbox_t * mailbox;
    XIPC_PRINTF("smp_end_short_ipc (tcb=%x)\n", to_tcb);

    if (to_tcb->thread_state == TS_XCPU_LOCKED_RUNNING)
	mailbox = &cpu_mailbox[to_tcb->cpu];
    else
	mailbox = get_mailbox();

    XIPC_PRINTF("notifying mailbox %x\n", mailbox);

    /* ok notification for partner cpu that transfer is complete */
    mailbox->set_status(MAILBOX_DONE);
}

dword_t smp_start_ipc(tcb_t * to_tcb, tcb_t * current)
{
    XIPC_PRINTF("sending start_ipc ipi (current=%p)\n", current);

    cpu_mailbox_t * mailbox = get_mailbox();    
    mailbox->tcb = to_tcb;
    mailbox->param[0] = (dword_t)current;

    dword_t status = mailbox->send_command(to_tcb->cpu, SMP_CMD_IPC_START);

    if (status == MAILBOX_OK)
	return 1;

    IPI_PRINTF("%d smp_start_ipc failed (%x (%d, %x) -> %x (%d, %x))\n", 
	       get_cpu_id(), 
	       current, current->cpu, current->thread_state,
	       to_tcb, to_tcb->cpu, to_tcb->thread_state);

    return 0;
}

dword_t smp_end_ipc(tcb_t * to_tcb, tcb_t * current)
{
    XIPC_PRINTF("sending end_ipc ipi (to_tcb=%p)\n", to_tcb);

    cpu_mailbox_t * mailbox = get_mailbox();    
    mailbox->tcb = to_tcb;
    mailbox->param[0] = (dword_t)current;

    dword_t status = mailbox->send_command(to_tcb->cpu, SMP_CMD_IPC_END);

    if (status == MAILBOX_OK)
	return 1;

    IPI_PRINTF("smp_end_ipc failed (%x (%d, %x)\n",
	       get_cpu_id(), 
	       to_tcb, to_tcb->cpu, to_tcb->thread_state);

    return 0;
}

int smp_delete_all_threads(space_t * space)
{
    //IPI_PRINTF("%s (%x)\n", __FUNCTION__, victim);
    cpu_mailbox_t * mailbox = get_mailbox();
    for (dword_t cpu = 0; cpu < CONFIG_SMP_MAX_CPU; cpu++)
    {
	if (cpu == get_cpu_id())
	    continue;
	if (!is_cpu_online(cpu))
	    continue;

	mailbox->param[0] = (dword_t)space;
	dword_t status = mailbox->send_command(cpu, SMP_CMD_DELETE_ALL_THREADS);
	switch(status)
	{
	case MAILBOX_OK:
	    return 1;
	case MAILBOX_UNWIND_REMOTE:
	    /* we have to perform a remote unwind */
	    IPI_PRINTF("%s: remote unwind %x\n", mailbox->tcb);
	    unwind_ipc(mailbox->tcb);
	    break;
	case MAILBOX_ERROR:
	    enter_kdebug("smp_delete_task: error deleting task");
	    break;
	default:
	    enter_kdebug("smp_delete_task: unexpected return value");
	    break;
	}
    }
    return 0;
}

int smp_unwind_ipc(tcb_t * tcb)
{
    IPI_PRINTF("smp_unwind_ipc(%x), partner=%x, current=%x, xcpu_mb=%x\n", 
	   tcb, tcb->partner, get_current_tcb(), xcpu_unwind_mailbox);
    /* check if we are already performing an unwind ipc operation */
    if (xcpu_unwind_mailbox)
    {
	xcpu_unwind_mailbox->tcb = tcb;
	xcpu_unwind_mailbox->set_status(MAILBOX_UNWIND_REMOTE);
	return 1;
    }

    dword_t status;
    cpu_mailbox_t * mailbox = get_mailbox();
    mailbox->tcb = tcb;
    
    status = mailbox->send_command(tcb->cpu, SMP_CMD_UNWIND);
    
    switch(status)
    {
    case MAILBOX_UNWIND_REMOTE:
	/* we have to perform a remote unwind */
	unwind_ipc(mailbox->tcb);
	break;
    case MAILBOX_OK:
	/* unwind done */
	break;
    case MAILBOX_ERROR:
	/* thread may have moved */
	return 0;
    default:
	enter_kdebug("smp_unwind: unexpected return value");
	return 0;
    };
    return 1;
}

int smp_ex_regs(tcb_t * tcb, dword_t * uip, dword_t * usp, 
		l4_threadid_t * pager, dword_t * flags)
{
    cpu_mailbox_t * mailbox = get_mailbox();
    IPI_PRINTF("xcpu ex_regs tcb=%x ip=%x sp=%x pager=%x\n", 
	       tcb, *uip, *usp, *pager);

    mailbox->tcb = tcb;
    mailbox->param[0] = *uip;
    mailbox->param[1] = *usp;
    mailbox->tid = *pager;

    switch(mailbox->send_command(tcb->cpu, SMP_CMD_THREAD_EX_REGS))
    {
    case MAILBOX_ERROR:
	IPI_PRINTF("xcpu ex_regs failed\n");
	return 0;
    case MAILBOX_UNWIND_REMOTE:
	enter_kdebug("smp_ex_regs unwind remote");
	*uip = mailbox->param[0];
	*usp = mailbox->param[1];
	*flags = mailbox->param[3];
	*pager = mailbox->tid;
	unwind_ipc(mailbox->tcb);
	return 1;
    case MAILBOX_OK:
	IPI_PRINTF("xcpu ex_regs done\n");
	*uip = mailbox->param[0];
	*usp = mailbox->param[1];
	*flags = mailbox->param[3];
	*pager = mailbox->tid;
	return 1;
    default:
	printf("smp_ex_regs: unknown response\n");
    }
    return 0;
}

void smp_flush_tlb()
{
#warning inefficient implementation of tlb shootdown
    cpu_mailbox_t * mailbox = get_mailbox();
    for (dword_t cpu = 0; cpu < CONFIG_SMP_MAX_CPU; cpu++)
    {
	if (cpu == get_cpu_id())
	    continue;
	if (!is_cpu_online(cpu))
	    continue;

	dword_t status = mailbox->send_command(cpu, SMP_CMD_FLUSH_TLB);
	if (status != MAILBOX_OK)
	    enter_kdebug("smp_flush_tlb");
    }
}

  
void smp_startup_processors()
{

#define gdt_idx(x) ((x) >> 3)
    /* the kernel code - kernel_space_exec */
    smp_boot_gdt[gdt_idx(X86_KCS)].set_seg(0, ~0, 0, GDT_DESC_TYPE_CODE);
    /* kernel data - linear_kernel_space */
    smp_boot_gdt[gdt_idx(X86_KDS)].set_seg(0, ~0, 0, GDT_DESC_TYPE_DATA);

#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("smp_start_processors\n");
#endif
    memcpy((dword_t*)SMP_STARTUP_PAGE, (dword_t*)_start_smp, (dword_t)(_end_smp) - (dword_t)(_start_smp));
    //enter_kdebug("after smp-copy");
    wbinvd();

    for (dword_t cpu = 0; cpu < CONFIG_SMP_MAX_CPU; cpu++) 
    {
	// the boot cpu is not necessarily cpu 0
	if (cpu == get_boot_cpu())
	    continue;

	// start only processor which are in the processor map
	if (!(processor_map & (1 << cpu)))
	    continue;

	send_startup_ipi(cpu);

	// wait for cpu to be online - aprox. 100 * 100us = 
	for (int i=0; i<100; i++) {
	    udelay(100);
	    if (is_cpu_online(cpu)) break;
	}
	
	if (!is_cpu_online(cpu)) {
	    printf("CPU %d failed to start\n", cpu);
	}
	else
	{
	    printf("CPU %d is online\n", cpu);
	}
    }
    //enter_kdebug("startup done");
}
#if 1
void system_sleep()
{
#if 1
    //apic_ack_irq();
    __asm__ __volatile__("sti	\n"
			 "hlt	\n"
			 "cli	\n"
			 :);
#else
    enable_interrupts();
    while(!get_mailbox()->pending_requests);
    disable_interrupts();
    get_mailbox()->handle_pending_requests();
#endif
}
#endif
#endif /* CONFIG_SMP */
