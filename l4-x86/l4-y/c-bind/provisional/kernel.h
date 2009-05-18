/* 
 * $Id: kernel.h,v 1.1 2001/03/07 11:30:48 voelp Exp $
 */
#ifndef __L4_KERNEL_H__ 
#define __L4_KERNEL_H__ 

#include <l4/types.h>

typedef struct 
{
  dword_t magic;
  dword_t version;
  byte_t offset_version_strings;
#if 0
  byte_t reserved[7 + 5 * 16];
#else
  byte_t reserved[7];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */
  dword_t init_default_kdebug, default_kdebug_exception, 
    __unknown, default_kdebug_end;
  dword_t sigma0_esp, sigma0_eip;
  l4_low_high_t sigma0_memory;
  dword_t sigma1_esp, sigma1_eip;
  l4_low_high_t sigma1_memory;
  dword_t root_esp, root_eip;
  l4_low_high_t root_memory;
#if 0
    byte_t reserved2[16];
#else
    dword_t l4_config;
    dword_t reserved2;
    dword_t kdebug_config;
    dword_t kdebug_permission;
#endif
#endif
  l4_low_high_t main_memory;
  l4_low_high_t reserved0, reserved1;
  l4_low_high_t semi_reserved;
  l4_low_high_t dedicated[4];
  volatile dword_t clock;
} l4_kernel_info_t;

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */

#endif
