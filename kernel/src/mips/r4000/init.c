/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/r4000/init.c
 * Description:   MIPS R4000 initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.2 2001/11/22 13:27:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

void init_platform()
{
    struct linux_promblock *pb;

    romvec = ROMVECTOR;
    pb = sgi_pblock = PROMBLOCK;
    
    if(pb->magic != 0x53435241) {
	panic("bad prom vector magic");
    }

    prom_vers = pb->ver;
    prom_rev = pb->rev;
}
