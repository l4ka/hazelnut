/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     l4/x86/kernel.h
 * Description:   kernel info page for x86
 *                
 * @LICENSE@
 *                
 * $Id: kernel.h,v 1.4 2001/12/13 08:36:40 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __L4__X86__KERNEL_H__
#define __L4__X86__KERNEL_H__

typedef struct 
{
    dword_t magic;
    dword_t version;
    byte_t offset_version_strings;
    byte_t reserved[7];

    /* The following stuff is undocumented; we assume that the kernel
       info page is located at offset 0x1000 into the L4 kernel boot
       image so that these declarations are consistent with section
       2.9 of the L4 Reference Manual. */
    dword_t kdebug_init; 
    dword_t kdebug_exception; 
    struct {
	dword_t low;
	dword_t high;
    } kdebug_memory;

    dword_t sigma0_esp, sigma0_eip;
    struct {
	dword_t low;
	dword_t high;
    } sigma0_memory;

    dword_t sigma1_esp, sigma1_eip;
    struct {
	dword_t low;
	dword_t high;
    } sigma1_memory;

    dword_t root_esp, root_eip;
    struct {
	dword_t low;
	dword_t high;
    } root_memory;

    dword_t l4_config;
    dword_t reserved2;
    dword_t kdebug_config;
    dword_t kdebug_permission;

    struct {
	dword_t low;
	dword_t high;
    } main_memory;

    struct {
	dword_t low;
	dword_t high;
    } reserved0;

    struct {
	dword_t low;
	dword_t high;
    } reserved1;

    struct {
	dword_t low;
	dword_t high;
    } dedicated[5];

    volatile dword_t clock;

} l4_kernel_info_t;

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */


#endif /* !__L4__X86__KERNEL_H__ */
