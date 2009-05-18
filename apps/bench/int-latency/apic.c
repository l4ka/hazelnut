/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     bench/int-latency/apic.c
 * Description:   Setup local APIC
 *                
 * @LICENSE@
 *                
 * $Id: apic.c,v 1.1 2002/05/07 19:09:30 skoglund Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include "apic.h"

dword_t local_apic;

void setup_local_apic (dword_t apic_location)
{
    dword_t tmp;
    extern dword_t int_num;

    local_apic = apic_location;

    /* Enable local apic. */
    tmp = get_local_apic (X86_APIC_SVR);
    tmp |= 0x100;			/* enable APIC */
    tmp &= ~(0x200);			/* enable focus processor */
    set_local_apic (X86_APIC_SVR, tmp);

    /* Set prio to accept all. */
    tmp = get_local_apic (X86_APIC_TASK_PRIO);
    tmp &= ~0xff;
    set_local_apic (X86_APIC_TASK_PRIO, tmp);

    /* Flat mode. */
    tmp = get_local_apic (X86_APIC_DEST_FORMAT);
    tmp |= 0xf0000000;
    set_local_apic (X86_APIC_DEST_FORMAT, tmp);

    /* Divider = 1 */
    set_local_apic (X86_APIC_TIMER_DIVIDE,
		    (get_local_apic (X86_APIC_TIMER_DIVIDE) & ~0xf) | 0xb);

    /* Periodic timer, masked */
    set_local_apic (X86_APIC_LVT_TIMER, APIC_IRQ_MASK |
		    APIC_TRIGGER_PERIODIC | ((int_num + 32) & 0xff));
}
