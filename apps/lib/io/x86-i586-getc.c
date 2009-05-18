/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/x86-i586-getc.c
 * Description:   keyboard getc() for x86-based PCs
 *                
 * @LICENSE@
 *                
 * $Id: x86-i586-getc.c,v 1.3 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/

#include <l4/l4.h>

/* No SHIFT key support!!! */

#define INLINE static inline

INLINE unsigned char inb(dword_t port)
{
    unsigned char tmp;
    __asm__ __volatile__("inb %1, %0\n"
			 : "=al"(tmp)
			 : "dN"(port));
    return tmp;
};

#define KBD_STATUS_REG		0x64	
#define KBD_CNTL_REG		0x64	
#define KBD_DATA_REG		0x60	

#define KBD_STAT_OBF 		0x01	/* Keyboard output buffer full */

#define kbd_read_input() inb(KBD_DATA_REG)
#define kbd_read_status() inb(KBD_STATUS_REG)

static unsigned char keyb_layout[128] =
	"\000\0331234567890-+\177\t"			/* 0x00 - 0x0f */
	"qwertyuiop[]\r\000as"				/* 0x10 - 0x1f */
	"dfghjkl;'`\000\\zxcv"				/* 0x20 - 0x2f */
	"bnm,./\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
	"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
	"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
	"\r\000/";					/* 0x60 - 0x6f */


char getc()
{
    static unsigned char last_key = 0;
    char c;
    while(1) {
	unsigned char status = kbd_read_status();
	while (status & KBD_STAT_OBF) {
	    unsigned char scancode;
	    scancode = kbd_read_input();
	    if (scancode & 0x80)
		last_key = 0;
	    else if (last_key != scancode)
	    {
		//printf("kbd: %d, %d, %c\n", scancode, last_key, keyb_layout[scancode]);
		last_key = scancode;
		c = keyb_layout[scancode];
		if (c > 0) return c;
	    }
	}
    }
}
