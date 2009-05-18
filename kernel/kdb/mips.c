/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips.c
 * Description:   MIPS specific kdebug stuff.
 *                
 * @LICENSE@
 *                
 * $Id: mips.c,v 1.5 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_ARCH(cpu.h)


#if defined(CONFIG_DEBUGGER_KDB)
int kdebug_arch_entry(exception_frame_t* frame)
{
    return 0;
}

void kdebug_arch_exit(exception_frame_t* frame)
{
    return;
};

void kdebug_dump_pgtable(ptr_t pgtable)
{
    
}

void kdebug_dump_frame(exception_frame_t *frame)
{
}


void kdebug_pf_tracing(int state)
{
    extern int __kdebug_pf_tracing;
    __kdebug_pf_tracing = state;
}

void kdebug_single_stepping(exception_frame_t *frame, dword_t onoff)
{
}

void kdebug_breakpoint()
{
}

void kdebug_arch_help()
{
}

void kdebug_check_breakin()
{

}

void kdebug_perfmon()
{

}


#endif

