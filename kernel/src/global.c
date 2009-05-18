/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     global.c
 * Description:   Data structures having no other logical place.
 *                
 * @LICENSE@
 *                
 * $Id: global.c,v 1.10 2001/11/22 13:04:52 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

extern dword_t _end_text_p;
extern dword_t text_paddr;
kernel_info_page_t kernel_info_page __attribute__((aligned(PAGE_SIZE))) = 
{
    {'L','4',0xE6,'K'},				/* magic		*/
    0x4711,					/* version		*/
    {0},					/* reserved		*/
#if defined(CONFIG_DEBUGGER_KDB) || defined(CONFIG_DEBUGGER_GDB)
    kdebug_init,NULL,0,0,			/* kdebug		*/
#else
    0,0,0,0,					/* kdebug		*/
#endif
#if defined(CONFIG_SIGMA0_ENTRY)
    CONFIG_SIGMA0_SP,CONFIG_SIGMA0_IP,		/* sigma0 sp,ip		*/
#else
    0,0xFFFFFFFF,				/* sigma0 sp,ip		*/
#endif
    0,0,				

    0,0xFFFFFFFF,0,0,				/* sigma1		*/

#if defined(CONFIG_ROOTTASK_ENTRY)
    CONFIG_ROOTTASK_SP,CONFIG_ROOTTASK_IP,	/* root task sp,ip	*/
#else
    0,0xFFFFFFFF,				/* root task sp,ip	*/
#endif
    0,0,

    0,						/* L4 config		*/

    0, 0, 0,

    0, 32*1024*1024,				/* main mem		*/
    (dword_t) &text_paddr, (dword_t) &_end_text_p,/* rsvd mem0 - kcode	*/
    (32-4)*1024*1024, 32*1024*1024,		/* rsvd mem1 - kdata	*/
    640*1024, 1024*1024,			/* dedi mem0 - adapter	*/
};



