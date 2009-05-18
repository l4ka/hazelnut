#ifndef __KIP_H__
#define __KIP_H__

#include <l4/l4.h>

typedef struct kernel_info_page_t {
    char	magic[4];		/* "L4uK"			*/
    dword_t	version;
    char	reserved_0x0C[8];	/* "L2" ...			*/
    
    /* these are physical addresses !!! */
    void	(*kdebug_init)();
    void	(*kdebug_exception)();
    dword_t	kdebug_low, kdebug_high;
    dword_t	sigma0_sp, sigma0_ip, sigma0_low, sigma0_high;
    dword_t	sigma1_sp, sigma1_ip, sigma1_low, sigma1_high;
    dword_t	root_sp, root_ip, root_low, root_high;

    dword_t	l4_config;
    dword_t	reserved_0x54;
    dword_t	kdebug_config, kdebug_perms;

    /* memory areas */
    dword_t	main_mem_low, main_mem_high;
    /* these regions must not be used. They contains kernel code
       (reserved mem0) or data (reserved mem1) grabbed by the kernel
       before sigma0 was initialized */
    dword_t	reserved_mem0_low, reserved_mem0_high;
    dword_t	reserved_mem1_low, reserved_mem1_high;

    /* these regions contain dedicated memory which cannot be used as
       standard memory. For example, on PCs [640, 1M] is a popular
       dedicated memory region. */
    dword_t	dedicated_mem0_low, dedicated_mem0_high;
    dword_t	dedicated_mem1_low, dedicated_mem1_high;
    dword_t	dedicated_mem2_low, dedicated_mem2_high;
    dword_t	dedicated_mem3_low, dedicated_mem3_high;
    dword_t	dedicated_mem4_low, dedicated_mem4_high;

    /* the kernel clock */
#if defined (CONFIG_SMP)
    volatile
#endif
    qword_t	clock;

    /* ? */
    dword_t	reserved_0xA8[2];

    dword_t	processor_frequency;
    dword_t	bus_frequency;
    
    dword_t	reserved_0xB8[2];
} kernel_info_page_t;
#endif
