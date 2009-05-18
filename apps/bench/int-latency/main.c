/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     bench/int-latency/main.c
 * Description:   Interrupt latancy benchmark
 *                
 * @LICENSE@
 *                
 * $Id: main.c,v 1.2 2002/05/07 19:11:10 skoglund Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4/helpers.h>
#include <l4/sigma0.h>
#include <l4io.h>
#include <multiboot.h>

#include "../../sigma0/kip.h"
#include "apic.h"


#define MASTER_PRIO	201
#define INT_PRIO	200
#define WORKER_PRIO	199

#if 1
extern "C" void memset (char * p, char c, int size)
{
    while (size--)
	*(p++) = c;
}

extern "C" char * strchr (char * s, int c)
{
    while (*s != c)
    {
	if (*s == 0)
	    return NULL;
	s++;
    }
    return s;
}

extern "C" char * strstr (char * big, char * little)
{
    char *cb, *cl;

    if (*strstr == '\0')
	return big;

    for (;;)
    {
	big = strchr (big, *little);
	if (big == NULL)
	    return NULL;

	for (cb = big, cl = little;
	     *cl != 0 && *cb != 0 && *cl == *cb;
	     cb++, cl++) ;

	if (*cl == 0)
	    return big;
	else if (*cb == 0)
	    return NULL;
	big++;
    }
}

extern "C" int atoi (char * nptr)
{
    int neg = 0, num = 0;

    if (*nptr == '-')
	neg = 1;

    while (*nptr >= '0' && *nptr <= '9')
    {
	num *= 10;
	num += *nptr - '0';
	nptr++;
    }

    return neg ? -num : num;
}

#endif

inline dword_t rdtsc (void)
{
    dword_t ret;
    asm volatile ("rdtsc" :"=a" (ret) : :"edx");
    return ret;
}


dword_t int_num = 9;
dword_t period = 50000000;

l4_threadid_t pager_tid;
l4_threadid_t master_tid;
l4_threadid_t int_tid;
l4_threadid_t worker_tid;
l4_threadid_t starter_tid;

dword_t cpu_freq, bus_freq;

dword_t starter_stack[1024];
dword_t int_stack[1024];
dword_t worker_stack[1024];

void worker_thread (void)
{
    l4_msgdope_t dope;

    /* Notify interrupt handler thread. */
    l4_ipc_send (int_tid, 0, 0, 0, 0, L4_IPC_NEVER, &dope);

    for (;;)
	;
}

void int_thread (void)
{
    dword_t dummy;
    dword_t rdtsc0, rdtsc1, rdtsc2, time0, time1;
    l4_msgdope_t dope;
    bool go = true;

    /* Setup local APIC. */
    extern dword_t _end;
    dword_t apic = ((dword_t) &_end + (1 << 12)) & ~((1 << 12) - 1);
    apic = (apic + (1 << 22)) & ~((1 << 22) - 1);

    l4_sigma0_getpage_rcvpos (L4_NIL_ID, l4_fpage (0xfee00000, 22, 1, 0),
			      l4_fpage ((dword_t) apic, 22, 1, 0));

    apic += 0xfee00000 & ((1 << 22) - 1);
    setup_local_apic (apic);

    /* Wait for worker thread to come up. */
    l4_ipc_receive (worker_tid, 0,
		    &dummy, &dummy, &dummy,
		    L4_IPC_NEVER, &dope);

    /* Associate with IRQ. */
    associate_interrupt (int_num);

    /* Unmask APIC IRQ. */
    set_local_apic (X86_APIC_LVT_TIMER,
		    (get_local_apic (X86_APIC_LVT_TIMER) & ~APIC_IRQ_MASK));

    set_local_apic (X86_APIC_TIMER_COUNT, period);

    while (go)
    {
	printf ("\n");
	printf ("     Bus cycles            CPU cycles\n");
	printf ("  hw-kern  kern-user   kern-ipc  ipc-user\n");

	apic_ack_irq ();
	l4_ipc_receive (L4_INTERRUPT (int_num), 0,
			&dummy, &dummy, &dummy,
			L4_IPC_NEVER, &dope);
	apic_ack_irq ();

	for (int j = 10; j--;)
	{
	    l4_ipc_receive (L4_INTERRUPT (int_num), 0,
			    &time0, &rdtsc0, &rdtsc1,
			    L4_IPC_NEVER, &dope);

	    rdtsc2 = rdtsc ();
	    time1 = get_local_apic (X86_APIC_TIMER_CURRENT);

	    apic_ack_irq ();

	    printf ("%8d %8d    %8d %8d\n",
		    period-time0, time0-time1,
		    rdtsc1-rdtsc0, rdtsc2-rdtsc1);
	}
	enter_kdebug ("done");

	for (;;)
	{
	    printf ("What now?\n"
		    "  g - Continue\n"
		    "  q - Quit/New measurement\n\n");
	    char c = kd_inchar ();
	    if (c == 'g') { break; }
	    if (c == 'q') { go = false; break; }
	}
    }
    
    /* Tell master that we're finished. */
    l4_ipc_send (master_tid, 0, 0, 0, 0, L4_IPC_NEVER, &dope);

    for (;;)
	l4_sleep (0);
    /* NOTREACHED */
}

void startup_worker (void)
{
    l4_threadid_t foo = pager_tid;
    dword_t dummy;

    /* Start worker thread. */
    l4_thread_ex_regs (int_tid,
		       (dword_t) int_thread,
		       (dword_t) int_stack + sizeof (int_stack),
		       &foo, &foo, &dummy, &dummy, &dummy);
    
    l4_set_prio (worker_tid, WORKER_PRIO);

    /* Start doing the work. */
    int_thread ();
}



