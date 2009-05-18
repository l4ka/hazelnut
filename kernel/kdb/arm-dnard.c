/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm-dnard.c
 * Description:   Kdebug interface for the Dnard.
 *                
 * @LICENSE@
 *                
 * $Id: arm-dnard.c,v 1.7 2001/12/13 12:26:37 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM)
#define COMPORT		CONFIG_DEBUGGER_COMPORT
#define RATE		CONFIG_DEBUGGER_COMSPEED

#define DATA(x)		(*(volatile byte_t*) ((x)))
#define STATUS(x)	(*(volatile byte_t*) ((x)+5))


void kdebug_init_serial()
{

    printf("%s(%x,%d)\n", __FUNCTION__, COMPORT, RATE);

#define IER	(COMPORT+1)
#define EIR	(COMPORT+2)
#define LCR	(COMPORT+3)
#define MCR	(COMPORT+4)
#define LSR	(COMPORT+5)
#define MSR	(COMPORT+6)
#define DLLO	(COMPORT+0)
#define DLHI	(COMPORT+1)

    outb(LCR, 0x80);		/* select bank 1	*/
    outb(DLLO, (((115200/RATE) >> 0) & 0x00FF));
    outb(DLHI, (((115200/RATE) >> 8) & 0x00FF));
    outb(LCR, 0x03);		/* set 8,N,1		*/
    for (volatile int i = 10000; i--; );
    outb(EIR, 0x07);		/* enable FIFOs	*/
    outb(IER, 0x00);		/* enable RX interrupts	*/

    inb(IER);
    inb(EIR);
    inb(LCR);
    inb(MCR);
    inb(LSR);
    inb(MSR);
}
#endif

#if defined(CONFIG_DEBUGGER_IO_OUTCOM)
void putc(char c)
{
    while (!(inb(COMPORT+5) & 0x60));
    outb(COMPORT,c);

    if (c == '\n')
	putc('\r');
};
#endif

#if defined(CONFIG_DEBUGGER_IO_INCOM)
char getc(void)
{
  while (!(inb(COMPORT+5) & 0x01));
  return inb(COMPORT);
};
#endif

#if defined(CONFIG_DEBUG_BREAKIN)
void kdebug_check_breakin()
{
    if ((inb(COMPORT+5) & 0x01))
    {
	if (inb(COMPORT) == 0x1b)
	    enter_kdebug("breakin");
    }
}
#endif

void kdebug_hwreset()
{
    outw(0x24, 0x09);
    outw(0x26, inw(0x26) | (1<<3) | (1<<10));
    /* it will take a while */
    while (1);
}

