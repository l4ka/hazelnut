/*********************************************************************
 *		  
 * Copyright (C) 1999, 2000, 2001-2002,	 Karlsruhe University
 *		  
 * File path:	  x86/interrupt.c
 * Description:	  Interrupt and exceptiond handling and initialization
 *		  for the x86.
 *		  
 * @LICENSE@
 *		  
 * $Id: interrupt.c,v 1.64 2002/05/31 11:51:44 stoess Exp $
 *		  
 ********************************************************************/
#include <universe.h>
#include <init.h>
#include <tracepoints.h>
#include <x86/init.h>
#include <x86/cpu.h>

#if defined(CONFIG_IO_FLEXPAGES)
#include <x86/syscalls.h>
#include <x86/io_mapping.h>
#endif

#if defined(CONFIG_ENABLE_PVI)
/* Virtual Interrupts Indicator*/
dword_t pending_irq;
#define IRQ_IN_SERVICE	(1<<31)

#define MAX_PENDING_IRQS 16
#define PENDING_IRQ_BIT(x) (1 << x)
#define PENDING_TIMER_IRQ  (1 << 31)

void handle_pending_irq(void);

#endif /* CONFIG_ENABLE_PVI */


#ifndef CONFIG_X86_APIC

#ifdef CONFIG_MEASURE_INT_LATENCY
#include <x86/apic.h>
#endif

#ifdef CONFIG_X86_INKERNEL_PIC
static byte_t pic1_mask, pic2_mask;
#endif


/**********************************************************************
 *
 *			 8259A - standard PIC 
 *
 **********************************************************************/

static void disable_8259A_irq(unsigned int irq)
{
    if (irq & 8) PIC2_MASK(irq - 8);
	    else PIC1_MASK(irq);
}

static void enable_8259A_irq(unsigned int irq)
{
    if (irq & 8) PIC2_UNMASK(irq - 8);
	    else PIC1_UNMASK(irq);
}

static int i8259A_irq_pending(unsigned int irq)
{
    unsigned int mask = 1<<irq;

    if (irq & 8)
	return (inb(X86_PIC1_CMD) & mask);
    return (inb(X86_PIC2_CMD) & (mask >> 8));
}

/*
 * Careful! The 8259A is a fragile beast, it pretty
 * much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI
 * to the two 8259s is important!
 */
static void mask_and_ack_8259A(unsigned int irq)
{
    if (irq & 8) {
	PIC2_MASK(irq - 8);
	outb(X86_PIC2_CMD, 0x60 + (irq&7));	/* Specific EOI to slave */
	outb(X86_PIC1_CMD, 0x62);	      /* Specific EOI to cascade */
    } else {
	PIC1_MASK(irq);
	outb(X86_PIC1_CMD, 0x60 + (irq));
    }
}

static void mask_8259A(unsigned int irq)
{
    if (irq & 8) {
	PIC2_MASK(irq - 8);
	outb(X86_PIC1_CMD, 0x62);	/* Specific EOI to cascade */
    } else {
	PIC1_MASK(irq);
    }
}

void init_irqs()
{
    printf("%s\n", __FUNCTION__);
    
    /* first mask all interrupts */
    PIC1_SET_MASK(0xff);
    PIC2_SET_MASK(0xff);

    /* setup pic (directly copied from Jochen's init)*/
    outb(X86_PIC1_CMD, 0x11);
    outb(X86_PIC1_DATA, 0x20); /* start at idt entry 0x20 */
    outb(X86_PIC1_DATA, 0x04);
    outb(X86_PIC1_DATA, 0x01); /* mode - *NOT* fully nested */
    outb(X86_PIC1_CMD, 0xc1);  /* prio 8..15, 3..7, 0, 1 */

    outb(X86_PIC2_CMD, 0x11);
    outb(X86_PIC2_DATA, 0x28); /* start at idt entry 0x28 */
    outb(X86_PIC2_DATA, 0x02);
    outb(X86_PIC2_DATA, 0x01); /* not fully nested */
    outb(X86_PIC2_CMD, 0xc7);  /* prios... */

    outb(X86_PIC1_CMD, 0x0A);
    outb(X86_PIC2_CMD, 0x0A);

    /* enable timer irq (int 8) and init PIC mask cache */
    PIC1_SET_MASK(0xfb);
    PIC2_SET_MASK(0xfe);

    outb(X86_PIC1_CMD, 0x60);

    /* enable nmi */
    in_rtc(0);
    outb(0x61, inb(0x61) & ~(0xC));

#if defined(CONFIG_ENABLE_PVI)
    pending_irq = 0;
#endif	  
}


void mask_interrupt(unsigned int number)
{
    disable_8259A_irq(number);
}

void unmask_interrupt(unsigned int number)
{
#ifdef CONFIG_X86_INKERNEL_PIC
    enable_8259A_irq(number);
#endif
}
	


void irq_handler(exception_frame_t *frame)
{

    /* We do not need special handling for the timer IRQ here, 
       because this is done by timer_irq_handler(). */

    TRACEPOINT_1PAR(INTERRUPT, frame->fault_code,
		    printf("irq_handler: irq #%d\n", frame->fault_code));
    
#ifdef CONFIG_X86_INKERNEL_PIC 
    mask_and_ack_8259A(frame->fault_code); 
#else 
    disable_8259A_irq(frame->fault_code); 
#endif 

#if defined(CONFIG_ENABLE_PVI)

    /*
     *	If IOPL == 3, we're either sigma0 or idle; in this case we'll handle the
     * the interrupt.
     * Otherwise we check the VIF bit. If it is 0, we tag the interrupt bit in
     * the fiels to  handle it later.
     */
    if(((frame->eflags & X86_EFL_IOPL(3)) == 0) 
       && ((frame->eflags & X86_EFL_VIF) == 0)){
	
	frame->eflags |= X86_EFL_VIP;

	tcb_t *current = get_current_tcb();
	current->resources |= TR_PVI;
	
	/* Save pending interrupt */
	pending_irq |= (1<< (frame->fault_code));

	// printf("IRQ Handler irq=%x, enq irq_pending %x\n", frame->fault_code,
	// pending_irq);
	
	return;
    }
#endif /* CONFIG_ENABLE_PVI */

    // printf("%s - irq: %d, tcb=%p\n", __FUNCTION__, frame->fault_code, get_current_tcb());
    // printf("IRQ %d pending\n ", frame->fault_code);

    handle_interrupt(frame->fault_code);
}


#else /* CONFIG_X86_APIC */

/**********************************************************************
 *
 *			 APIC and IO-APIC 
 *
 **********************************************************************/

#include <x86/apic.h>

#warning APIC experimental

void apic_setup_timer(int tickrate) L4_SECT_INIT;
void setup_local_apic() L4_SECT_INIT;


