/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86_smp.c
 * Description:   kernel-debugger SMP functionality
 *                
 * @LICENSE@
 *                
 * $Id: x86_smp.c,v 1.1 2001/12/04 21:17:43 uhlig Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include "kdebug.h"

#if defined(CONFIG_SMP)
#include <x86/smp.h>

INLINE void broadcast_nmi_ipi()
{
    /* send nmi to all cpu's excl. myself */
    set_local_apic(X86_APIC_INTR_CMD2, 0xff << (56 - 32));
    set_local_apic(X86_APIC_INTR_CMD1, 0xc0000 | APIC_DEL_NMI);
}


void smp_handler_control_ipi()
{
    //printf("cpu %d: smp control ipi\n", get_cpu_id());
    /* currently only used for enter_kdebug */
    //printf("#cpu %d, ", get_cpu_id());
    asm("int3;nop\n");
    //printf("$CPU %d, ", get_cpu_id());

    apic_ack_irq();
}

static volatile int kdb_first_cpu = 0;
void smp_enter_kdebug()
{
    if (!(kdb_first_cpu++))
	broadcast_nmi_ipi();
    //printf("cpu %d entered kdb\n", get_cpu_id());
    //cpu_mailbox_t * mailbox = get_mailbox();
    //mailbox->broadcast_command(SMP_CMD_ENTER_KDEBUG);
}

void smp_leave_kdebug()
{
    kdb_first_cpu = 0;
}

extern cpu_mailbox_t cpu_mailbox[];
static void kdebug_dump_mailboxes()
{
    for (int i = 0; i < CONFIG_SMP_MAX_CPU; i++)
    {
	printf("cpu: %d\tpend: %x\n", i, cpu_mailbox[i].pending_requests);
	printf("command: %d\tstatus: %d\ttcb: %x\n", 
	       cpu_mailbox[i].get_command(),
	       cpu_mailbox[i].get_status(), 
	       cpu_mailbox[i].tid);
	printf("params:\t");
	for (int j = 0; j < MAX_MAILBOX_PARAM; j++)
	    printf("%x ", cpu_mailbox[i].param[j]);
	printf("\n\n");
    }
}

void kdebug_smp_info()
{
    switch(kdebug_get_choice("SMP Info: Mailboxes", "m"))
    {
    case 'm':
	kdebug_dump_mailboxes();
	break;
    }
}
	

#endif /* CONFIG_SMP */
