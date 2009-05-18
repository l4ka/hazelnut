/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *               and Dresden University
 *                
 * File path:     l4/x86/idt.h
 * Description:   idt entry specification 
 *                (see: IA-32 architecture reference manual)
 *                
 * @LICENSE@
 *                
 * $Id: idt.h,v 1.2 2001/12/13 08:36:40 uhlig Exp $
 *                
 ********************************************************************/

#ifndef __L4_X86_IDT_H__ 
#define __L4_X86_IDT_H__ 

#define TRAPGATE        0x70
#define USERLEVEL       0x03
#define SEGPRESENT      0x01

typedef struct { 
   unsigned IntHandlerLow:  16; 
   unsigned Selector:       16; 
   unsigned Reserved:        5; 
   unsigned TrapGate:        8; 
   unsigned Privilege:       2; 
   unsigned Present:         1; 
   unsigned IntHandlerHigh: 16;
} IDTEntryT;

typedef void (*IntHandlerT)(void);

#endif /* __L4_X86_IDT_H__ */
