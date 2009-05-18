/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/pleb-uart.h
 * Description:   Macros for serial port handling on PLEB
 *                
 * @LICENSE@
 *                
 * $Id: pleb-uart.h,v 1.1 2001/12/27 17:07:07 ud3 Exp $
 *                
 ********************************************************************/
#ifndef L4__ARM__PLEB__UART_H
#define L4__ARM__PLEB__UART_H

/*
 * Base address of UART
 */
#ifdef NATIVE
# define L4_UART_BASE	0x80050000
#else
# define L4_UART_BASE	0xFFF20000
#endif

/* Control registers */
#define L4_UART_UTCR0	*((volatile dword_t *) (L4_UART_BASE + 0x00))
#define L4_UART_UTCR1	*((volatile dword_t *) (L4_UART_BASE + 0x04))
#define L4_UART_UTCR2	*((volatile dword_t *) (L4_UART_BASE + 0x08))
#define L4_UART_UTCR3	*((volatile dword_t *) (L4_UART_BASE + 0x0c))

/* Data register */
#define L4_UART_UTDR	*((volatile dword_t *) (L4_UART_BASE + 0x14))

/* Status registers */
#define L4_UART_UTSR0	*((volatile dword_t *) (L4_UART_BASE + 0x1c))
#define L4_UART_UTSR1	*((volatile dword_t *) (L4_UART_BASE + 0x20))



/*
 * Bits defined in control register 0.
 */
#define L4_UART_PE	(1 << 0)	/* Parity enable */
#define L4_UART_OES	(1 << 1)	/* Odd/even parity select */
#define L4_UART_SBS	(1 << 2)	/* Stop bit select */
#define L4_UART_DSS	(1 << 3)	/* Data size select */
#define L4_UART_SCE	(1 << 4)	/* Sample clock rate enable */
#define L4_UART_RCE	(1 << 5)	/* Receive clk. rate edge select */
#define L4_UART_TCE	(1 << 6)	/* Transmit clk. rate edge select */


/*
 * Bits defined in control register 3.
 */
#define L4_UART_RXE	(1 << 0)	/* Receiver enable */
#define L4_UART_TXE	(1 << 1)	/* Transmitter enable */
#define L4_UART_BRK	(1 << 2)	/* Break */
#define L4_UART_RIO	(1 << 3)	/* Receive FIFO interrupt enable */
#define L4_UART_TIE	(1 << 4)	/* Transmit FIFO interrupt enable */
#define L4_UART_LBM	(1 << 5)	/* Loopback mode */


/*
 * Baud rate devisror (contained in control registers 1 and 2).
 */
#define L4_UART_GET_BRD() 					\
    ( (((l4_uint32_t) L4_UART_UTCR1 & 0x0f) << 8) + 		\
      (l4_uint8_t) L4_UART_UTCR2 )

#define L4_UART_SET_BRD(brd) 					\
    ( *(l4_uint32_t *) L4_UART_UTCR1 = brd & 0xff,		\
      *(l4_uint32_t *) L4_UART_UTCR2 = (brd >> 8) & 0x0f )	\

#define L4_BRD_TO_BAUDRATE(brd)		(3686400 / ((brd+1) << 4))
#define L4_BAUDRATE_TO_BRD(rate)	(3686400 / (rate << 4) - 1)


/*
 * Bits defined in status register 0.
 */
#define L4_UART_TFS	(1 << 0)	/* Transmit FIFO service req. */
#define L4_UART_RFS	(1 << 1)	/* Receive FIFO service req. */
#define L4_UART_RID	(1 << 2)	/* Receiver idle */
#define L4_UART_RBB	(1 << 3)	/* Receiver begin of break */
#define L4_UART_REB	(1 << 4)	/* Receiver end of break */
#define L4_UART_EIF	(1 << 5)	/* Error in FIFO */


/*
 * Bits defined in status register 0.
 */
#define L4_UART_TBY	(1 << 0)	/* Transmitter busy flag */
#define L4_UART_RNE	(1 << 1)	/* Receiver FIFO not empty */
#define L4_UART_TNF	(1 << 2)	/* Transmitter FIFO not full */
#define L4_UART_PRE	(1 << 3)	/* Parity error */
#define L4_UART_FRE	(1 << 4)	/* Framing error */
#define L4_UART_ROR	(1 << 5)	/* Receive FIFO overrun */



#endif /* !L4__ARM__PLEB__UART_H */
