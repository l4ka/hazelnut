/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/ep7211/interrupt.c
 * Description:   EP7211 specific interrupt handling code.
 *                
 * @LICENSE@
 *                
 * $Id: interrupt.c,v 1.9 2001/11/22 13:18:49 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include INC_ARCH(farcalls.h)

#include INC_PLATFORM(platform.h)


void init_irqs() L4_SECT_INIT;

void init_irqs()
{
    printf("%s\n", __FUNCTION__);
    volatile ptr_t x;

    /* Enable IRQ11, the 64Hz timer */
    x = (ptr_t) 0xFFF00280; *x = *x | (1 << 11);
}

void mask_interrupt(dword_t number)
{
    volatile ptr_t x = (ptr_t) ((IO_VBASE + 0x0240) | ((number >> 4) * 0x1000));
    *x &= ~(1 << number);
}

void unmask_interrupt(dword_t number)
{
    volatile ptr_t x = (ptr_t) ((IO_VBASE + 0x0240) | ((number >> 4) * 0x1000));
    *x |= (1 << number);
}


void irq_handler(dword_t spsr)
{
    /* INTSR3 has been omitted */
    dword_t irqs = ((((*(volatile ptr_t) 0xFFF00240) & (*(volatile ptr_t) 0xFFF00280) & 0xFFFF) <<  0) | /* INTSR1 */
		    (((*(volatile ptr_t) 0xFFF01240) & (*(volatile ptr_t) 0xFFF01280) & 0x3007) << 16)); /* INTSR2 */
		    
    for (int i=0; i < 32; i++)
	if (irqs & (1 << i))
	{
	    mask_interrupt(i);

	    if (i == 11) /* timer interrupt ??? */
	    {
#if defined(CONFIG_DEBUG_TRACE_IRQS)
		putc('T');
#endif
		/* reset timer intr line */
		(*(volatile ptr_t) 0xFFF00680) = 7;
		/* enable the irq here before we eventually
		   loose the CPU in handle_timer_interrupt */
		unmask_interrupt(i);
		handle_timer_interrupt();
	    }
	    else
	    {	       
#if defined(CONFIG_DEBUG_TRACE_IRQS)
		putc('0'+i);
#endif
		handle_interrupt(i);
	    }
	};
    return;
}
