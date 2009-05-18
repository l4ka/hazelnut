/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/config.h
 * Description:   Configuration for the MIPS platform.
 *                
 * @LICENSE@
 *                
 * $Id: config.h,v 1.2 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_CONFIG_H__
#define __MIPS_CONFIG_H__

/* thread configuration */
#define TCB_AREA		0xC0000000
#define TCB_AREA_SIZE		0x01000000

#define MEM_COPYAREA1		0xC2000000
#define MEM_COPYAREA2		(MEM_COPYAREA1 + (PAGEDIR_SIZE * 2))
#define MEM_COPYAREA_END	(MEM_COPYAREA2 + (PAGEDIR_SIZE * 2))

#define L4_NUMBITS_TCBS		10
#define L4_NUMBITS_THREADS	22

#define L4_TOTAL_TCB_SIZE       (1 << L4_NUMBITS_TCBS)
#define L4_TCB_MASK		((~0) << L4_NUMBITS_TCBS)

#define EXCEPTION_VECTOR_BASE	(0xA0000000)
#define EXCEPTION_TLB_REFILL	(EXCEPTION_VECTOR_BASE + 0x000)
#define EXCEPTION_XTLB_REFILL	(EXCEPTION_VECTOR_BASE + 0x080)
#define EXCEPTION_CACHE_ERROR	(EXCEPTION_VECTOR_BASE + 0x100)
#define EXCEPTION_OTHER		(EXCEPTION_VECTOR_BASE + 0x180)
#define EXCEPTION_RESET		(0xBFC00000)


#define KERNEL_VIRT		0x80000000
#define KERNEL_PHYS		0x00000000
#define KERNEL_SIZE		0x00010000

#define KERNEL_OFFSET		0x88000000

/* scheduler configuration */
#define DEFAULT_TIMESLICE	2
#define TIME_QUANTUM		1

#endif
