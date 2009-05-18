/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-pleb-putc.c
 * Description:   serial putc() for StrongARM 1100 based PLEB
 *                
 * @LICENSE@
 *                
 * $Id: arm-pleb-putc.c,v 1.1 2001/12/27 17:07:07 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include "pleb-uart.h"

void putc(int c)
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
    L4_UART_UTDR = (dword_t) c;

    if ( c == '\n' )
	putc('\r');
}
