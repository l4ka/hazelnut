/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-brutus-putc.c
 * Description:   serial putc() for StrongARM 1100 based Brutus board
 *                
 * @LICENSE@
 *                
 * $Id: arm-brutus-putc.c,v 1.7 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include "brutus-uart.h"

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
