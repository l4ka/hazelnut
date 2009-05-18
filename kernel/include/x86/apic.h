/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/apic.h
 * Description:   Definitions and inlines for the IA-32 Local APIC and
 *                IO-APIC.
 *                
 * @LICENSE@
 *                
 * $Id: apic.h,v 1.9 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86_APIC_H__
#define __X86_APIC_H__


#define X86_IO_APIC_BASE	(0xFEC00000)
#define X86_LOCAL_APIC_BASE	(0xFEE00000)
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

/* IOAPIC register ids */
#define X86_IOAPIC_ID		0x00
#define X86_IOAPIC_VERSION	0x01
#define X86_IOAPIC_ARB		0x02
#define X86_IOAPIC_REDIR0	0x10

/* APIC delivery modes */
#define APIC_DEL_FIXED		(0x0)
#define APIC_DEL_LOWESTPRIO	(0x1 << 8)
#define APIC_DEL_SMI		(0x2 << 8)
#define APIC_DEL_NMI		(0x4 << 8)
#define APIC_DEL_INIT		(0x5 << 8)
#define APIC_DEL_EXTINT		(0x7 << 8)

#define APIC_TRIGGER_LEVEL	(1 << 15)
#define APIC_TRIGGER_EDGE	0

#define APIC_ASSERT		(1 << 14)
#define APIC_DEASSERT		0

#define APIC_IRQ_MASK		(1 << 16)

#define APIC_DEST_PHYS		0x0
#define APIC_DEST_VIRT		0x1

#define IOAPIC_POLARITY_LOW	0x1
#define IOAPIC_POLARITY_HIGH	0x0

#define IOAPIC_TRIGGER_LEVEL	0x1
#define IOAPIC_TRIGGER_EDGE	0x0

#define IOAPIC_IRQ_MASK		(1 << 16)
#define IOAPIC_IRQ_MASKED	0x1
#define IOAPIC_IRQ_UNMASKED	0x0

#define MAX_IOAPIC_INTR		32					      


typedef union {
    struct {
	unsigned version	: 8;
	unsigned reserved1	: 8;
	unsigned max_lvt	: 8;
	unsigned reserved0	: 8;
    } ver;
    dword_t raw;
} apic_version_t;


typedef struct {
    dword_t vector		: 8,
	    delivery_mode	: 3,
	    dest_mode		: 1,
	    delivery_status	: 1,
	    polarity		: 1,
	    irr			: 1,
	    trigger_mode	: 1,
	    mask		: 1,
	    __reserved1		: 15;
    union {
	struct {
	    dword_t __reserved_1	: 24,
		    physical_dest	: 4,
		    __reserved_2	: 4;
	} physical;

	struct {
	    dword_t __reserved_1	: 24,
		    logical_dest	: 8;
	} logical;
    } dest;
} apic_redir_t __attribute__((packed));


/* 
 * local APIC 
 */
INLINE dword_t get_local_apic(dword_t reg)
{
    dword_t tmp;
    tmp = *(__volatile__ dword_t*)(KERNEL_LOCAL_APIC + reg);
    return tmp;
}

INLINE void set_local_apic(dword_t reg, dword_t val)
{
    *(__volatile__ dword_t*)(KERNEL_LOCAL_APIC + reg) = val;
}

INLINE void apic_ack_irq()
{
    get_local_apic(X86_APIC_SVR);
    set_local_apic(X86_APIC_EOI, 0);
}



/* 
 *   IO-APIC 
 */
INLINE dword_t get_io_apic(dword_t reg)
{
    *(__volatile__ dword_t*)(KERNEL_IO_APIC) = reg;
    return *(__volatile__ dword_t*)(KERNEL_IO_APIC + 0x10);
}

INLINE void set_io_apic(dword_t reg, dword_t val)
{
    *(__volatile__ dword_t*)(KERNEL_IO_APIC) = reg;
    *(__volatile__ dword_t*)(KERNEL_IO_APIC + 0x10) = val;
}


INLINE void io_apic_mask_irq(int num)
{
    int reg = X86_IOAPIC_REDIR0 + (num * 2);
    set_io_apic(reg, get_io_apic(reg) | IOAPIC_IRQ_MASK);
}

INLINE void io_apic_unmask_irq(int num)
{
    int reg = X86_IOAPIC_REDIR0 + (num * 2);
    set_io_apic(reg, get_io_apic(reg) & (~IOAPIC_IRQ_MASK));
}


void setup_local_apic() L4_SECT_INIT;

#endif /* __X86_APIC_H__ */
