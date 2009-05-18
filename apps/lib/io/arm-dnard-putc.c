/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-dnard-putc.c
 * Description:   serial putc() for StrongARM 110 based DNARD board
 *                
 * @LICENSE@
 *                
 * $Id: arm-dnard-putc.c,v 1.9 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>

#include "arm-dnard-uart.h"

void putc(int c)
{
    while (!(STATUS(COM1) & 0x60));
    DATA(COM1) = c;
    
    if (c == '\n')
	putc('\r');
};