/*
 * Function for accessing PCI configuration space with type 1 accesses
 */
static byte_t pci_read_config_byte(byte_t bus, byte_t devfn, byte_t where)
{
    byte_t value;
    outl(0xCF8, (0x80000000 | (bus << 16) | (devfn << 8) | (where & ~3)));
    value = inb(0xCFC + (where&3));
    return value;
}

/* store information about io-apic interrupts */
static apic_redir_t io_apic_intr[MAX_IOAPIC_INTR];
/* A table for getting the IO-APIC line for an IRQ number */
static byte_t irq_to_ioapicline_map[16];
static void setup_io_apic() L4_SECT_INIT;

#include INC_ARCH(mps-14.h)

#if defined(CONFIG_DEBUG_TRACE_INIT)
#define Dprintf(x...) printf(x)
#else
#define Dprintf(x...)
#endif

void setup_io_apic()
{
    /* check for io apic */
    apic_version_t io_apic_version;

    io_apic_version.raw = get_io_apic(X86_IOAPIC_VERSION);

    if (io_apic_version.ver.version != 0x11) {
	printf("Panic: no io APIC detected. syshalt.");
	asm("hlt\n");
    }

    Dprintf("io-apic max lvt: %d\n", io_apic_version.ver.max_lvt);

    int num_irqs = io_apic_version.ver.max_lvt + 1;

    /* first mask all interrupts on the PIC */
    outb(X86_PIC1_DATA, 0xff);
    outb(X86_PIC2_DATA, 0xff);

    /* set apic to symmetric mode */
    outb(0x22, 0x70);
    outb(0x23, 0x01);


    /* initialize the array */
    for (int i = 0; i < num_irqs; i++)
    {
	apic_redir_t *entry = &io_apic_intr[i];
	entry->vector = 0;
	entry->delivery_mode = APIC_DEL_FIXED;
	entry->dest_mode = APIC_DEST_PHYS;
	entry->trigger_mode = 0;
	entry->polarity = 0;
	entry->mask = IOAPIC_IRQ_MASKED;
	entry->dest.physical.physical_dest = 0; /* send everything to apic 0 */
    }
    

    intel_mp_floating_t *f;
    byte_t isabus_id = 0xFE;
    byte_t ioapic_id = 0xFE;
    for (f = (intel_mp_floating_t*) 0xF0000;
	 f < (intel_mp_floating_t*) 0x100000;
	 f = (intel_mp_floating_t*) (((dword_t) f) + 0x10))
	if (f->signature == MPS_MPF_SIGNATURE)
	{
#if defined(CONFIG_DEBUG_TRACE_INIT)
	    printf("SMP: Found MPS signature at %x\n", f);
	    printf("SMP:  Version 1.%c\n", '0' + f->specification);
	    if (f->feature[0] != 0)
	    {
		printf("SMP:  Default config requested. No idea how!\n");
		f = (intel_mp_floating_t*) 0x100000;
		break;
	    };
#endif
	    mp_config_table_t *t = (mp_config_table_t*) f->physptr;
	    
	    if (t->signature == MPS_MPC_SIGNATURE)
	    {
		Dprintf("SMP: Found MPC table at %x\n", t);
		Dprintf("SMP:  Length: %x\n", t->length);
		Dprintf("SMP:  Enties: %d\n", t->entries);
		Dprintf("SMP:  Local APIC at %x\n", t->lapic);

		union e_t {
		    byte_t			type;
		    mpc_config_processor_t	cpu;
		    mpc_config_bus_t		bus;
		    mpc_config_ioapic_t		ioapic;
		    mpc_config_intsrc_t		irq;
		    mpc_config_lintsrc		lint;
		} *e = (union e_t *) (t + 1);
		
		for (int inc=0, i = t->entries;
		     i--;
		     e = (union e_t *) ((dword_t) e + inc))
		    switch (e->type)
		    {
		    case MP_PROCESSOR:
			Dprintf("SMP:	Cpu: %d %cP\n",
				e->cpu.apicid,
				(e->cpu.cpuflag & 2) ? 'B':'A');
			inc = sizeof(e->cpu);
#if defined(CONFIG_SMP)
			extern dword_t processor_map;
			processor_map |= (1 << e->cpu.apicid);
#endif
			break;
		    case MP_BUS:
			Dprintf("SMP:	Bus: %d %c%c%c%c%c%c\n",
				e->bus.busid,
				e->bus.bustype[0], e->bus.bustype[1],
				e->bus.bustype[2], e->bus.bustype[3],
				e->bus.bustype[4], e->bus.bustype[5]);
			if ((e->bus.bustype[0] == 'I') &&
			    (e->bus.bustype[1] == 'S') &&
			    (e->bus.bustype[2] == 'A'))
			{
			    
			    isabus_id = e->bus.busid;
			    Dprintf("SMP:    ISA Bus is %2x\n", isabus_id);
			};
			inc = sizeof(e->bus);
			break;
		    case MP_IOAPIC:
			Dprintf("SMP:	IOAPIC: id=%2x, flags=%02, ver=%2x @ %p\n",
				e->ioapic.apicid, e->ioapic.flags,
				e->ioapic.apicver, e->ioapic.apicaddr);
			/* keep the IOAPICs id */
			if (e->ioapic.flags & 1)
			    ioapic_id = e->ioapic.apicid;
			inc = sizeof(e->ioapic);
			break;
		    case MP_INTSRC:
			Dprintf("SMP:	IRQ: %2x %4x %2x %2x %2x %2x\n",
				e->irq.irqtype, e->irq.irqflag,
				e->irq.srcbus, e->irq.srcbusirq,
				e->irq.dstapic, e->irq.dstirq);
			if (e->irq.irqtype == 0)
			    /* set up ISA irqs - if we know the ISA bus already */
			    if (e->irq.dstapic == ioapic_id)
			    {
				apic_redir_t *entry = &io_apic_intr[e->irq.dstirq];
				/* PCI defaults to level-triggered, active low */
				int dpo = 1, dtm = 1;
				if (e->irq.srcbus == isabus_id)
				{
				    Dprintf("SMP:    ISA irq 0x%2x is on APIC pin 0x%2x\n",
					    e->irq.srcbusirq, e->irq.dstirq);
				    entry->vector = e->irq.srcbusirq + 0x20;
				    irq_to_ioapicline_map[e->irq.srcbusirq] = e->irq.dstirq;
				    /* ISA defaults to edge-triggered, active high */
				    dpo = 0;
				    dtm = 0;
				};
				/* (irqflag & 0x3) - polarity
				   = 0 - default, = 1 - active high, = 2 - reserved, = 3 - active low
				   IO-APIC: 0 - active high, 1 - active low */
				entry->polarity = ((int []){ dpo, 0, 0, 1})[e->irq.irqflag & 3];
				/* ((irqflag >> 2) & 0x3) - trigger mode
				   = 0 - default, = 1 - edge, = 2 - reserved, = 3 - level
				   IO-APIC: 0 - edge, 1 - level */
				entry->trigger_mode = ((int []){ dtm, 0, 0, 1})[(e->irq.irqflag >> 2) & 3];
			    };
			inc = sizeof(e->irq);
			break;
		    case MP_LINTSRC:
			Dprintf("SMP:	LINT: %2x %4x %2x %2x %2x %2x\n",
				e->lint.irqtype, e->lint.irqflag,
				e->lint.srcbus, e->lint.srcbusirq,
				e->lint.dstapic, e->lint.dstlint);
			inc = sizeof(e->lint);
			break;
#if defined(CONFIG_DEBUG_TRACE_INIT)
		    default:
			printf("SMP:   Unknown type %2x. Aborting\n", e->type);
			i = 0;
			break;
#endif
		    };
		
	    }
#if defined(CONFIG_DEBUG_TRACE_INIT)
	    else
	    {
		printf("MPC shall be at %x\n", f->physptr);
		printf("mpc.sig: %x should be %x\n",
		       t->signature, MPS_MPC_SIGNATURE);
	    }
#endif
	    /* exit from scan loop */
	    break;
	}
    
#if defined(CONFIG_DEBUG_TRACE_INIT)
    /* when we can't find anything useful go with very hacky defaults */
    if ( f >= (intel_mp_floating_t*) 0x100000)
    {
	printf("SMP: couldn't find MPS floating structure.\n"
	       "SMP: Doing default setup.\n");
	
	/* initialize the ISA interrupts - line 0 is unused */
	for (int i = 1; i < 16; i++)
	{
	    apic_redir_t *entry = &io_apic_intr[i];
	    entry->vector = i + 0x20; /* hw irqs start at 0x20 */
	    entry->trigger_mode = 0;
	    entry->polarity = 0;
	    irq_to_ioapicline_map[i] = i;
	}
	/* IRQ0 is linked to input line 2 - link that back to IRQ0 */
	io_apic_intr[2].vector = 0x20;
	irq_to_ioapicline_map[0] = 2;
    }
#endif
    
    
    {
	/* try to figure out the mapping of PCI INT# to ISA IRQ#
	   - PCI INT#A is wired to IO-APIC's INT16 input
	   - PCI INT#B is wired to IO-APIC's INT17 input
	   - PCI INT#C is wired to IO-APIC's INT18 input
	   - PCI INT#D is wired to IO-APIC's INT19 input	*/
	
	/* arry to temporarily store the apicpin_to_irq mapping */
	char ioapicline_to_irq[4] = { 0 , 0, 0, 0 };
	
	/* scan BIOS area for the "$PIR" signature of the PCI IRQ Routing table */
	struct irq_routing_table *r;
	for (r = (irq_routing_table*) 0xF0000;
	     r < (irq_routing_table*) 0x100000;
	     r = (irq_routing_table*) (((dword_t) r) + 0x10))
	    if (r->signature == MPS_PIRT_SIGNATURE)
	    {
		Dprintf("IRQ: Found PCI IRQ routing information at %p\n", r);

#if defined(CONFIG_DEBUG_TRACE_INIT)
		int slots = (r->size - sizeof(irq_routing_table))/sizeof(irq_info);
		printf("IRQ:  %d slots in table\n", slots);
		
		for (int i = slots; i--;)
		{
		    printf("IRQ:   %d: bus=%02x dev=%02x link=(%02x,%02x,%02x,%02x) slot=%02x\n",
			   i,
			   r->slots[i].bus, r->slots[i].devfn,
			   r->slots[i].irq[0].link, 
			   r->slots[i].irq[1].link, 
			   r->slots[i].irq[2].link, 
			   r->slots[i].irq[3].link, 
			   r->slots[i].slot);

		};
#endif

		/* do we know how to handle the IRQ router ? */
		switch((r->rtr_vendor << 16) | (r->rtr_device))
		{
		case 0x80867000:
		{
		    Dprintf("IRQ:  Router is Intel PIIX4\n");
		    
		    /* The PIIX stores the mapping in its PCI config space at
		       offsets 0x60-0x63 for INTA-INTD.
		       See 82371AB (PIIX4) specification, section 4.1.10, p.59	*/
		    for (int i = 0; i < 4; i++)
		    {
			int irq;
			irq = pci_read_config_byte(r->rtr_bus, r->rtr_devfn, 0x60 + i);
			/* bit 7 = 0 marks valid entries */
			if ((irq & 0x80) == 0)	
			{
			    ioapicline_to_irq[i] = irq;
			    Dprintf("IRQ:   INT#%c routed to vector %d\n", 'A' + i, irq);
			}
		    }
		}; break;
		case 0x11660200:
		{
		    Dprintf("IRQ:  Router is ServerWorks\n");
		    
		    /* port 0xc00 is index register
		       port 0xc01 is data register	*/
		    for (int i = 0; i < 4; i++)
		    {
			int irq;
			outb(0xc00, i | 0x10);
			irq = inb(0xc01);
			Dprintf("IRQ:	Got back %02x for %02x\n", irq + 0x66, i);
			{
			    ioapicline_to_irq[i] = irq;
			    Dprintf("IRQ:   INT#%c routed to vector %d\n", 'A' + i, irq);
			}
		    }
		}; break;
#if defined(CONFIG_DEBUG_TRACE_INIT)
		default:
		{
		    printf("IRQ: Unknown PCI IRQ router %4x:%4x on %2x:%2x ... :-(\n",
			   r->rtr_vendor, r->rtr_device, r->rtr_bus, r->rtr_devfn);
		}; break;
#endif
		}
	    }
	
	/* with the information gathered configure the PCI inputs of the IO-APIC */
	for (int i = 16; i < 20; i++)
	{
	    char irq;
	    apic_redir_t *entry = &io_apic_intr[i];
	    irq = ioapicline_to_irq[i-16];
	    if (irq == 0)
		continue;	/* no entry */
	    entry->vector = irq + 0x20; /* hw irqs start at 0x20 */
	    irq_to_ioapicline_map[irq] = i;
	};

    }

    /* write the entries to the IO-APIC */
    for (int i = 0; i < num_irqs; i++)
    {
	apic_redir_t *entry = &io_apic_intr[i];
	set_io_apic(X86_IOAPIC_REDIR0 + (i*2) + 0, *(((dword_t*)entry) + 0));
	set_io_apic(X86_IOAPIC_REDIR0 + (i*2) + 1, *(((dword_t*)entry) + 1));
#if 0
	printf("ioredir(%d): %x, %x\n", i,
	       get_io_apic(X86_IOAPIC_REDIR0 + 0 + (i*2)),
	       get_io_apic(X86_IOAPIC_REDIR0 + 1 + (i*2)));
#endif
    }
}


