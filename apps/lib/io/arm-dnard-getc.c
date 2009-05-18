/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/arm-dnard-getc.c
 * Description:   serial getc() for StrongARM 110 based DNARD board
 *                
 * @LICENSE@
 *                
 * $Id: arm-dnard-getc.c,v 1.7 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>

#include "arm-dnard-uart.h"

int getc(void)
{
  while (!(STATUS(COM1) & 0x01));
  return DATA(COM1);
};


