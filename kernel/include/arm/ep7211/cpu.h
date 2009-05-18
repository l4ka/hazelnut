/*********************************************************************
 *                
 * Copyright (C) 1999, 200, 2001,  Karlsruhe University
 *                
 * File path:     arm/ep7211/cpu.h
 * Description:   CPU related declarations specific to EP7211.
 *                
 * @LICENSE@
 *                
 * $Id: cpu.h,v 1.2 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__EP7211__CPU_H__
#define __ARM__EP7211__CPU_H__

/* CP15 Flush Instruction Cache & Data Cache */ 
#define Flush_ID_Cache()						\
        do {								\
									\
	volatile dword_t* x;						\
        for (x = (dword_t*) 0xFFFFC000;					\
	     x != (dword_t*) 0x00000000;				\
	     x += 4)							\
	    __asm__ __volatile__ ("/* clean DCACHE */" : : "r"(*x));	\
									\
        __asm__ __volatile__						\
        (								\
        "mcr     p15, 0, r0, c7, c7, 0	\n"				\
        "mcr     p15, 0, r0, c7, c10, 4	\n"				\
         : : : "r0" );							\
        } while (0)

#endif /* !__ARM__EP7211__CPU_H__ */


