/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/kdebug.h
 * Description:   Kdebug macros specific to ARM.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug.h,v 1.11 2001/12/11 23:27:21 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __ARM__KDEBUG_H__
#define __ARM__KDEBUG_H__

INLINE void spin(int pos = 80)
{
    panic(__FUNCTION__);
    while (1);
}

INLINE void spin1(dword_t x)
{
    //putc('_');
}

#if 0
INLINE void enter_kdebug(const char* message = NULL)
{
    if (message)
	printf(message);
    
    __asm__ __volatile__ (
	"2:	adr	lr, 1f			\n"
	"	ldr	pc, 0f			\n"
	"0:	.word	kern_kdebug_entry	\n"
	"1:					\n"
	);
}
#else
#define enter_kdebug(args...)				\
{							\
    printf("KD# " args );				\
							\
    __asm__ __volatile__ (				\
	"2:	adr	lr, 1f			\n"	\
	"	ldr	pc, 0f			\n"	\
	"0:	.word	kern_kdebug_entry	\n"	\
	"1:					\n"	\
	);						\
}
#endif

INLINE ptr_t get_current_pgtable()
{								
    dword_t x;							
								
    __asm__ __volatile__ (					
	"	mrc	p15, 0, %0, c2, c0, 0	\n"		
	:"=r"(x));						
								
    return (ptr_t) (x & ~((1<<14) - 1));
}

#endif /* __ARM__KDEBUG_H__ */

