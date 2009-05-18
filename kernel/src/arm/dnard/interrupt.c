/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/dnard/interrupt.c
 * Description:   Interruppt handling code for the Dnard.
 *                
 * @LICENSE@
 *                
 * $Id: interrupt.c,v 1.8 2001/11/22 13:18:49 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include INC_ARCH(farcalls.h)

#include INC_PLATFORM(platform.h)


void init_irqs() L4_SECT_INIT;

/*
 * This contains the irq mask for both 8259A irq controllers,
 */
static unsigned int cached_irq_mask = 0x0000;

#define __byte(x,y) 	(((unsigned char *)&(y))[x])
#if 1
#define cached_21	(__byte(0,cached_irq_mask))
#define cached_A1	(__byte(1,cached_irq_mask))
#else
#define cached_21	inb(0x21)
#define cached_A1	inb(0xA1)
#endif
static void disable_8259A_irq(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	cached_irq_mask |= mask;
	if (irq & 8) {
		outb(0xA1, cached_A1);
	} else {
		outb(0x21, cached_21);
	}
}

static void enable_8259A_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);
	cached_irq_mask &= mask;
	if (irq & 8) {
		outb(0xA1, cached_A1);
	} else {
		outb(0x21, cached_21);
	}
}

static int i8259A_irq_pending(unsigned int irq)
{
	unsigned int mask = 1<<irq;

	if (irq < 8)
                return (inb(0x20) & mask);
        return (inb(0xA0) & (mask >> 8));
}

/*
 * Careful! The 8259A is a fragile beast, it pretty
 * much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI
 * to the two 8259s is important!
 */
static void mask_and_ack_8259A(unsigned int irq)
{
	cached_irq_mask |= 1 << irq;
	if (irq & 8) {
		inb(0xA1);	/* DUMMY */
		outb(0xA1, cached_A1);
		outb(0x20, 0x62);	/* Specific EOI to cascade */
		outb(0xA0, 0x20);
	} else {
		inb(0x21);	/* DUMMY */
		outb(0x21, cached_21);
		outb(0x20, 0x20);
	}
}

void mask_interrupt(dword_t irq)
{
#warning: HACK
    mask_and_ack_8259A(irq);
}

void unmask_interrupt(unsigned int number)
{
    // do nothing - no automatic irq acking in pic mode
}






void init_irqs()
{
    printf("%s\n", __FUNCTION__);
    
    volatile char* x;
    
#if 1	/* enable all irqs */
    x = (char*) 0xFFF00021; *x = cached_21;
    x = (char*) 0xFFF000A1; *x = cached_A1;
#endif    
    x = (char*) 0xFFF00020; *x = 0x0A;
    x = (char*) 0xFFF000A0; *x = 0x0A;
    
}



void irq_handler(dword_t spsr)
{
//    printf("%s - spsr=%x tcb=%p\n", __FUNCTION__, spsr, get_current_tcb());

    for (int i=0; i < 16; i++)
	if ((i != 2) && !(cached_irq_mask & (1 << i)) && i8259A_irq_pending(i))
	{
//	    printf(" IRQ %d pending ", i);
#warning REVIEWME: we should not ack here, should we?
	    mask_and_ack_8259A(i);
#if defined(CONFIG_DEBUG_TRACE_IRQS)
	    putc('0'+i);
#endif
	    //printf(" masked and ack'd\n");
	    

#if 0
	    printf("\nIER=%x EIR=%x, DATA=%c MSR=%x PIC %x%x   ",
		   inb(0x3F9), inb(0x3FA), inb(0x3F8), inb(0x3FE),
		   inb(0xA0), inb(0x20));
#endif
	    if (i == 8) /* timer interrupt ??? */
	    {
		/* reset timer intr line */
		in_rtc(0x0c);
		/* enable the irq here before we eventually
		   loose the CPU in handle_timer_interrupt */
		enable_8259A_irq(i);
		handle_timer_interrupt();
//	printf("irq: cached=%x, current %x:%x\n", cached_irq_mask, inb(0x21), inb(0xA1));
	    };

	};

//    panic("foofoo");
    return;
}
