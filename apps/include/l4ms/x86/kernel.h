/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *               and Dresden University
 *                
 * File path:     l4/x86/kernel.h
 * Description:   kernel info page
 *                
 * @LICENSE@
 *                
 * $Id: kernel.h,v 1.2 2001/12/13 08:47:30 uhlig Exp $
 *                
 ********************************************************************/

#ifndef __L4_KERNEL_H__ 
#define __L4_KERNEL_H__ 

typedef struct 
{
    dword_t magic;
    dword_t version;
    byte_t offset_version_strings;
    byte_t reserved[7];
    dword_t init_default_kdebug, default_kdebug_exception, 
	__unknown, default_kdebug_end;
    dword_t sigma0_esp, sigma0_eip;
    l4_low_high_t sigma0_memory;
    dword_t sigma1_esp, sigma1_eip;
    l4_low_high_t sigma1_memory;
    dword_t root_esp, root_eip;
    l4_low_high_t root_memory;
    dword_t l4_config;
    dword_t reserved2;
    dword_t kdebug_config;
    dword_t kdebug_permission;
    l4_low_high_t main_memory;
    l4_low_high_t reserved0, reserved1;
    l4_low_high_t semi_reserved;
    l4_low_high_t dedicated[4];
    volatile dword_t clock;
} l4_kernel_info_t;

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */

#endif
