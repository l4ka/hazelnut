/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-dnard-uart.h
 * Description:   Macros for serial ports on DNARD
 *                
 * @LICENSE@
 *                
 * $Id: arm-dnard-uart.h,v 1.2 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/

#if defined(NATIVE)
#define COM1		0x400003f8
#define COM2		0x400002f8
#else
#define COM1		0xFFF003f8
#define COM2		0xFFF002f8
#endif

#define DATA(x)		(*(volatile byte_t*) ((x)))
#define STATUS(x)	(*(volatile byte_t*) ((x)+5))
