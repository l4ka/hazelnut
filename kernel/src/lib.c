/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     lib.c
 * Description:   Various helper functions.
 *                
 * @LICENSE@
 *                
 * $Id: lib.c,v 1.3 2001/11/22 13:04:52 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

void zero_memory(dword_t* dest, int len)
{
    len /= 4;
    while(len--)
	*(dest++) = 0;
}

void memcpy(dword_t* dest, dword_t * src, int len)
{
    //printf("memcpy(%p, %p, %d)\n", dest, src, len);

    len /= 4;
    while(len--)
	*(dest++) = *(src++);

}
