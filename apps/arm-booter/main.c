/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-booter/main.c
 * Description:   System Image loader for Brutus and EP7211 boards
 *                
 * @LICENSE@
 *                
 * $Id: main.c,v 1.11 2002/01/24 07:18:34 uhlig Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4/arm/kernel.h>
#include <l4io.h>

void loadELF(void *, unsigned*);
void bootELF(void *);

#include "modules.h"

int main(void)
{
    l4_kernel_info_t *kip = NULL;
    extern dword_t _start[];
    extern dword_t _end[];
    printf("\n\n\n\n");
//    putc(12); /* clear screen */
    printf("Welcome to ARM-Booter version 0.1 - built on %s %s\n",
	   __DATE__, __TIME__);
    printf("using %p-%p\n", _start, _end);

    loadmodules();

    {
	byte_t *p = (byte_t*) _binary_arm_kernel_start;
	int i = (int) _binary_arm_kernel_size;
	printf("looking for KIP: [%x:%x]\n", (unsigned) p, i);
	for (;i--;p++)
	    if ((p[0] == 'L') &&
		(p[1] == '4') &&
		(p[2] == 0xe6) &&
		(p[3] == 'K'))
	    {
		printf("Found KIP at %x\n", (unsigned) p);
		printf("Sigma0 entry   : %x\n", sigma0_entry);
		printf("Root Task entry: %x\n", root_task_entry);
	      
		kip = (l4_kernel_info_t *) p;

		kip->sigma0_eip		= sigma0_entry;
		kip->root_eip		= root_task_entry;

#if defined(CONFIG_ARCH_ARM_BRUTUS)
		kip->main_memory.low	= 0xC0000000;
		kip->main_memory.high	= 0xD8400000;

		/* Remove memory in between banks. */
		kip->dedicated[0].low	= 0xC0400000;
		kip->dedicated[0].high	= 0xC8000000;
		kip->dedicated[1].low	= 0xC8400000;
		kip->dedicated[1].high	= 0xD0000000;
		kip->dedicated[2].low	= 0xD0400000;
		kip->dedicated[2].high	= 0xD8000000;
#elif defined(CONFIG_ARCH_ARM_IPAQ)
		/* the IPaq uses just one bank. for more details see:
		 * http://handhelds.org/Compaq/iPAQH3600/iPAQ_H3600.html
		 */
		kip->main_memory.low	= 0xC0000000;
		kip->main_memory.high	= 0xC2000000;

		/* exclude all others */
		kip->dedicated[0].high	= 0;
		kip->dedicated[1].high	= 0;
		kip->dedicated[2].high	= 0;
		kip->dedicated[3].high	= 0;

#elif defined(CONFIG_ARCH_ARM_PLEB)
		kip->main_memory.low	= 0xC0000000;
		kip->main_memory.high	= 0xC8800000;

		/* Remove memory in between banks. */
		kip->dedicated[0].low	= 0xC0800000;
		kip->dedicated[0].high	= 0xC8000000;
#elif defined(CONFIG_ARCH_ARM_DNARD)
		kip->main_memory.low	= 0x08000000;
		kip->main_memory.high	= 0x0E800000;

		/* Remove memory in between banks. */
		kip->dedicated[0].low	= 0x08800000;
		kip->dedicated[0].high	= 0x0A000000;
		kip->dedicated[1].low	= 0x0A800000;
		kip->dedicated[1].high	= 0x0C000000;
		kip->dedicated[2].low	= 0x0C800000;
		kip->dedicated[2].high	= 0x0E000000;
#elif defined(CONFIG_ARCH_ARM_EP7211)
		/* EP7211 */
		((unsigned*)p)[24] = 0xC0000000;
		((unsigned*)p)[25] = 0xC0800000;
#else
#error unknown ARM platform
#endif
		break;
	    }
	if (i == -1)
	    printf("KIP not found!\n");
    }

    kip->sigma0_memory.low	= sigma0_vaddr;
    kip->sigma0_memory.high	= sigma0_vaddr + (dword_t) _binary_sigma0_size;
    kip->root_memory.low	= root_task_vaddr;
    kip->root_memory.high	= root_task_vaddr +
	(dword_t) _binary_root_task_size;

    printf("Loading kernel\n");
    loadelf(_binary_arm_kernel_start, &arm_kernel_entry, &arm_kernel_vaddr);

    printf("kernel_entry: %x\n", arm_kernel_entry);
    __asm__ __volatile__ ("mov pc,%0\n": :"r"(arm_kernel_entry));

    while(1);

}
