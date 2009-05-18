/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     macros.h
 * Description:   Collection of various "global" kernel macros.
 *                
 * @LICENSE@
 *                
 * $Id: macros.h,v 1.8 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MACROS_H__
#define __MACROS_H__

/*
 * macros for CPP
 *
 * Precondition: __ARCH__ and __PLATFORM__ are defined
 * 
 * Use: If __ARCH__=x86 and __PLATFORM__=i686
 *    #include INC_ARCH(thread.h) includes include/x86/thread.h
 *    #include INC_PLATFORM(kdebug.h) includes include/x86/i686/thread.h
 *
 */
#define INC_ARCH(x) <__ARCH__/x>
#define INC_PLATFORM(x) <__ARCH__/__PLATFORM__/x>

#define L4_SECT_INIT		__attribute__ ((section (".init")))
#define L4_SECT_ROINIT		__attribute__ ((section (".roinit")))
#define L4_SECT_TEXT		__attribute__ ((section (".text")))
#define L4_SECT_KDEBUG		__attribute__ ((section (".kdebug")))
#if !defined(CONFIG_SMP)
# define L4_SECT_CPULOCAL	
#else
# define L4_SECT_CPULOCAL	__attribute__ ((section (".cpulocal")))
#endif

#define INLINE			static inline

#if (__GNUC__ >= 3)
#define EXPECT_TRUE(x)		__builtin_expect((x), 1)
#define EXPECT_FALSE(x)		__builtin_expect((x), 0)
#define EXPECT_VALUE(x,val)	__builtin_expect((x), (val))
#else /* __GNUC__ < 3 */
#define EXPECT_TRUE(x)		(x)
#define EXPECT_FALSE(x)		(x)
#define EXPECT_VALUE(x,val)	(x)
#endif /* __GNUC__ < 3 */


#endif /* __MACROS_H__ */
