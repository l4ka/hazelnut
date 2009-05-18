/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/pr31700/init.c
 * Description:   MIPS PR31700 initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.2 2001/11/22 13:27:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include "mips/pr31700/platform.h"

void init_platform()
{
    int start = 0x80000000;
    while(1) {
	siu_out(SIU_VIDEO_CTRL_3, start);
	start+=0x20;
	//putc('a');
    };
}
