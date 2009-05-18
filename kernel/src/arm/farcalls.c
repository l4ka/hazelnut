/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/farcalls.c
 * Description:   Hooks for doing farcalls on ARM.
 *                
 * @LICENSE@
 *                
 * $Id: farcalls.c,v 1.9 2001/11/22 13:18:48 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

const int (*__farcall_printf)(const char *, ...) = printf;
const void (*__farcall_panic)(const char*) = panic;
const void (*__farcall_kmem_free)(ptr_t address, dword_t size) = kmem_free;
