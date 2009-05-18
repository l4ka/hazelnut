/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     arm/pleb/cpu.h
 * Description:   SA-1100 specific functions
 *                
 * @LICENSE@
 *                
 * $Id: cpu.h,v 1.1 2002/01/24 20:32:30 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __ARM__PLEB__CPU_H__
#define __ARM__PLEB__CPU_H__

#define DCACHE_SIZE 8192

#define Flush_ID_Cache()						\
    do {								\
        volatile dword_t *x;						\
									\
	for ( x  = (dword_t *) (0x00000000 - DCACHE_SIZE);		\
	      x != (dword_t *) 0x00000000;				\
	      x += 8)							\
	{								\
	    __asm__ __volatile__ ("/* clean DCACHE */" : : "r"(*x));	\
	}								\
									\
        __asm__ __volatile__ (						\
	    "	mcr p15, 0, r0, c7, c7, 0  @ Flush I+D		\n"	\
	    "	mcr p15, 0, r0, c7, c10, 4 @ Drain writebuf	\n"	\
	    :								\
	    :								\
	    : "r0");							\
									\
    } while (0)


#define CLOCK_FREQUENCY	3686400


#endif /* !__ARM__PLEB__CPU_H__ */
