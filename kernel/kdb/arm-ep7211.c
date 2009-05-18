/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm-ep7211.c
 * Description:   Kdebug interface for the EP7211.
 *                
 * @LICENSE@
 *                
 * $Id: arm-ep7211.c,v 1.7 2001/12/11 23:11:50 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>

#define HWBASE		0xFFF00000

#define HWBASE1		(HWBASE + 0x00000000)
#define HWBASE2		(HWBASE + 0x00001000)

#define DATA(x)		(*(volatile dword_t*) (((x)+0x480)))
#define STATUS(x)	(*(volatile dword_t*) (((x)+0x140)))

#define UTXFF		(1 << 23)
#define URXFE		(1 << 22)

#if defined(CONFIG_DEBUGGER_IO_OUTCOM)
void putc(char c)
{
    while (STATUS(HWBASE2) & UTXFF);
    DATA(HWBASE2) = c;
    
    if (c == '\n')
	putc('\r');
}
#endif /* defined(CONFIG_DEBUGGER_IO_OUTCOM) */


#if defined(CONFIG_DEBUGGER_IO_INCOM)
char getc(void)
{
  while (STATUS(HWBASE2) & URXFE);
  return DATA(HWBASE2);
};
#endif /* defined(CONFIG_DEBUGGER_IO_INCOM) */

#if defined(CONFIG_DEBUG_BREAKIN)
void kdebug_check_breakin()
{
#if defined(CONFIG_DEBUGGER_IO_INCOM)
    if (!(STATUS(HWBASE2) & URXFE))
    {
	if ((DATA(HWBASE2) & 0xFF) == 0x1b)
	    enter_kdebug("breakin");
    }
#endif
}
#endif



#if defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM)
#define RATE		CONFIG_DEBUGGER_COMSPEED

void kdebug_init_serial(void)
{
    printf("%s\n", __FUNCTION__);
#if 0

#define PRTEN   1<<13
#define EVENPTR 1<<14
#define XSTOP   1<<15
#define FIFOEN  1<<16
#define WRDLEN  3<<17     /* 8 Bit word length*/

    /* enabling Serial port 2 */
    dword_t 
    dword_t t=indw(SYSCON2+IO_VBASE);
    t=t|(1<<2);
    outdw(SYSCON2, t);

    /* programming bit rate and line ctl + FIFO disabled */
    dword_t p=(RATE/115200)|WRDLEN;
    outdw(COM2_BASE+IO_VBASE, p);

#endif
    TRACE();
}


#endif /* defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM) */


void kdebug_hwreset()
{
    panic(__FUNCTION__);
}

