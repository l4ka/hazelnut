/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     sync.h
 * Description:   Basic definitions for synchronization needed for SMP
 *                systemss (null-stubs if not used in an SMP
 *                environment).
 *                
 * @LICENSE@
 *                
 * $Id: sync.h,v 1.3 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __SYNC_H__
#define __SYNC_H__


#ifndef CONFIG_SMP

#define DEFINE_SPINLOCK(name)
#define spin_lock_init(x)	do { } while(0)
#define spin_lock(x)		do { } while(0)
#define spin_unlock(x)		do { } while(0)

#else /* CONFIG_SMP */
/* 
 * for smp we need a platform specific file 
 * herewith we avoid rewriting empty spinlocks 
 * for each new platform again and again and again.
 */

#include INC_ARCH(sync.h)

#endif /* CONFIG_SMP */

#endif
