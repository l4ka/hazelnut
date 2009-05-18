/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-ep7211-putc.c
 * Description:   serial putc() for EP7211 board
 *                
 * @LICENSE@
 *                
 * $Id: arm-ep7211-putc.c,v 1.2 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include "arm-ep7211-uart.h"

void putc(int c)
{
    while (STATUS(HWBASE2) & UTXFF);
    DATA(HWBASE2) = c;
    
    if (c == '\n')
	putc('\r');
}
