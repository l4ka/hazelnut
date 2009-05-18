/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     init.c
 * Description:   Initialization functions for kdebug.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.4 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

void kdebug_init_arch();

#if defined(CONFIG_DEBUGGER_NEW_KDB)
void init_tracing();
int name_tab_index;
#endif

void kdebug_init()
{
    kdebug_init_arch();
    kernel_info_page.kdebug_exception = (void (*)()) kdebug_entry;

#if defined(CONFIG_DEBUGGER_NEW_KDB)
    //initialisation of the global variables
    name_tab_index = 0;
    init_tracing();
#endif
}
