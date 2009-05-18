/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-brutus-getc.c
 * Description:   serial getc() for StrongARM 1100 based Brutus board
 *                
 * @LICENSE@
 *                
 * $Id: arm-brutus-getc.c,v 1.5 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include "brutus-uart.h"


int getc(void)
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
    return (byte_t) ( L4_UART_UTDR );
}
