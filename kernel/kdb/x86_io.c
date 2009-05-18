/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86_io.c
 * Description:   Various low level I/O functions for x86.
 *                
 * @LICENSE@
 *                
 * $Id: x86_io.c,v 1.4 2001/12/04 16:56:26 uhlig Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_ARCH(cpu.h)
#include "kdebug.h"

/*
**
** Console I/O functions.
**
*/

#if defined(CONFIG_DEBUGGER_IO_OUTSCRN)
# if defined(CONFIG_DEBUGGER_IO_SCREEN_VGA)
#  define DISPLAY ((char*)KERNEL_VIDEO)
#  define NUM_LINES 25
# else
#  define DISPLAY ((char*)KERNEL_VIDEO_HERC)
#  define NUM_LINES 24
# endif
#define COLOR 10
static unsigned __cursor = 0;

void putc(char c)
{
    switch(c) {
    case '\r':
        __cursor -= (__cursor % 160);
	break;
    case '\n':
	__cursor += (160 - (__cursor % 160));
	break;
    case '\t':
	__cursor += (8 - (__cursor % 8));
	break;
    default:
	DISPLAY[__cursor++] = c;
	DISPLAY[__cursor++] = COLOR;
    }

    if ((__cursor / 160) == NUM_LINES) {
	for (int i = 40; i < 40*NUM_LINES; i++)
	    ((dword_t*)DISPLAY)[i - 40] = ((dword_t*)DISPLAY)[i];
	for (int i = 0; i < 40; i++)
	    ((dword_t*)DISPLAY)[40*(NUM_LINES-1) + i] = 0;
	__cursor -= 160;
    }
}
#endif /* CONFIG_DEBUGGER_IO_OUTSCRN */


#if defined(CONFIG_DEBUGGER_IO_INKBD)

#define KBD_STATUS_REG		0x64	
#define KBD_CNTL_REG		0x64	
#define KBD_DATA_REG		0x60	

#define KBD_STAT_OBF 		0x01	/* Keyboard output buffer full */

#define kbd_read_input() inb(KBD_DATA_REG)
#define kbd_read_status() inb(KBD_STATUS_REG)

static unsigned char keyb_layout[2][128] =
{
	"\000\0331234567890-=\177\t"			/* 0x00 - 0x0f */
	"qwertyuiop[]\r\000as"				/* 0x10 - 0x1f */
	"dfghjkl;'`\000\\zxcv"				/* 0x20 - 0x2f */
	"bnm,./\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
	"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
	"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
	"\r\000/"					/* 0x60 - 0x6f */
	,
	"\000\033!@#$%^&*()_+\177\t"			/* 0x00 - 0x0f */
	"QWERTYUIOP{}\r\000AS"				/* 0x10 - 0x1f */
	"DFGHJKL:\"`\000\\ZXCV"				/* 0x20 - 0x2f */
	"BNM<>?\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
	"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
	"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
	"\r\000/"					/* 0x60 - 0x6f */
};

char getc()
{
    static byte_t last_key = 0;
    static byte_t shift = 0;
    char c;
    while(1) {
	unsigned char status = kbd_read_status();
	while (status & KBD_STAT_OBF) {
	    byte_t scancode;
	    scancode = kbd_read_input();
	    /* check for SHIFT-keys */
	    if (((scancode & 0x7F) == 42) || ((scancode & 0x7F) == 54))
	    {
		shift = !(scancode & 0x80);
		continue;
	    }
	    /* ignore all other RELEASED-codes */
	    if (scancode & 0x80)
		last_key = 0;
	    else if (last_key != scancode)
	    {
//		printf("kbd: %d, %d, %c\n", scancode, last_key,
//		       keyb_layout[shift][scancode]);
		last_key = scancode;
		c = keyb_layout[shift][scancode];
		if (c > 0) return c;
	    }
	}
    }
}

#endif /* CONFIG_DEBUGGER_IO_INKBD */

#if !defined(CONFIG_DEBUGGER_IO_INCOM) && defined(CONFIG_DEBUG_BREAKIN)
void kdebug_check_breakin(void)
{
}
#endif




/*
**
** Serial port I/O functions.
**
*/


#if defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM)
#define COMPORT		CONFIG_DEBUGGER_COMPORT
#define RATE		CONFIG_DEBUGGER_COMSPEED

void kdebug_init_serial(void)
{
#define IER	(COMPORT+1)
#define EIR	(COMPORT+2)
#define LCR	(COMPORT+3)
#define MCR	(COMPORT+4)
#define LSR	(COMPORT+5)
#define MSR	(COMPORT+6)
#define DLLO	(COMPORT+0)
#define DLHI	(COMPORT+1)

    outb(LCR, 0x80);		/* select bank 1	*/
    for (volatile int i = 10000000; i--; );
    outb(DLLO, (((115200/RATE) >> 0) & 0x00FF));
    outb(DLHI, (((115200/RATE) >> 8) & 0x00FF));
    outb(LCR, 0x03);		/* set 8,N,1		*/
    outb(IER, 0x00);		/* disable interrupts	*/
    outb(EIR, 0x07);		/* enable FIFOs	*/
    outb(IER, 0x01);		/* enable RX interrupts	*/
    inb(IER);
    inb(EIR);
    inb(LCR);
    inb(MCR);
    inb(LSR);
    inb(MSR);
}
#endif /* CONFIG_DEBUGGER_IO_INCOM || CONFIG_DEBUGGER_IO_OUTCOM */



#if defined(CONFIG_DEBUGGER_IO_OUTCOM)
void putc(const char c)
{
    while (!(inb(COMPORT+5) & 0x60));
    outb(COMPORT,c);
    if (c == '\n')
	putc('\r');

}
#endif /* CONFIG_DEBUGGER_IO_OUTCOM */


#if defined(CONFIG_DEBUGGER_IO_INCOM)
char getc(void)
{
    while (!(inb(COMPORT+5) & 0x01));
    return inb(COMPORT);
}

#if defined(CONFIG_DEBUG_BREAKIN)
void kdebug_check_breakin(void)
{
    if ((inb(COMPORT+5) & 0x01))
    {
	if (inb(COMPORT) == 0x1b)
	    enter_kdebug("breakin");
    }
}
#endif
#endif /* CONFIG_DEBUGGER_IO_INCOM */
