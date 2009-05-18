/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/ipaq/cpu.h
 * Description:   CPU related definitions specific to the IPaq.
 *		  based on Brutus code
 *                
 * @LICENSE@
 *                
 * $Id: cpu.h,v 1.1 2002/01/24 07:15:41 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __ARM__IPAQ__CPU_H__
#define __ARM__IPAQ__CPU_H__

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


#endif /* !__ARM__IPAQ__CPU_H__ */
