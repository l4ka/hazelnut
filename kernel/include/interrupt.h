/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     interrupt.h
 * Description:   Interrupt handling declarations.
 *                
 * @LICENSE@
 *                
 * $Id: interrupt.h,v 1.6 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

/* interrupt owner */
#define MAX_INTERRUPTS		32

/* defined in interrupt.c */
extern tcb_t * interrupt_owner[MAX_INTERRUPTS];

/* handlers called from architecture specific part */
void handle_timer_interrupt();
void handle_interrupt(dword_t number);

/* architecture specific interrupt masking */
void mask_interrupt(unsigned int number);
void unmask_interrupt(unsigned int number);

#endif /* __INTERRUPT_H__ */