#define PAGE_BITS 	12
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(~(PAGE_SIZE-1))

dword_t pager_stack[512];

void pager (void)
{
    l4_threadid_t t;
    l4_msgdope_t dope;
    dword_t dw0, dw1, dw2;
    dword_t map = 2;
    
    for (;;)
    {
	l4_ipc_wait (&t, 0, &dw0, &dw1, &dw2, L4_IPC_NEVER, &dope);
	
	for (;;)
	{
	    dw0 &= PAGE_MASK;
	    dw1 = dw0 | 0x32;
	    l4_ipc_reply_and_wait (t, (void *) map,
				   dw0, dw1, dw2,
				   &t, 0,
				   &dw0, &dw1, &dw2,
				   L4_IPC_NEVER, &dope);

	    if (L4_IPC_ERROR (dope))
	    {
		printf ("%s: error reply_and_wait (%x)\n",
			__FUNCTION__, dope.raw);
		break;
	    }
	}
    }
}



int main (dword_t mbvalid, multiboot_info_t * mbi)
{
    l4_threadid_t foo;
    l4_msgdope_t dope;
    dword_t dummy;
    bool is_small, is_inter_as;

    /* Parse commandline. */
    if (mbvalid == MULTIBOOT_VALID)
    {
	if (1) // (mbi->flags & MULTIBOOT_MODS)
	{
	    char *arg, *cmdline = NULL;
	    for (dword_t i = 0; i < mbi->mods_count; i++)
	    {
		cmdline = strstr (mbi->mods_addr[i].string, "/int-latency ");
		if (cmdline)
		    break;
	    }

	    if (cmdline)
	    {
		if ((arg = strstr (cmdline, "interrupt=")) != NULL)
		    int_num = atoi (arg + sizeof ("interrupt=") - 1);
		else if ((arg = strstr (cmdline, "period=")) != NULL)
		    period = atoi (arg + sizeof ("period=") - 1);
	    }
	}
    }

    /* Get kernel info page. */
    extern dword_t _end;
    kernel_info_page_t * kip = (kernel_info_page_t *)
	(((dword_t) &_end + (1 << 12)) & ~((1 << 12) - 1));

    l4_sigma0_getkerninfo_rcvpos (L4_NIL_ID,
				  l4_fpage ((dword_t) kip, 12, 0, 0));

    cpu_freq = kip->processor_frequency;
    bus_freq = kip->bus_frequency;
    printf ("CPU freq: %d,  Bus freq: %d\n", cpu_freq, bus_freq);

    printf ("Int latency using the following code sequence for IPC:\n");
    printf ("->->->->->-\n");
    printf ("%s", IPC_SYSENTER);
    printf ("\r-<-<-<-<-<-\n");

    /* Create pager. */
    pager_tid = master_tid = l4_myself ();
    pager_tid.id.thread = 1;
    l4_thread_ex_regs (pager_tid,
		       (dword_t) pager,
		       (dword_t) pager_stack + sizeof (pager_stack),
		       &foo, &foo, &dummy, &dummy, &dummy);

    /* Increase prio of self. */
    l4_set_prio (l4_myself(), MASTER_PRIO);
    l4_set_prio (pager_tid, MASTER_PRIO);

    for (;;)
    {
	for (;;)
	{
	    printf ("\nPlease select interrupt ipc type:\n");
	    printf ("\n"
		    "0: INTER-AS\n"
		    "1: INTER-AS (small)\n"
		    "2: INTRA-AS\n");
	    char c = kd_inchar();
	    if (c == '0') { is_inter_as = true;  is_small = false; break; };
	    if (c == '1') { is_inter_as = true;  is_small = true;  break; };
	    if (c == '2') { is_inter_as = false; is_small = false; break; };
	}

	extern dword_t _end, _start;
	for (ptr_t x = (&_start); x < &_end; x++)
	{
	    dword_t q;
	    q = *(volatile dword_t*)x;
	}

	starter_tid = int_tid = worker_tid = master_tid;
    
	if (is_inter_as)
	{
	    int_tid.id.task += 1;
	    worker_tid.id.task += 2;
	    int_tid.id.thread = 0;
	    worker_tid.id.thread = 0;

	    /* Create separate tasks for both threads. */
	    l4_task_new (int_tid, 255,
			 (dword_t) &int_stack[1024],
			 (dword_t) int_thread, pager_tid);

	    l4_task_new (worker_tid, 255,
			 (dword_t) &worker_stack[1024],
			 (dword_t) worker_thread, pager_tid);

	    if (is_small)
	    {
		l4_set_small (int_tid, l4_make_small_id (8, 0));
		l4_set_small (worker_tid, l4_make_small_id (8, 1));
	    }

	    l4_set_prio (int_tid, INT_PRIO);
	    l4_set_prio (worker_tid, WORKER_PRIO);
	}
	else
	{
	    int_tid.id.task += 1;
	    int_tid.id.thread = 0;
	    worker_tid.id.task = int_tid.id.task;
	    worker_tid.id.thread = 1;

	    /* Create new task containing the new threads. */
	    l4_task_new (int_tid, 255,
			 (dword_t) &int_stack[1024],
			 (dword_t) startup_worker, pager_tid);
	}

	/* Wait for measurement to finish. */
	l4_ipc_receive (int_tid, 0,
			&dummy, &dummy, &dummy,
			L4_IPC_NEVER, &dope);

	l4_task_new (int_tid, 0, 0, 0, L4_NIL_ID);
	if (is_inter_as)
	    l4_task_new (worker_tid, 0, 0, 0, L4_NIL_ID);
    }

    for (;;)
	enter_kdebug ("EOW");    
}