/* 
 * initializes the local apic
 * the timer setup happens in apic_second_tick after 
 * measuring the duration of one second
 */
void setup_local_apic()
{
    dword_t tmp;

    /* enable local apic */
    tmp = get_local_apic(X86_APIC_SVR);
    tmp |= 0x100;	/* enable APIC */
    tmp &= ~(0x200);	/* enable focus processor */
    ASSERT((APIC_SPURIOUS_INT_VECTOR & 0xf) == 0xf);
    tmp |= APIC_SPURIOUS_INT_VECTOR;	/* spurious interrupt = 0xff */
    set_local_apic(X86_APIC_SVR, tmp);

    /* set prio to accept all */
    tmp = get_local_apic(X86_APIC_TASK_PRIO);
    tmp &= ~0xff;
    set_local_apic(X86_APIC_TASK_PRIO, tmp);

    /* flat mode */
    tmp = get_local_apic(X86_APIC_DEST_FORMAT);
    tmp |= 0xf0000000;
    set_local_apic(X86_APIC_DEST_FORMAT, tmp);

    /* set lint0 as extINT - keep disabled for now */
    set_local_apic(X86_APIC_LVT_LINT0, 0x18000 | APIC_LINT0_INT_VECTOR);
    /* set lint1 as NMI */
    set_local_apic(X86_APIC_LVT_LINT1, 0x08000 | APIC_DEL_NMI);
    /* set error handler - keep disabled for now */
    set_local_apic(X86_APIC_LVT_ERROR, 0x18000 | APIC_ERROR_INT_VECTOR);
    /* set timer handler - keep it disabled for now */
    set_local_apic(X86_APIC_LVT_TIMER, APIC_TIMER_INT_VECTOR | APIC_IRQ_MASK);

#if defined(CONFIG_ENABLE_PROFILING)
# if defined(CONFIG_PROFILING_WITH_NMI)
    /* Set perfctr as NMI */
    set_local_apic(X86_APIC_PERF_COUNTER, 0x0400 | APIC_PERFCTR_INT_VECTOR);
# else
    /* Set perfctr as Fixed */
    set_local_apic(X86_APIC_PERF_COUNTER, 0x0000 | APIC_PERFCTR_INT_VECTOR);
# endif
#endif

    /* Enable NMI */
    in_rtc(0);
    outb(0x61, inb(0x61) & ~(0xC));
}

