/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     bench/int-latency/apic.h
 * Description:   APIC register and bit definitions
 *                
 * @LICENSE@
 *                
 * $Id: apic.h,v 1.1 2002/05/07 19:09:46 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __APIC_H__
#define __APIC_H__


/*
 * APIC register locations.
 */

#define X86_APIC_ID		0x020
#define X86_APIC_VERSION	0x030
#define X86_APIC_TASK_PRIO	0x080
#define X86_APIC_ARBITR_PRIO	0x090
#define X86_APIC_PROC_PRIO	0x0A0
#define X86_APIC_EOI		0x0B0
#define X86_APIC_LOCAL_DEST	0x0D0
#define X86_APIC_DEST_FORMAT	0x0E0
#define X86_APIC_SVR		0x0F0
#define X86_APIC_ISR_BASE	0x100
#define X86_APIC_TMR_BASE	0x180
#define X86_APIC_IRR_BASE	0x200
#define X86_APIC_ERR_STATUS	0x280
#define X86_APIC_INTR_CMD1	0x300
#define X86_APIC_INTR_CMD2	0x310
#define X86_APIC_LVT_TIMER	0x320
#define X86_APIC_PERF_COUNTER	0x340
#define X86_APIC_LVT_LINT0	0x350
#define X86_APIC_LVT_LINT1	0x360
#define X86_APIC_LVT_ERROR	0x370
#define X86_APIC_TIMER_COUNT	0x380
#define X86_APIC_TIMER_CURRENT	0x390
#define X86_APIC_TIMER_DIVIDE	0x3E0


/*
 * APIC LVT bits.
 */

#define APIC_DEL_FIXED		(0x0 << 8)
#define APIC_DEL_LOWESTPRIO	(0x1 << 8)
#define APIC_DEL_SMI		(0x2 << 8)
#define APIC_DEL_NMI		(0x4 << 8)
#define APIC_DEL_INIT		(0x5 << 8)
#define APIC_DEL_EXTINT		(0x7 << 8)

#define APIC_TRIGGER_LEVEL	(1 << 15)
#define APIC_TRIGGER_EDGE	(0 << 15)

#define APIC_IRQ_MASK		(1 << 16)

#define APIC_TRIGGER_ONE_SHOT	(0 << 17)
#define APIC_TRIGGER_PERIODIC	(1 << 17)


/*
 * Interface to APIC.
 */

inline dword_t get_local_apic (dword_t reg)
{
    extern dword_t local_apic;
    dword_t tmp;
    tmp = *(volatile dword_t *) (local_apic + reg);
    return tmp;
}

inline void set_local_apic (dword_t reg, dword_t val)
{
    extern dword_t local_apic;
    *(volatile dword_t *) (local_apic + reg) = val;
}


inline void apic_ack_irq (void)
{
    set_local_apic (X86_APIC_EOI, 0);
}


void setup_local_apic (dword_t apic_location);


#endif /* !__APIC_H__ */
