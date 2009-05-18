/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     universe.h
 * Description:   File including all include files.  Using this scheme
 *                we only need to include a single file.  If we touch
 *                a single include file, however, we need to recompile
 *                the whole kernel, but since the kernel is small (in
 *                contrast to, e.g., Linux) we can live with that.
 *                
 * @LICENSE@
 *                
 * $Id: universe.h,v 1.17 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __UNIVERSE_H__
#define __UNIVERSE_H__

/* the file created by the xconfig tool */
#include <config.h>

#include <macros.h>
#include <types.h>

// kip and kernel identification
#include <kernel.h>

/* some of the include files rely on 
 * architecture specific configuration */
#include INC_ARCH(config.h)

#include <lib.h>
#include <sync.h>

/* some of the macros of the notify mechanism are used to
   create function prototypes */
#include INC_ARCH(notify.h)

#include <kmemory.h>
#include <thread.h>
#include <ipc.h>
#include <tcb.h>
#include <schedule.h>
#include <interrupt.h>

#include INC_ARCH(cpu.h)
#include INC_ARCH(memory.h)
#include INC_ARCH(thread.h)


#if defined(CONFIG_SMP)
# include INC_ARCH(smp.h)
#endif

#include <kdebug.h>

#if defined(CONFIG_DEBUG)
# define ASSERT(x) do { if (!(x)) {printf("ASSERT(%s) failed in %s, file: %s:%d\n", #x, __FUNCTION__, __FILE__, __LINE__); enter_kdebug("ASSERT FAILED"); while(1);} } while (0)
#else
# define ASSERT(x) do { } while (0)
#endif
#endif /* __UNIVERSE_H__ */
