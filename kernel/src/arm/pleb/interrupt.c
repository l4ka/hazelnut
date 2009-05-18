/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     arm/pleb/interrupt.c
 * Description:   PLEB-specific interrupt/timer stuff
 *                
 * @LICENSE@
 *                
 * $Id: interrupt.c,v 1.1 2002/01/24 20:20:06 ud3 Exp $
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
    dword_t m, i, icip = ICIP, icmr = ICMR;

    /* Timer match 0 interrupt, don't care if it's masked out */
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

    /* other interrupts, must check mask register too */
    for ( i = 31, m = (1 << 31); !(m & icmr & icip) && m; m >>= 1, i-- );

#if defined(CONFIG_DEBUG_TRACE_IRQS)
    putc('\n');
    putc('0'+i);
#endif

    if ( m == 0 )
    {
	printf("No interrupt pending\n");
	enter_kdebug();
	return;
    }

    /* will be unmasked again by next ipc wait */
    mask_interrupt(i);
    
    handle_interrupt(i);
}


void mask_interrupt(dword_t num)
{
#if defined(CONFIG_DEBUG_TRACE_IRQS)
      printf("MASKing %d\n", num);
#endif

  ICMR &= ~(1 << num);
}

void unmask_interrupt(dword_t num)
{
  // do nothing - no automatic irq acking???
#if defined(CONFIG_DEBUG_TRACE_IRQS)
      printf("unmasking %d\n", num);
#endif

  ICMR |= (1 << num);
}
