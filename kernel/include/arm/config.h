/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/config.h
 * Description:   Various kernel configuration stuff for ARM.
 *                
 * @LICENSE@
 *                
 * $Id: config.h,v 1.14 2001/11/22 15:09:12 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__CONFIG_H__
#define __ARM__CONFIG_H__

/* On ARM we need farcalls for invoking certain functions. */
#define NEED_FARCALLS		1


#include INC_PLATFORM(platform.h)

/* thread configuration */
#define TCB_AREA		0xE0000000
#define TCB_AREA_SIZE		0x01000000

#define L4_NUMBITS_TCBS		10
#define L4_NUMBITS_THREADS	22

#define L4_TOTAL_TCB_SIZE       (1 << L4_NUMBITS_TCBS)
#define L4_TCB_DATA_SIZE        88
#define L4_TCB_STACK_SIZE       (L4_TOTAL_TCB_SIZE - L4_TCB_DATA_SIZE)

#define L4_TCB_MASK		((~0) << L4_NUMBITS_TCBS)


#define KERNEL_VIRT		0xFFFF0000
#define KERNEL_SIZE		0x00010000

#define KERNEL_OFFSET		(KERNEL_VIRT - KERNEL_PHYS)

#define cpaddr	0xF0000000U

#define IN_KERNEL_AREA(x)	((x) > TCB_AREA)


/* the copy areas cover two 1st level entries */
#define MEM_COPYAREA1		0xFF800000
#define MEM_COPYAREA2		(MEM_COPYAREA1 + (PAGEDIR_SIZE * 2))
#define MEM_COPYAREA_END	(MEM_COPYAREA2 + (PAGEDIR_SIZE * 2))


#if defined(CONFIG_USE_ARCH_KMEMORY)
#define HAVE_ARCH_KMEMORY
#endif

#endif /* __ARM__CONFIG_H__ */
