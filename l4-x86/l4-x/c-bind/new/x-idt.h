/*
 *  $Id: x-idt.h,v 1.1 2001/03/07 11:30:43 voelp Exp $ 
 */ 

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