void init_irqs()
{
#if defined(CONFIG_DEBUG_TRACE_INIT)
    printf("%s\n", __FUNCTION__);
#endif
    
    /* first mask all interrupts on the pic */
    outb(X86_PIC1_DATA, 0xff);
    outb(X86_PIC2_DATA, 0xff);

    /* set apic to symetric mode */
    outb(0x22, 0x70);
    outb(0x23, 0x01);

#if defined(CONFIG_DEBUG_TRACE_INIT)
    printf("local APIC-id: %x\n", get_local_apic(X86_APIC_ID));
    printf("local APIC-Version: %x\n", get_local_apic(X86_APIC_VERSION));
    printf("local APIC SVR: %x\n", get_local_apic(X86_APIC_SVR));

    printf("io APIC-id: %x\n", get_io_apic(X86_IOAPIC_ID));
    printf("io APIC-Version: %x\n", get_io_apic(X86_IOAPIC_VERSION));
#endif

    /* check for local apic */
    if ((get_local_apic(X86_APIC_VERSION) & 0xf0) != 0x10) {
	printf("Panic: no local APIC detected. syshalt.");
	asm("hlt\n":);
    }
    setup_local_apic();

    /* initialize io apic interrupts */
    setup_io_apic();
}

/* timer is handled in apic_handle_timer */
void irq_handler(exception_frame_t *frame)
{
    //printf("irq %d\n", frame->fault_code);

    TRACEPOINT_1PAR(INTERRUPT, frame->fault_code,
		    printf("irq_handler: irq #%d\n", frame->fault_code));

    /* edge triggered irq's can be simply ack'd
     * level triggered have to be masked first.
     * they are re-enabled in the ipc path
     */
    if (io_apic_intr[irq_to_ioapicline_map[frame->fault_code]].trigger_mode == IOAPIC_TRIGGER_LEVEL)
    {
	//printf("mask_irq %d\n", frame->fault_code);
	io_apic_mask_irq(irq_to_ioapicline_map[frame->fault_code]);
    }
    apic_ack_irq();

#if defined(CONFIG_ENABLE_PVI)
    /*
     * If IOPL == 3, we're either sigma0 or idle; in this case we'll handle the
     * the interrupt.
     * Otherwise we check the VIF bit. If it is 0, we tag the interrupt bit in
     * the field to handle it later.
     */
    if(	 ((frame->eflags & X86_EFL_IOPL(3)) == 0) 
	 && ((frame->eflags & X86_EFL_VIF) == 0) )

    {
	tcb_t *current = get_current_tcb();
	/* set VIP flag - sti will cause #GP */
	frame->eflags |= X86_EFL_VIP;
	current->resources |= TR_PVI;
	
	/* Save pending interrupt */
	pending_irq |= (1 << frame->fault_code);
	
	/* Bail out - keep the actual IRQ handling for later */
	return;
    }
#endif /* CONFIG_ENABLE_PVI */

    handle_interrupt(frame->fault_code);
}

void apic_handler_lint0()
{
    enter_kdebug("apic_handler_lint0");
    apic_ack_irq();
}

void apic_handler_lint1()
{
    enter_kdebug("apic_handler_lint1");
    apic_ack_irq();
}

void apic_handler_error()
{
    enter_kdebug("apic_handler_error");
    apic_ack_irq();
}

void apic_handler_spurious_int()
{
    enter_kdebug("apic_handler_spurious");
    apic_ack_irq();
}

void apic_handler_timer()
{
    apic_ack_irq();
    handle_timer_interrupt();
}


void mask_interrupt(unsigned int number)
{
    //printf("mask_interrupt %d\n", number);
    io_apic_mask_irq(irq_to_ioapicline_map[number]);
}

void unmask_interrupt(unsigned int number)
{
    //printf("unmask_interrupt %d\n", number);
    io_apic_unmask_irq(irq_to_ioapicline_map[number]);
}


#endif /* CONFIG_X86_APIC */



