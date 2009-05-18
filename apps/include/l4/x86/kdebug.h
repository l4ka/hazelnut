/*********************************************************************
 *                
 * Copyright (C) 1999 - 2002,  Karlsruhe University
 *               and Dresden University
 *                
 * File path:     l4/x86/kdebug.h
 * Description:   kernel debugger protocol
 *                
 * @LICENSE@
 *                
 * $Id: kdebug.h,v 1.7 2002/03/13 13:34:58 uhlig Exp $
 *                
 ********************************************************************/

#ifndef __L4_X86_KDEBUG_H__ 
#define __L4_X86_KDEBUG_H__ 


#define enter_kdebug(text...) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

#define asm_enter_kdebug(text...) \
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"

#define kd_display(text) \
asm(\
    "int	$3	\n\t"\
    "nop		\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

#define ko(c) 					\
  asm(						\
      "int	$3	\n\t"			\
      "cmpb	%0,%%al	\n\t"			\
      : /* No output */				\
      : "N" (c)					\
      )

/*
 * prototypes
 */
L4_INLINE void outchar(char c);
L4_INLINE void outstring(char *text);
L4_INLINE void outhex32(int number);
L4_INLINE void outhex20(int number);
L4_INLINE void outhex16(int number);
L4_INLINE void outdec(int number);

L4_INLINE void outchar(char c)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$0,%%al	\n\t"
      : /* No output */
      : "a" (c)
      );
}

/* actually outstring is outcstring */
L4_INLINE void outstring(char *text)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$2,%%al \n\t"
      : /* No output */
      : "a" (text)
      );
}

L4_INLINE void outhex32(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$5,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outhex20(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$6,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outhex16(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$7, %%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outdec(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$11, %%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE char kd_inchar(void)
{
    /* be aware that this stops the entire box until a key is pressed */
    char c;
    __asm__ __volatile__ (
	"int	$3		\n\t"
	"cmpb	$13, %%al	\n\t"
	: "=a" (c));
    return c;
}

#endif /* __L4_X86_KDEBUG_H__ */
