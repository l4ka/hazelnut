/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/brutus/interrupt.c
 * Description:   Brutus specific interrupt/timer stuff.
 *                
 * @LICENSE@
 *                
 * $Id: interrupt.c,v 1.9 2001/11/22 13:18:49 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include <init.h>
#include INC_ARCH(farcalls.h)
#include INC_PLATFORM(platform.h)


void init_irqs(void) L4_SECT_INIT;
void init_timer() L4_SECT_INIT;


/*
 * Function init_irqs ()
 *
 *    Initialize interrupt handling for the SA-1100.
 *
 */
void init_irqs(void)
{
    /*
     * Set all interrupts to generate IRQs rather than FIQs.
     */
    ICLR = 0x00000000;

    /*
     * Only enabled and unmasked interrupts will bring CPU out of idle
     * mode.
     */
    ICCR = 1;

    /*
     * Acknowledge any pending interrupts.
     */
    ICIP = 0xffffffff;
    ICFP = 0xffffffff;

    /*
     * Enable interrupts from ostimer0.
     */
    ICMR = INTR_OSTIMER0;
}


/*
 * Function init_timer ()
 *
 *    Initialize the SA-1100 operating system timer.
 *
 */
void init_timer(void)
{
    /* Clear all OS-timer matches */
    OSSR = 0x0f;

    /* Acknowledge pending OS Timer 0 interrupt */
    ICIP = INTR_OSTIMER0;

    /* Enable OS Timer 0 */
    OIER = 0x01;

    OSMR0 = OSCR + (18432000 >> 2); /* Wait 5/4 sec */
}


void irq_handler(dword_t spsr)
{
    dword_t m, i, icip = ICIP;

    if ( icip & INTR_OSTIMER0 )
    {
#if defined(CONFIG_DEBUG_TRACE_IRQS)
	putc('T');
#endif

	OSMR0 = OSCR + (CLOCK_FREQUENCY/1024) * (TIME_QUANTUM/1024);

	OSSR = 0x01;
	ICIP = INTR_OSTIMER0;

    	handle_timer_interrupt();
	return;
    }

    for ( i = 31, m = 0x8000000U; (~icip & m) && m; m >>= 1, i-- )
	;

#if defined(CONFIG_DEBUG_TRACE_IRQS)
    putc('0'+i);
#endif

    if ( m == 0 )
    {
	printf("No interrupt pending\n");
	enter_kdebug();
	return;
    }

    handle_interrupt(i);
}


void mask_interrupt(dword_t num)
{
    ICMR = ICMR & ~(1 << num);
}

void unmask_interrupt(unsigned int number)
{
    // do nothing - no automatic irq acking???
}
