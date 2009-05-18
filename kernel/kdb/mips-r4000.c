/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips-r4000.c
 * Description:   Kdebug glue for the MIPS R4000.
 *                
 * @LICENSE@
 *                
 * $Id: mips-r4000.c,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include "sgialib.h"

void kdebug_hwreset()
{
    romvec->reboot();
}

void kdebug_init_arch()
{
}

void putc(char c)
{
    unsigned long cnt;
    char it = c;

    romvec->write(1, &it, 1, &cnt);
}

char getc()
{
	unsigned long cnt;
	char c;

	romvec->read(0, &c, 1, &cnt);
	return c;
}
