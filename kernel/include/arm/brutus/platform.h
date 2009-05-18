/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/brutus/platform.h
 * Description:   Definitions specific to the Brutus platform.
 *                
 * @LICENSE@
 *                
 * $Id: platform.h,v 1.12 2001/12/12 22:28:03 ud3 Exp $
 *                
 ********************************************************************/
#ifndef L4__ARM__BRUTUS__PLATFORM_H
#define L4__ARM__BRUTUS__PLATFORM_H


/* scheduler configuration */
/* 10 ms = 1 timeslice */
#define DEFAULT_TIMESLICE	(10 * 1024)
/* 2 ms per tick */
#define TIME_QUANTUM		(2 * 1024)





#define angel_SWIreason_EnterSVC 0x17   /* from arm.h, in angel source */
#define angel_SWI_ARM (0xEF123456 & 0xffffff)
 
/* Virtual Addresses */

#define INTCTRL_VBASE	0xFFF00000
#define OSTIMER_VBASE	0xFFF10000
#define UART_VBASE	0xFFF20000
#define GPIO_VBASE	0xFFF30000
#define RSTCTL_VBASE	0xFFF40000

/* Physical Addresses */

#define INTCTRL_PBASE	0x90050000
#define OSTIMER_PBASE	0x90000000
#define UART_PBASE	0x80050000
#define GPIO_PBASE	0x90040000
#define RSTCTL_PBASE	0x90030000

/* base address */
#define DRAM_BASE		0xC0000000U
/* log2 of the populated size per bank */
#define DRAM_BANKSIZE_BITS	22	/* 4M    */
/* log2 of the bank distance */
#define DRAM_BANKDIST_BITS	27	/* 128M  */

#define DRAM_BANKSIZE		(1 << DRAM_BANKSIZE_BITS)
#define DRAM_BANKDIST		(1 << DRAM_BANKDIST_BITS)

/* overall DRAM memory size */
#define DRAM_SIZE    (4*DRAM_BANKSIZE)	/* 16M */



/*
 * Interrupt masks
 */
#define INTR_RTC	(1<<31)
#define INTR_1HZ_CLOCK	(1<<30)
#define INTR_OSTIMER3	(1<<29)
#define INTR_OSTIMER2	(1<<28)
#define INTR_OSTIMER1	(1<<27)
#define INTR_OSTIMER0	(1<<26)

/*
 * Interrupt controller registers.
 */
#define ICIP *((volatile dword_t *) (INTCTRL_VBASE + 0x00)) /* Pending IRQ */
#define ICMR *((volatile dword_t *) (INTCTRL_VBASE + 0x04)) /* Mask register */
#define ICLR *((volatile dword_t *) (INTCTRL_VBASE + 0x08)) /* Level reg. */
#define ICFP *((volatile dword_t *) (INTCTRL_VBASE + 0x10)) /* Pending FIQs */
#define ICPR *((volatile dword_t *) (INTCTRL_VBASE + 0x20)) /* Pending intr. */
#define ICCR *((volatile dword_t *) (INTCTRL_VBASE + 0x0c)) /* Control reg. */

/*
 * OS Timer registers.
 */
#define OSMR0 *((volatile dword_t *) (OSTIMER_VBASE + 0x00)) /* Match reg 0 */
#define OSMR1 *((volatile dword_t *) (OSTIMER_VBASE + 0x04)) /* Match reg 1 */
#define OSMR2 *((volatile dword_t *) (OSTIMER_VBASE + 0x08)) /* Match reg 2 */
#define OSMR3 *((volatile dword_t *) (OSTIMER_VBASE + 0x0c)) /* Match reg 3 */
#define OSCR  *((volatile dword_t *) (OSTIMER_VBASE + 0x10)) /* Count reg. */
#define OSSR  *((volatile dword_t *) (OSTIMER_VBASE + 0x14)) /* Status reg. */
#define OWER  *((volatile dword_t *) (OSTIMER_VBASE + 0x18)) /* Watchdg enbl */
#define OIER  *((volatile dword_t *) (OSTIMER_VBASE + 0x1c)) /* Intr. enable */


/*
 * Number of bytes in a cache line.
 */
#define CACHE_LINE_SIZE	32


/*
 * For aligning structures on cache line boundaries.
 */
#define CACHE_ALIGN __attribute__ ((aligned (CACHE_LINE_SIZE)))


#endif /* !L4__ARM__BRUTUS__PLATFORM_H */
