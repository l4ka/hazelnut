/**********************************************************************
 * (c) 1999, 2000 by University of Karlsruhe and Dresden University
 *
 * filename: l4/x86/kdebug.h
 * 
 */

#ifndef __L4_X86_KDEBUG_H__ 
#define __L4_X86_KDEBUG_H__ 

/*
 * prototypes
 */
L4_INLINE void outchar(char c)
{
	__asm {
		mov		al, c;
		int		0x3;
		cmp		al, 0
	}
}

L4_INLINE void enter_kdebug(char *text)
{
	__asm {
		mov		eax, text
		int		0x3
		cmp		al, 2
		int		0x3
		nop
		nop
	}
}

/* actually outstring is outcstring */
L4_INLINE void outstring(char *text)
{
	__asm {
		mov		eax, text
		int		0x3
		cmp		al, 2
	}
}

L4_INLINE void outhex32(int number)
{
	__asm {
		mov		eax, number
		int		0x3
		cmp		al, 5
	}
}

L4_INLINE void outhex20(int number)
{
	__asm {
		mov		eax, number
		int		0x3
		cmp		al, 6
	}
}

L4_INLINE void outhex16(int number)
{
  __asm {
		mov		eax, number
		int		0x3
		cmp		al, 7
	}
}

L4_INLINE void outdec(int number)
{
  __asm {
		mov		eax, number
		int		0x3
		cmp		al, 11
	}
}


#endif /* __L4_X86_KDEBUG_H__ */
