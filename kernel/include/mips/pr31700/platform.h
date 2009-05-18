/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/pr31700/platform.h
 * Description:   MIPS PR31700 platform specific definitions.
 *                
 * @LICENSE@
 *                
 * $Id: platform.h,v 1.4 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_PR31700_PLATFORM_H__
#define __MIPS_PR31700_PLATFORM_H__

/* defines system interface unit registers */
#define SIU_BASE	0x10C00000

// memory configuration
#define SIU_MEMCFG_0		0x000
#define SIU_MEMCFG_1		0x004
#define SIU_MEMCFG_2		0x008
#define SIU_MEMCFG_3		0x00C
#define SIU_MEMCFG_4		0x010
#define SIU_MEMCFG_5		0x014
#define SIU_MEMCFG_6		0x018
#define SIU_MEMCFG_7		0x01C
#define SIU_MEMCFG_8		0x020

// video control
#define SIU_VIDEO_CTRL_1	0x028
#define SIU_VIDEO_CTRL_2	0x02C
#define SIU_VIDEO_CTRL_3	0x030
#define SIU_VIDEO_CTRL_4	0x034
#define SIU_VIDEO_CTRL_5	0x038
#define SIU_VIDEO_CTRL_6	0x03C
#define SIU_VIDEO_CTRL_7	0x040
#define SIU_VIDEO_CTRL_8	0x044
#define SIU_VIDEO_CTRL_9	0x048
#define SIU_VIDEO_CTRL_10	0x04C
#define SIU_VIDEO_CTRL_11	0x050
#define SIU_VIDEO_CTRL_12	0x054
#define SIU_VIDEO_CTRL_13	0x058
#define SIU_VIDEO_CTRL_14	0x05C

// SIB
#define SIB_SIZE		0x060
#define SIB_SOUND_RCV_START	0x064
#define SIB_SOUND_TRANS_START	0x068
#define SIB_TEL_RCV_START	0x06C
#define SIB_TEL_TRANS_START	0x070
#define SIB_CONTROL		0x074
#define SIB_SOUND_HOLDING	0x078
#define SIB_TEL_HOLDING		0x07C
#define SIB_SF0_CONTROL		0x080
#define SIB_SF1_CONTROL		0x084
#define SIB_SF0_STATUS		0x088
#define SIB_SF1_STATUS		0x08C
#define SIB_DMA_CONTROL		0x090

// InfraRed
#define IR_CONTROL_1		0x0A0
#define IR_CONTROL_2		0x0A4
#define IR_HOLDING		0x0A8

// UARTA
#define UARTA_CONTROL_1		0x0B0
#define UARTA_CONTROL_2		0x0B4
#define UARTA_DMA_CONTROL_1	0x0B8
#define UARTA_DMA_CONTROL_2	0x0BC
#define UARTA_DMA_COUNT		0x0C0
#define UARTA_HOLDING		0x0C4

// UARTB
#define UARTB_CONTROL_1		0x0C8
#define UARTB_CONTROL_2		0x0CC
#define UARTB_DMA_CONTROL_1	0x0D0
#define UARTB_DMA_CONTROL_2	0x0D4
#define UARTB_DMA_COUNT		0x0D8
#define UARTB_HOLDING		0x0DC

// Interrupt Controller
#define IRQ_1			0x100
#define IRQ_2			0x104
#define IRQ_3			0x108
#define IRQ_4			0x10C
#define IRQ_5			0x110
#define IRQ_6			0x114
#define IRQ_ENABLE_1		0x118
#define IRQ_ENABLE_2		0x11C
#define IRQ_ENABLE_3		0x120
#define IRQ_ENABLE_4		0x124
#define IRQ_ENABLE_5		0x128
#define IRQ_ENABLE_6		0x12C

// RTC
#define RTC_HIGH		0x140
#define RTC_LOW			0x144
#define RTC_ALARM_HIGH		0x148
#define RTC_ALARM_LOW		0x14C
#define RTC_TIMER_CONTROL	0x150
#define RTC_PERIODIC_TIMER	0x154

// SPI
#define SPI_CONTROL		0x160
#define SPI_HOLDING		0x164

// IO
#define IO_CONTROL		0x180
#define IO_MF_IO_OUT		0x184
#define IO_MF_IO_DIRECTION	0x188
#define IO_MF_IO_IN		0x18C
#define IO_MF_IO_SELECT		0x190
#define IO_POWER_DOWN		0x194
#define IO_MF_POWER_DOWN	0x198

// Control
#define CLOCK_CONTROL		0x1C0
#define POWER_CONTROL		0x1C4
#define SIU_TEST		0x1C8

// CHI
#define CHI_CONTROL		0x1D8
#define CHI_POINTER_ENABLE	0x1DC
#define CHI_RCV_POINTER_A	0x1E0
#define CHI_RCV_POINTER_B	0x1E4
#define CHI_TRANS_POINTER_A	0x1E8
#define CHI_TRANS_POINTER_B	0x1EC
#define CHI_SIZE		0x1F0
#define CHI_RECV_START		0x1F4
#define CHI_TRANS_START		0x1F8
#define CHI_HOLDING		0x1FC

INLINE void siu_out(dword_t reg, dword_t val)
{
    *(__volatile__ dword_t*)(0x80000000U + SIU_BASE + reg) = val;
}

INLINE dword_t siu_in(dword_t reg)
{
    return *(__volatile__ dword_t*)(0x80000000U + SIU_BASE + reg);
}


#endif /* __MIPS_PR31700_PLATFORM_H__ */
