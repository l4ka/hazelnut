/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/x86-i586-putc.c
 * Description:   putc() for x86-based PCs, serial and screen
 *                
 * @LICENSE@
 *                
 * $Id: x86-i586-putc.c,v 1.6 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>

#if defined(CONFIG_LIBIO_OUTCOM)

#define COMPORT CONFIG_LIBIO_COMPORT

static inline byte_t inb(dword_t port)
{
    byte_t tmp;

    if (port < 0x100) /* GCC can optimize this if constant */
	__asm__ __volatile__ ("inb %w1, %0" :"=al"(tmp) :"dN"(port));
    else
	__asm__ __volatile__ ("inb %%dx, %0" :"=al"(tmp) :"d"(port));

    return tmp;
}

static inline void outb(dword_t port, byte_t val)
{
    if (port < 0x100) /* GCC can optimize this if constant */
	__asm__ __volatile__ ("outb %1, %w0" : :"dN"(port), "al"(val));
    else
	__asm__ __volatile__ ("outb %1, %%dx" : :"d"(port), "al"(val));
}


void putc(char c)
{
    while (!(inb(COMPORT+5) & 0x60));
    outb(COMPORT,c);
    if (c == '\n')
	putc('\r');
}

#else /* CONFIG_LIBIO_OUTSCRN */

#define DISPLAY ((char*)0xb8000 + 15*160)
#define COLOR 7
#define NUM_LINES 10
unsigned __cursor = 0;

void putc(char c)
{
    int i;

    switch(c) {
    case '\r':
        break;
    case '\n':
        __cursor += (160 - (__cursor % 160));
        break;
    case '\t':
	for ( i = 0; i < (8 - (__cursor % 8)); i++ )
	{
	    DISPLAY[__cursor++] = ' ';
	    DISPLAY[__cursor++] = COLOR;
	}
        break;
    default:
        DISPLAY[__cursor++] = c;
        DISPLAY[__cursor++] = COLOR;
    }
    if ((__cursor / 160) == NUM_LINES) {
	for (i = 40; i < 40*NUM_LINES; i++)
	    ((dword_t*)DISPLAY)[i - 40] = ((dword_t*)DISPLAY)[i];
	for (i = 0; i < 40; i++)
	    ((dword_t*)DISPLAY)[40*(NUM_LINES-1) + i] = 0;
	__cursor -= 160;
    }
}

#endif