void timer_irq_handler(exception_frame_t *frame)
{

#if defined(CONFIG_ENABLE_PVI)

    /*
     *	If IOPL == 3, we're either sigma0 or idle; in this case we'll handle the
     * the interrupt.
     * Otherwise we check the VIF bit. If it is 0, we bear the interrupt in mind
     * and handle it later.
     */

    if(	 ((frame->eflags & X86_EFL_IOPL(3)) == 0) 
	 && ((frame->eflags & X86_EFL_VIF) == 0) ){
		
	// printf("enq timer interrupt, eip=%x\n", frame->fault_address);
	// enter_kdebug("timer_irq");

	mask_and_ack_8259A(8);
	/* reset timer intr line */
	in_rtc(0x0c);
	
	frame->eflags |= X86_EFL_VIP;
	pending_irq |= PENDING_TIMER_IRQ;

	tcb_t *current = get_current_tcb();
	current->resources |= TR_PVI;
	return;
    }
    
#endif /* CONFIG_ENABLE_PVI */
	
#if !defined(CONFIG_X86_APICTIMER)
    mask_and_ack_8259A(8);
    /* reset timer intr line */
    in_rtc(0x0c);
    enable_8259A_irq(8);
#else
    apic_ack_irq();
#endif /* CONFIG_X86_APICTIMER */
    handle_timer_interrupt();
}


