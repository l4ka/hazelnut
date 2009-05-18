/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm-brutus.c
 * Description:   Kdebug interface for the Brutus.
 *                
 * @LICENSE@
 *                
 * $Id: arm-sa1100.c,v 1.9 2001/12/12 23:05:28 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_PLATFORM(platform.h)
#include INC_PLATFORM(uart.h)


#if defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM)

# define BRD L4_BAUDRATE_TO_BRD(CONFIG_DEBUGGER_COMSPEED)

void kdebug_init_serial(void)
{
    dword_t enbl = 0;

#if defined(CONFIG_DEBUGGER_IO_INCOM)
    enbl |= L4_UART_RXE;
#endif
#if defined(CONFIG_DEBUGGER_IO_OUTCOM)
    enbl |= L4_UART_TXE;
#endif

//    printf("%s(%d)\n", __FUNCTION__, CONFIG_DEBUGGER_COMSPEED);
//    for (volatile int i = 10000; i--; );

    L4_UART_UTCR3 = 0;			/* Diable UART */
    L4_UART_UTSR0 = 0xf;		/* Clear pending interrupts */

    L4_UART_UTCR0 = L4_UART_DSS;	/* No parity, 1 stop bit, 8 bit */
    L4_UART_UTCR1 = BRD >> 8;		/* Set baudrate */
    L4_UART_UTCR2 = BRD & 0xff;
    L4_UART_UTCR3 = enbl;		/* Enable UART */

//    for (volatile int i = 10000; i--; );
}
#endif


void putc(char chr)
{
    volatile dword_t tmp;

    /*
     * Wait till the transmit FIFO has a free slot.
     */
    do {
	tmp = L4_UART_UTSR1;
    } while ( !(tmp & L4_UART_TNF) );
    
    /*
     * Add the character to the transmit FIFO.
     */
    L4_UART_UTDR = (dword_t) chr;

    if (chr == '\n')
	putc('\r');
}

char getc(void)
{
    volatile dword_t tmp;

    /*
     * Wait till the receive FIFO has something in it.
     */
    do {
	tmp = L4_UART_UTSR1;
    } while ( !(tmp & L4_UART_RNE) );

    /*
     * Read a character from the receive FIFO.
     */
    return (char) L4_UART_UTDR;
}

#if defined(CONFIG_DEBUG_BREAKIN)
void kdebug_check_breakin()
{
    /* Check if the receive FIFO has something in it.	*/
    if ( L4_UART_UTSR1 & L4_UART_RNE)
	/* Check if it's an ESC.	*/
	if ((char) L4_UART_UTDR == 0x1b)
	    enter_kdebug("breakin");
}
#endif

void kdebug_hwreset()
{
    *(ptr_t) RSTCTL_VBASE = 1;
    while (1);
}


