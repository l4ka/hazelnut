/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     kmemory.h
 * Description:   Kernel memory allocator function declarations.
 *                
 * @LICENSE@
 *                
 * $Id: kmemory.h,v 1.11 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __KMEMORY_H__
#define __KMEMORY_H__

#ifdef CONFIG_HAVE_ARCH_KMEM
 
#include INC_ARCH(kmemory.h)

#else /* !CONFIG_HAVE_ARCH_KMEM */

#define KMEM_CHUNKSIZE 0x400

ptr_t kmem_alloc(dword_t size);
void kmem_free(ptr_t address, dword_t size);


#endif /* !CONFIG_HAVE_ARCH_KMEM */


#endif /* !__KMEMORY_H__ */