#define TRACEPOINT_INSTR(instruction...)			\
	TRACEPOINT(INSTRUCTION_EMULATION,			\
		   { printf("instruction emulation @ %p: ",	\
			    frame->fault_address);		\
		     printf(##instruction); })


static int handle_faulting_instruction(tcb_t* current,
				       exception_frame_t* frame)
{
    /* This function returns TRUE if the fault was handled, FALSE otherwise.
       current	pointer to current TCB
       frame	pointer to exception frame
       instr	pointer to first byte of instruction that caused the exception
    */

    unsigned char * instr = (unsigned char *) frame->fault_address;

#if defined(CONFIG_ENABLE_SMALL_AS)
    /*
     * Translate fault address so that we can access it using
     * kernel segments.
     */

    smallid_t sid = current->space->smallid ();
    if (sid.is_valid () && ((dword_t) instr < sid.bytesize ()))
	instr += sid.offset ();
#endif
    
#if defined(CONFIG_ENABLE_PVI)
    if ( (frame->eflags & X86_EFL_VIP) && (frame->eflags & X86_EFL_VIF)){
	// printf("PVI2: tid=%x, eflags=%x\n", current->myself.raw, frame->eflags);
	// enter_kdebug();
	/* Set VIF flag, clear VIP flag */
	frame->eflags |= X86_EFL_VIF;
	frame->eflags &= ~X86_EFL_VIP;
	current->resources &= ~TR_PVI;
		
	/* Handle pending interrupts */
	if (pending_irq){
	    handle_pending_irq();
	}
	return TRUE;
		
    }	
#endif
    switch (instr[0]) {

    case 0x0f:
	switch (instr[1]) {
	case 0x01:
	    if ( instr[2] != 0x18 ) break;
	    /*
	     * 0F 01 18	lidt (%eax)
	     *
	     * Currently, we only support LIDT emulation where the
	     * IDT descriptor address is in EAX.  Other ways to
	     * load IDT are not detected and result in a GP
	     * exception.
	     */
	    if ( frame->eax < USER_AREA_MAX - 6 )
	    {
		dword_t idt = frame->eax + 2;
#if defined(CONFIG_ENABLE_SMALL_AS)
		if (sid.is_valid () && (idt < sid.bytesize ()))
		    idt += sid.offset ();
#endif	  
		idt = *(dword_t *) idt;
		if ( idt >= (USER_AREA_MAX - 32*8) )
		    idt = 0;
		current->excpt = idt;
		TRACEPOINT_INSTR("lidt (idt-desc: %x, idt: %x)\n", 
				 frame->eax, idt);
	    }
	    frame->fault_address += 3;
	    return TRUE;

		   
	case 0x09:
	    /* 
	     * 0F 09 /r wbinvd
	     *
	     * wbinvd emulation is turned off by default but can
	     * be enabled if _really_ necessary
	     */

	    TRACEPOINT_INSTR("wbinvd\n");
#if defined(CONFIG_X86_WBINVD_EMULATION)
	    __asm__("wbinvd\n\r");
#endif
	    frame->fault_address += 2;
	    return TRUE;

	case 0x21:
	case 0x23:
	{
	    /*
	     * 0F 21 /r	mov %dreg, %reg
	     * 0F 23 /r	mov %reg, %dreg
	     *
	     * Virtualize debug registers.	Registers DR4 and DR5
	     * are not accessible and only task local breakpoints
	     * can be enabled.
	     *
	     * HERE BE DRAGONS!  This code is by no means
	     * thoroughly tested and will not work properly in
	     * conjunction with KDB breakpoints.
	     */
#		define MOVDR_GENREG (&frame->eax)[0-(instr[2] & 0x7)]
	    dword_t val = MOVDR_GENREG;
	    dword_t dreg = (instr[2] >> 3) & 0x7;

	    if ( dreg == 4 || dreg == 5 )
		break;

	    if ( instr[1] == 0x21 )
		/* Get register contents from TCB. */
		MOVDR_GENREG = current->priv[0].dbg.dr[dreg];
	    else
	    {
		if ( dreg == 7 ) val &= ~0xaa;

		if ( current->resources & TR_DEBUG_REGS )
		{
		    TRACEPOINT_INSTR("mov reg, dr%d\n", dreg);
		    switch (dreg) {
		    case 0: asm("mov %0, %%dr0" : :"r"(val)); break;
		    case 1: asm("mov %0, %%dr1" : :"r"(val)); break;
		    case 2: asm("mov %0, %%dr2" : :"r"(val)); break;
		    case 3: asm("mov %0, %%dr3" : :"r"(val)); break;
		    case 6: asm("mov %0, %%dr6" : :"r"(val)); break;
		    case 7: asm("mov %0, %%dr7" : :"r"(val)); break;
		    }
		}

		/* Store register contents into TCB. */
		current->priv[0].dbg.dr[dreg] = val;

		if ( dreg == 7 )
		    if ( val & 0x33 )
		    {
			if ( !(current->resources & TR_DEBUG_REGS) )
			    /*
			     * Debug registers are beeing enabled.
			     * Load contents of all debug
			     * registers from TCB.
			     */
			    load_debug_regs((ptr_t) &current->priv[0].dbg);
			current->resources |= TR_DEBUG_REGS;
		    }
		    else
			current->resources &= ~TR_DEBUG_REGS;
	    }
	    enter_kdebug("debug register access");
	    frame->fault_address += 3;
	    return TRUE;
	}
		
	case 0x30:
	case 0x32:
	    /*
	     * 0F 30	wrmsr
	     * 0F 32	rdmsr
	     *
	     * Virtualize accesses to model specific registers.
	     * We must check the validity of the register number
	     * too, or we might end up shooting ourself in the
	     * foot.
	     */
	    switch (frame->ecx & 0xffff) {
#if defined(CONFIG_ARCH_X86_I686)
	    case IA32_PERFCTR0:
	    case IA32_PERFCTR1:
	    case IA32_EVENTSEL0:
	    case IA32_EVENTSEL1:
#elif defined(CONFIG_ARCH_X86_I586)
	    case IA32_TSC:
	    case IA32_CESR:
	    case IA32_CTR0:
	    case IA32_CTR1:
#elif defined(CONFIG_ARCH_X86_P4)
	    default:
#else
	    case 0xffffffff:
#endif
		if ( instr[1] == 0x30 )
		{
		    TRACEPOINT_INSTR("wrmsr (eax=%x, edx=%x, %ecx=%x)\n",
				     frame->eax, frame->edx, frame->ecx);
		    asm volatile ("wrmsr" : :
				  "a"(frame->eax),
				  "d"(frame->edx),
				  "c"(frame->ecx));
		}
		else
		{
		    asm volatile ("rdmsr"
				  : "=a"(frame->eax),
				  "=d"(frame->edx)
				  : "c"(frame->ecx));
		    TRACEPOINT_INSTR("rdmsr (eax=%x, edx=%x, %ecx=%x)\n",
				     frame->eax, frame->edx, frame->ecx);
		}
		frame->fault_address += 2;
		return TRUE;
	    }
	    break;

	case 0xa1:
	case 0xa9:
	    goto pop_seg;
	}
	break;

    case 0x8e:
    case 0x07:
    case 0x17:
    case 0x1f:
    pop_seg:
	/*
	 * 8E /r		mov %reg, %segreg
	 * 07		pop %es
	 * 17		pop %ss
	 * 1F		pop %ds
	 * 0F A1		pop %fs
	 * 0F A9		pop %gs
	 *
	 * Segment descriptor is being written with an invalid
	 * value.  Reset all descriptors with the correct value
	 * and skip the instruction.
	 */
	if ( frame->fault_address >= KERNEL_VIRT ) break;
	TRACEPOINT(SEGREG_RELOAD);

	asm volatile (
#if defined(CONFIG_TRACEBUFFER)		   
	    "	movl %1, %%fs	\n"
#else		     
	    "	movl %0, %%fs	\n"
#endif
	    "	movl %0, %%gs	\n"
	    :
	    :"r"(X86_UDS), "r"(X86_TBS));
	frame->ds = frame->es = X86_UDS;

	frame->fault_address++;
	if ( instr[0] == 0x8e || instr[0] == 0x0f )
	    frame->fault_address++;
	return TRUE;

#if defined(CONFIG_ENABLE_SMALL_AS)
    case 0xcf:
    {
	/*
	 * CF		iret
	 *
	 * When returning to user-level the instruction pointer
	 * might happen to be outside the small space.  If so, we
	 * must promote the space to a large one.
	 */
	if ( (dword_t) instr >= KERNEL_VIRT &&
	     sid.is_valid () &&
	     get_user_ip(current) > sid.bytesize () )
	{
	    small_space_to_large (current->space);
	    return TRUE;
	}
	break;
    }
#endif

    case 0xf4:
	/*
	 * F4		hlt
	 *
	 * Kernel version / API interface detection.  Return the
	 * appropriate values in register EAX.
	 */
	TRACEPOINT(GET_KERNEL_VERSION,
		   printf("GetKernelVersion @ %x\n",frame->fault_address));
	frame->eax = ((KERNEL_VERSION_API << 24) |
		      (KERNEL_VERSION_KID << 20) |
		      (KERNEL_VERSION_VER << 16) |
		      (KERNEL_VERSION_SUB <<  1) | 1);
	frame->fault_address += 1;
	return TRUE;
#if defined(CONFIG_ENABLE_PVI)	
	
    case 0xfb: /* sti */
    {
	/* 
	 * If running in PVI Mode, sti can only cause an exception 
	 * if  a virtual interrupt is pending
	 */
	// printf("PVI: tid = %x, eflags=%x\n", current->myself.raw, frame->eflags);
	// enter_kdebug("");
	/* Set VIF flag, clear VIP flag */
	frame->eflags |= X86_EFL_VIF;
	frame->eflags &= ~X86_EFL_VIP;
	current->resources &= ~TR_PVI;
		
	/* Handle pending interrupts */
	if (pending_irq){
	    handle_pending_irq();
	}
	return TRUE;
    }

#else
/* This can be used to virtualize sti/cli completely in software */
	
#if 0
    case 0xfa: /* sti */
    {
	frame->eflags &= ~X86_EFL_IF;
	frame->fault_address += 1;
	return TRUE;
    }
    case 0xfb: /* sti */
    {
	frame->eflags |= X86_EFL_IF;
	frame->fault_address += 1;
	return TRUE;
    }

#endif	
	
#endif /* CONFIG_ENABLE_PVI */

#if defined(CONFIG_IO_FLEXPAGES)

    case 0xe4:  /* in  %al,	 port imm8  (byte)  */
    case 0xe6:  /* out %al,	 port imm8  (byte)  */
    {
	io_fpage_t io_fault = io_fpage((word_t) instr[1], 0, 0);
	do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	return TRUE;
    }
    case 0xe5:  /* in  %eax, port imm8  (dword) */
    case 0xe7:  /* out %eax, port imm8  (dword) */
    {
	io_fpage_t io_fault = io_fpage((word_t) instr[1],  2, 0);
	do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	return TRUE;
    }
    case 0xec:  /* in  %al,	   port %dx (byte)  */
    case 0xee:  /* out %al,	   port %dx (byte)  */
    case 0x6c:  /* insb	   port %dx (byte)  */
    case 0x6e:  /* outsb	   port %dx (byte)  */
    {
	io_fpage_t io_fault = io_fpage((word_t) (frame->edx & 0xFFFF),	0, 0);
	do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	return TRUE;
    }
    case 0xed:  /* in  %eax,   port %dx (dword) */
    case 0xef:  /* out %eax,   port %dx (dword) */
    case 0x6d:  /* insd	   port %dx (dword) */
    case 0x6f:  /* outsd	   port %dx (dword) */
    {
	io_fpage_t io_fault = io_fpage((word_t) (frame->edx & 0xFFFF), 2, 0);
	printf("pagefault, addr = %x\n", io_fault.raw);
	do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	return TRUE;
    }
    case 0x66:

	/* operand size override prefix */
	switch (instr[1])
	{
	case 0xe5:  /* in  %ax, port imm8  (word) */
	case 0xe7:  /* out %ax, port imm8  (word) */
	{
	    io_fpage_t io_fault = io_fpage((word_t) instr[1], 1 , 0);
	    do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	    return TRUE;
	}
	case 0xed:  /* in  %ax, port %dx  (word) */
	case 0xef:  /* out %ax, port %dx  (word) */
	case 0x6d:  /* insw	port %dx  (word) */
	case 0x6f:  /* outsw	port %dx  (word) */
	{
	    io_fpage_t io_fault = io_fpage((word_t) (frame->edx & 0xFFFF), 1, 0);
	    do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	    return TRUE;
	}
	}
    case 0xf3:
		
	/* rep instruction */
	switch (instr[1]){
	case 0xe4:  /* in  %al,	 port imm8  (byte)  */
	case 0xe6:  /* out %al,	 port imm8  (byte)  */
	{
	    io_fpage_t io_fault = io_fpage((word_t) instr[1], 0, 0);
	    do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	    return TRUE;
	}
	case 0xe5:  /* in  %eax, port imm8  (dword) */
	case 0xe7:  /* out %eax, port imm8  (dword) */
	{
	    io_fpage_t io_fault = io_fpage((word_t) instr[1],  2, 0);
	    do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	    return TRUE;
	}
	case 0xec:  /* in  %al,	   port %dx (byte)  */
	case 0xee:  /* out %al,	   port %dx (byte)  */
	case 0x6c:  /* insb	   port %dx (byte)  */
	case 0x6e:  /* outsb	   port %dx (byte)  */
	{
	    io_fpage_t io_fault = io_fpage((word_t) (frame->edx & 0xFFFF),	0, 0);
	    do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	    return TRUE;
	}
	case 0xed:  /* in  %eax,   port %dx (dword) */
	case 0xef:  /* out %eax,   port %dx (dword) */
	case 0x6d:  /* insd	   port %dx (dword) */
	case 0x6f:  /* outsd	   port %dx (dword) */
	{
	    io_fpage_t io_fault = io_fpage((word_t) (frame->edx & 0xFFFF), 2, 0);
	    do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
	    return TRUE;
	}
	case 0x66:
			
	    /* operand size override prefix */
	    switch (instr[2])
	    {
	    case 0xe5:  /* in  %ax, port imm8  (word) */
	    case 0xe7:  /* out %ax, port imm8  (word) */
	    {
		io_fpage_t io_fault = io_fpage((word_t) instr[1], 1 , 0);
		do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
		return TRUE;
	    }
	    case 0xed:  /* in  %ax, port %dx  (word) */
	    case 0xef:  /* out %ax, port %dx  (word) */
	    case 0x6d:  /* insw	port %dx  (word) */
	    case 0x6f:  /* outsw	port %dx  (word) */
	    {
		io_fpage_t io_fault = io_fpage((word_t) (frame->edx & 0xFFFF), 1, 0);
		do_pagefault_ipc(current, io_fault.raw, frame->fault_address);
		return TRUE;
	    }
	    }
	}
#endif /* CONFIG_IO_FLEXPAGES */
    }

    /* Nothing to repair */
    return FALSE;
}


static int handle_nm_exception(tcb_t *current)
{
    /* the current FPU owner resides in the cpu-local section */
    extern tcb_t* fpu_owner;

    TRACEPOINT(FPU_VIRTUALIZATION,
	       printf("FPU: %x uses fpu at %x, owner=%p\n",
		      current->myself.raw, get_user_ip(current), 
		      fpu_owner));
    
    /* allow access to the FPU until the next thread switch */
    enable_fpu();

    /* mark the FPU usage in the resources */
    current->resources |= TR_FPU;

    /* if we own the fpu just re-enable and go */
    if (fpu_owner == current)
	return TRUE;

#if defined(X86_FLOATING_POINT_ALLOC_SIZE)
# if X86_FLOATING_POINT_ALLOC_SIZE < KMEM_CHUNKSIZE
# error FIXME: Too small allocation size for floating point state
# endif
    /* save the fpu state into the fpu-owner's save area */
    if (fpu_owner != NULL)
	save_fpu_state ((byte_t *) fpu_owner->priv->fpu_ptr);

    /* now reload the fpu state */
    if (current->priv->fpu_ptr)
	load_fpu_state ((byte_t *) current->priv->fpu_ptr);
    else
    {
	init_fpu();
	current->priv->fpu_ptr = (x86_fpu_state_t *)
	    kmem_alloc (X86_FLOATING_POINT_ALLOC_SIZE);
	if (current->priv->fpu_ptr == NULL)
	    panic ("fpu virtualization: alloc mem for fpu state failed");
    }
#else
    /* save the fpu state into the fpu-owner's tcb */
    if (fpu_owner != NULL)
	save_fpu_state ((byte_t *) &fpu_owner->priv->fpu);

    /* now reload the fpu state */
    if (current->priv->fpu_used)
	load_fpu_state ((byte_t *) &current->priv->fpu);
    else
    {
	init_fpu();
	current->priv->fpu_used = 1;
    }
#endif

    /* update the owner */
    fpu_owner = current;
    
    return TRUE;
}


#if defined(CONFIG_ENABLE_SMALL_AS)
extern "C" void sysexit_tramp(void);
extern "C" void reenter_sysexit(void);
#endif

void exception_handler(exception_frame_t * frame)
{
    tcb_t * current = get_current_tcb();

#if defined(CONFIG_DEBUGGER_KDB) && defined(CONFIG_DEBUG_BREAKIN)
    /* We might happen to get into a loop catching exceptions. */
    kdebug_check_breakin();
#endif

    /*
    **
    ** General Protection Fault
    **
    */
    if (frame->fault_code == 13)
    {
	/*
	 * A GP(0) could mean that we have a segment register with
	 * zero contents, either because the user did a reload
	 * (valid), or because of some strange bi-effects of using the
	 * SYSEXIT instruction (see comment in exception.S).
	 */
	if ( frame->error_code == 0 )
	{
	    word_t fs, gs;
	    asm ("	mov	%%fs, %w0	\n"
		 "	mov	%%gs, %w1	\n"
		 :"=r"(fs), "=r"(gs));

	    if ( (frame->ds & 0xffff) == 0 || (frame->es & 0xffff) == 0 ||
		 fs == 0 || gs == 0 )
	    {
		TRACEPOINT (SEGREG_RELOAD, kdebug_dump_frame (frame));
		reload_user_segregs ();
		frame->ds = frame->es = (frame->cs & 0xffff) == X86_UCS 
		    ? X86_UDS : X86_KDS;
		frame->es = X86_UDS;
		return;
	    }
	}

#if defined(CONFIG_ENABLE_SMALL_AS)
# if defined(CONFIG_IA32_FEATURE_SEP)
	/*
	 * Check if we caught an exception when doing the LRET.
	 */
	dword_t user_eip = get_user_ip(current);

	if ( user_eip >= (dword_t) sysexit_tramp &&
	     user_eip <	 (dword_t) reenter_sysexit )
	{
	    /*
	     * If we faulted at the LRET instruction (i.e., still in
	     * user level) we must IRET to the kernel due to the user
	     * space code segment limitation.  We must also disable
	     * interrupts since we can not be allowed to be preempted
	     * in the reenter-trampoline.
	     */
	    frame->cs = X86_KCS;
	    frame->eflags &= ~X86_EFL_IF;
	    frame->ecx = (dword_t) get_user_sp(current);
	    frame->fault_address = (dword_t) reenter_sysexit;
	    return;
	}
# endif /* defined(CONFIG_IA32_FEATURE_SEP) */

#endif

	
	/* Do we have to emulate/virtualize something? Analyze the
	 * instruction that caused the exception. The function should
	 * return TRUE if no further processing is required.  */
	if (handle_faulting_instruction(current, frame) == TRUE)
	    return;

	/*
	 * IDT emulation.
	 */

	if ( (frame->error_code & 2) && (current->excpt) )
	{
	    TRACEPOINT(IDT_FAULT,
		       printf("IDT fault: tcb=%p  excpt=%d\n",
			      current, frame->error_code >> 3));

	    /*
	     * Validate that we can safely push an iframe on the user
	     * stack.  Current->excpt is already checked during
	     * emulation of the LIDT instruction.
	     */

	    if ( (dword_t) frame->user_stack > USER_AREA_MAX ||
		 (dword_t) frame->user_stack <= 4 * sizeof(dword_t) )
	    {
		enter_kdebug("IDT emulation user stack out of bounds");
		goto Unhandled_fault;
	    }

	    /*
	     * Validate that we do not access user memory outside of
	     * the small space boundaries.
	     */

	    dword_t offset = 0;
#if defined(CONFIG_ENABLE_SMALL_AS)
	    smallid_t sid = current->space->smallid ();
	    if (sid.is_valid ())
	    {
		if ( ((dword_t) frame->user_stack <= sid.bytesize ()) &&
		     ((dword_t) (current->excpt + (13 * 8) + 4) + 4 <=
		      sid.bytesize ()) )
		{
		    offset = sid.offset ();
		}
		else
		    small_space_to_large (current->space);
	    }
#endif

	    /*
	     * Grab the fault handler location from the thread's IDT.
	     */

	    dword_t *ex = (dword_t *) (current->excpt + offset + (13*8));
	    dword_t fault_handler = (ex[0] & 0xffff) | (ex[1] & 0xffff0000);

	    /*
	     * Create a fake fault frame and trap into the fault handler.
	     */

	    dword_t *usp = frame->user_stack + (offset / sizeof(dword_t));
	    *(--usp) = frame->eflags;
	    *(--usp) = frame->cs;
	    *(--usp) = frame->fault_address;
	    *(--usp) = frame->error_code;
	    frame->fault_address = fault_handler;
	    frame->user_stack = usp - (offset / sizeof(dword_t));
	    return;
	}
    }


    /*
    **
    ** Processor Extension not Available
    **
    */
    if (frame->fault_code == 7)
	if (handle_nm_exception(current) == TRUE)
	    return;



#if defined(CONFIG_ENABLE_SMALL_AS)
    if ( frame->fault_code == 12 || frame->fault_code == 13 )
    {
	/*
	 * If a GP(0) or SS(0) is raised it might happen to be a small
	 * address space accessing an address outside of its segment
	 * boundaries.
	 */
	if (frame->error_code == 0 && current->space->is_small ())
	{
	    small_space_to_large (current->space);
	    return;
	}
    }
#endif

Unhandled_fault:

    printf("exception (%d)\n", frame->fault_code);
    kdebug_dump_frame(frame);
    enter_kdebug();
}


#if defined(CONFIG_ENABLE_PVI)

void handle_pending_irq(void){

    tcb_t *current = get_current_tcb();
    tcb_t *max_priority = current;
    tcb_t *owner;
	
    // printf("handle_pending_irq %x\n", pending_irq);
	

    for (dword_t num = 0; num < MAX_PENDING_IRQS; num++){
	    
	if (pending_irq & PENDING_IRQ_BIT(num)){
	    // printf("handle_pending_irq number %x\n", num);
			
	    owner = interrupt_owner[num];
			
	    /* if no one is associated - we simply drop the interrupt */
	    if (owner)
	    {
		if (IS_WAITING(owner) && 
		    ((owner->partner.raw == (num + 1)) || l4_is_nil_id(owner->partner)))
		{
		    owner->intr_pending = (num + 1) | IRQ_IN_SERVICE;
		    // printf("enqueue ready owner %x, me is %x\n", owner->myself.raw, current->myself.raw);
		    thread_enqueue_ready(owner);
		    owner->thread_state = TS_LOCKED_RUNNING;
				    
		    if (max_priority->priority < owner->priority)
			max_priority = owner;
				    
		}
		else 
		    owner->intr_pending = num + 1;
			
	    }
	}
    }
	
    if (current != get_idle_tcb()){
	// printf("enqueue ready current %x\n", current->myself.raw);
	current->thread_state = TS_RUNNING;
	thread_enqueue_ready(current);
    } 

    if (pending_irq & PENDING_TIMER_IRQ){
	// printf("handle pending timer interrupt; max_priority = %x, current = %x, resources=%x\n", max_priority->myself.raw, current->myself.raw, current->resources);
	
	enable_8259A_irq(8);
	pending_irq = 0;
	    
#if (defined(CONFIG_DEBUGGER_KDB) || defined(CONFIG_DEBUGGER_GDB)) && defined(CONFIG_DEBUG_BREAKIN)
	kdebug_check_breakin();
#endif
	    
#if defined(CONFIG_SMP)
	/* only cpu 0 maintains the clock */
	if (get_cpu_id() == 0)
#endif
	    kernel_info_page.clock += TIME_QUANTUM;
	    
	current->current_timeslice -= TIME_QUANTUM;
	    
	if (max_priority == current){
	    tcb_t * wakeup = parse_wakeup_queue(current->priority, kernel_info_page.clock);
	    if (wakeup)
	    {
		dispatch_thread(wakeup);
		return;
	    }
		
	    if (current->current_timeslice <= 0)
	    {
		spin1(78);
		current->current_timeslice = current->timeslice;
		dispatch_thread(find_next_thread());
	    }
		    
	    return;
	}
    }
	
    pending_irq = 0;
	
    if (max_priority != current){
	/* make sure runnable threads are in the ready queue */
	switch_to_thread(max_priority, current);
    }
	
}
#endif /* CONFIG_ENABLE_PVI */


