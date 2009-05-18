/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-pleb-getc.c
 * Description:   serial getc() for StrongARM 1100 based PLEB
 *                
 * @LICENSE@
 *                
 * $Id: arm-pleb-getc.c,v 1.1 2001/12/27 17:07:07 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include "pleb-uart.h"


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
