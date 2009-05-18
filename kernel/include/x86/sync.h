/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/sync.h
 * Description:   SMP synchronizarion inlines for x86.
 *                
 * @LICENSE@
 *                
 * $Id: sync.h,v 1.3 2001/12/04 20:14:32 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __X86_SYNC_H__
#define __X86_SYNC_H__

#ifndef CONFIG_SMP
# error should not be included!
#endif

typedef struct { volatile dword_t lock; } spinlock_t;

#define DECLARE_SPINLOCK(name) extern spinlock_t name;
#define DEFINE_SPINLOCK(name) spinlock_t name = (spinlock_t) { 0 }

INLINE void spin_lock_init(spinlock_t * lock) { lock->lock = 0; }

INLINE void spin_lock(spinlock_t * lock)
{
    __asm__ __volatile__ (
	"1:			\n"
	"xchg	%0, %1		\n"
	"cmpl	$0, %0		\n"
	"jnz	2f		\n"
	".section .spinlock	\n"
	"2:			\n"
	"cmpl	$0, %1		\n"
	"jne	2b		\n"
	"jmp	1b		\n"
	".previous		\n"
	:
	: "r"(1), "m"(*lock));
}

INLINE void spin_unlock(spinlock_t * lock)
{
    __asm__ __volatile__ (
	"movl	%0, %1\n"
	:
	: "r"(0), "m"(*lock));
}


#endif
