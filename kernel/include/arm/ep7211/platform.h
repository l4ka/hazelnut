/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/ep7211/platform.h
 * Description:   Definitions specific to the EP7211 platform.
 *                
 * @LICENSE@
 *                
 * $Id: platform.h,v 1.9 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__EP7211__PLATFORM_H__
#define __ARM__EP7211__PLATFORM_H__


/* scheduler configuration */
/* 10 ms = 1 timeslice */
#define DEFAULT_TIMESLICE	(15 * 1024)
/* 2 ms per tick */
#define TIME_QUANTUM		(15 * 1024)


/*
 * Physical memory description

        |            |
        +------------+
        |   empty    |
        +------------+
        |   DRAM3    |
        +------------+
        |   empty    |
        +------------+
        |   DRAM2    |
        +------------+
        |   empty    |
        +------------+
        |   DRAM1    |
        +------------+ DRAM_BASE + DRAM_BANKDIST
        |   empty    |
        +------------+ DRAM_BASE + DRAM_BANKSIZE
        |   DRAM0    |
        +------------+ DRAM_BASE
        |            |
*/

/* base address */
#define DRAM_BASE		0xC0000000U
/* log2 of the populated size per bank */
#define DRAM_BANKSIZE_BITS	23	/* 8M    */
/* log2 of the bank distance */
#define DRAM_BANKDIST_BITS	28	/* 256M  */

#define DRAM_BANKSIZE		(1 << DRAM_BANKSIZE_BITS)
#define DRAM_BANKDIST		(1 << DRAM_BANKDIST_BITS)

/* overall DRAM memory size */
#define DRAM_SIZE    (1*DRAM_BANKSIZE)	/* 8M ??? */



#define IO_PBASE	0x80000000U
#define IO_VBASE	0xFFF00000

#define COM1_BASE	0x00000480
#define COM2_BASE       0x00001480
#define SYSFLG          0x00000140  /*System status registers*/
#define SYSFLG2         0x00001140
#define SYSCON          0x00000100  /*System control registers*/
#define SYSCON2         0x00001100
#define SYSCON3         0x00002200

#ifndef ASSEMBLY
static inline void outb(dword_t port, char val)
{
//    printf("%s(%x,%x)\n", __FUNCTION__, port, val);
    __asm__("@ outb >>>>>");
    *(volatile char*) (IO_VBASE + port) = val;
    __asm__("@ outb <<<<< ");
}

static inline char inb(dword_t port)
{
    char t;
    __asm__("@ inb >>>>>");
    t = *(volatile char*) (IO_VBASE + port);
    __asm__("@ inb <<<<< ");
//    printf("%s(%x)=%x\n", __FUNCTION__, port, t);
    return t;
}

static inline void outdw(dword_t port, dword_t val)
{
//    printf("%s(%x,%x)\n", __FUNCTION__, port, val);
    __asm__("@ outw >>>>>");
    *(volatile dword_t*) (IO_VBASE + port) = val;
    __asm__("@ outw <<<<< ");
}

static inline dword_t indw(dword_t port)
{
    dword_t t;
    __asm__("@ inw >>>>>");
    t = *(volatile dword_t*) (IO_VBASE + port);
    __asm__("@ inw <<<<< ");
//    printf("%s(%x)=%x\n", __FUNCTION__, port, t);
    return t;
}

#define RTC_MATCH	0x3C0     //generate interrupt if DATA=MATCH
#define RTC_DATA	0x380     //RTC data 32 bit, direkt R/W

/* hier wird es noch krachen
static inline void out_rtc(dword_t reg, dword_t val)
{
//    printf("%s(%x,%x)\n", __FUNCTION__, reg, val);
    __asm__("@ out_rtc >>>>>");
    *(volatile dword_t*) (IO_VBASE + RTC_DATA) = val;
    __asm__("@ out_rtc <<<<< ");
}

static inline char in_rtc(dword_t reg)
{
    char t; 
    __asm__("@ in_rtc >>>>>");
    t = *(volatile dword_t*) (IO_VBASE + RTC_DATA);
    __asm__("@ in_rtc <<<<< ");
//    printf("%s(%x)=%x\n", __FUNCTION__, reg, t);
    return t;
}

*/
#endif /* ASSEMBLY */

#endif /* __ARM__EP7211__PLATFORM_H__ */

