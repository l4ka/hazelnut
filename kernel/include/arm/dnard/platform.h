/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/dnard/platform.h
 * Description:   Platform configurations for DNARD.
 *                
 * @LICENSE@
 *                
 * $Id: platform.h,v 1.12 2001/12/11 23:25:39 ud3 Exp $
 *                
 ********************************************************************/

/* this file contains ARM specific declarations */
#ifndef __ARM_DNARD_PLATFORM_H__
#define __ARM_DNARD_PLATFORM_H__


/* scheduler configuration */
/* 10 ms = 1 timeslice */
#define DEFAULT_TIMESLICE	(10 * 1024)
/* 2 ms per tick */
#define TIME_QUANTUM		(2 * 1024)




#define IO_PBASE	0x40000000
#define IO_VBASE	0xFFF00000

/* base address */
#define DRAM_BASE		0x08000000U
/* log2 of the populated size per bank */
#define DRAM_BANKSIZE_BITS	23	/* 8M    */
/* log2 of the bank distance */
#define DRAM_BANKDIST_BITS	25	/* 32M  */

#define DRAM_BANKSIZE		(1 << DRAM_BANKSIZE_BITS)
#define DRAM_BANKDIST		(1 << DRAM_BANKDIST_BITS)

/* overall DRAM memory size */
#define DRAM_SIZE    (4*DRAM_BANKSIZE)	/* 32M */



#define COM1_BASE	0x000003F8


#define RTC_REG		0x70
#define RTC_DATA	0x71



#ifndef ASSEMBLY
INLINE void outb(dword_t port, byte_t val)
{
    __asm__("@ outb <<<<<");
    *(volatile byte_t*) (IO_VBASE + port) = val;
    __asm__("@ outb >>>>>");
}

INLINE byte_t inb(dword_t port)
{
    __asm__("@ inb <<<<<");
    return *(volatile byte_t*) (IO_VBASE + port);
    __asm__("@ inb >>>>>");
}

INLINE void outw(dword_t port, word_t val)
{
    __asm__("@ outw <<<<<");
    *(volatile word_t*) (IO_VBASE + port) = val;
    __asm__("@ outw >>>>>");
}

INLINE word_t inw(dword_t port)
{
    __asm__("@ inw <<<<<");
    return *(volatile word_t*) (IO_VBASE + port);
    __asm__("@ inw >>>>>");
}

INLINE void out_rtc(dword_t reg, byte_t val)
{
    __asm__("@ out_rtc <<<<<");
    *(volatile byte_t*) (IO_VBASE + RTC_REG) = reg;
    __asm__("@ out_rtc -----");
    *(volatile byte_t*) (IO_VBASE + RTC_DATA) = val;
    __asm__("@ out_rtc >>>>>>");
}

INLINE byte_t in_rtc(dword_t reg)
{
    __asm__("@ in_rtc <<<<<");
    *(volatile byte_t*) (IO_VBASE + RTC_REG) = reg;
    __asm__("@ in_rtc -----");
    return *(volatile char*) (IO_VBASE + RTC_DATA);
    __asm__("@ in_rtc >>>>>");
}
#endif /* !ASSEMBLY */



#endif /* __ARM_DNARD_PLATFORM_H__ */
