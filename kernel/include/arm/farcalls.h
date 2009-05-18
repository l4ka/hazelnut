/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/farcalls.h
 * Description:   Farcall wrappers.
 *                
 * @LICENSE@
 *                
 * $Id: farcalls.h,v 1.10 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__FARCALLS_H__
#define __ARM__FARCALLS_H__

extern int (*__farcall_printf)(const char *, ...);
extern void (*__farcall_panic)(const char*);
extern void (*__farcall_kmem_free)(ptr_t address, dword_t size);

#define printf __farcall_printf
#define panic __farcall_panic
#define kmem_free __farcall_kmem_free

#endif /* __ARM__FARCALLS_H__ */
