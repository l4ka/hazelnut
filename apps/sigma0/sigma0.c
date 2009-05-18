/*********************************************************************
 *                
 * Copyright (C) 2001-2002,  Karlsruhe University
 *                
 * File path:     sigma0/sigma0.c
 * Description:   old sigma0 implementation
 *                
 * @LICENSE@
 *                
 * $Id: sigma0.c,v 1.23 2002/05/06 13:52:51 ud3 Exp $
 *                
 ********************************************************************/
#include <config.h>

#if defined(CONFIG_L4_NEWSIGMA0)
#include "sigma0-new.c"
#else

#include <l4/l4.h>
#include <l4io.h>

#include "kip.h"

#if defined(CONFIG_ARCH_X86)
#define PAGE_BITS	    12
#define SUPERPAGE_BITS	    22
#endif
#if defined(CONFIG_ARCH_ARM)
#define PAGE_BITS	    12
#define SUPERPAGE_BITS	    20
#endif

#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#define SUPERPAGE_SIZE	(1 << SUPERPAGE_BITS)
#define SUPERPAGE_MASK	(~(SUPERPAGE_SIZE-1))



#if 1
extern "C" void memset(char* p, char c, int size)
{
    for (;size--;)
	*(p++)=c;
}
#endif


#define iskernelthread(x)	(x.raw < myself.raw)
#define MB64	64L*1024L*1024L
#define MB128	128L*1024L*1024L
#define MB256	256L*1024L*1024L
#define MB512	512L*1024L*1024L

#define MAX_MEM	MB256
static unsigned char page_array[MAX_MEM/PAGE_SIZE];


void dump_kip(kernel_info_page_t* kip)
{
    
#define kipel(x) printf(" kip: %s=\t%x\n", #x, kip->x);

    printf("%s: kernel_info_page magic is %c%c%c%c\n", __FUNCTION__,
	   ((char*) kip)[0],
	   ((char*) kip)[1],
	   ((char*) kip)[2],
	   ((char*) kip)[3]);
	
    kipel(main_mem_low); kipel(main_mem_high);

//enter_kdebug("foo");
    kipel(sigma0_low); kipel(sigma0_high); 
    kipel(root_low); kipel(root_high); 
//enter_kdebug("foo");
    kipel(reserved_mem0_low); kipel(reserved_mem0_high);
    kipel(reserved_mem1_low); kipel(reserved_mem1_high);
//enter_kdebug("foo");
    kipel(dedicated_mem0_low); kipel(dedicated_mem0_high);
    kipel(dedicated_mem1_low); kipel(dedicated_mem1_high);
//enter_kdebug("foo");
    printf("\nsigma0: Available main memory: %d MB (%d KB)\n",
	   (kip->main_mem_high-kip->main_mem_low)/1024/1024,
	   (kip->main_mem_high-kip->main_mem_low)/1024);
//enter_kdebug("foo");
}


extern "C" void sigma0_main(kernel_info_page_t* kip)
{

    l4_threadid_t client;
    int r;
    l4_msgdope_t result;
    dword_t dw0, dw1, dw2;
    dword_t msg;

    l4_threadid_t myself = l4_myself();

    printf("%s: kernel_info_page is at %p\n", __FUNCTION__, kip);
    if ((((char*) kip)[0] != 'L') |
	(((char*) kip)[1] != '4') |
	(((char*) kip)[2] != 0xE6) |
	(((char*) kip)[3] != 'K'))
	enter_kdebug("sigma0: invalid KIP!");
//    dump_kip(kip);

    dword_t free_mem = kip->main_mem_low;
    dword_t kernel_mem = kip->main_mem_high-PAGE_SIZE;

#define IDX(x) ((x)-(kip->main_mem_low/PAGE_SIZE))

    if (sizeof(page_array) < (kip->main_mem_high-kip->main_mem_low)/PAGE_SIZE)
    {
	printf("sigma0: too much memory - %d < %d\n", sizeof(page_array), (kip->main_mem_high-kip->main_mem_low)/PAGE_SIZE);
	enter_kdebug("too much memory");
    };

#define PAGE_SHARED	0xFC
#define PAGE_GONE	0xFD
#define PAGE_FREE	0xFE
#define PAGE_RESERVED	0xFF
#define PAGE_ROOT	0x04

    /* initialize the page array */
    for (dword_t i = 0; i < sizeof(page_array); i++)
	page_array[i] = PAGE_RESERVED;
    for (dword_t i = kip->main_mem_low/PAGE_SIZE;
	 i < kip->main_mem_high/PAGE_SIZE; i++)
	page_array[IDX(i)] = PAGE_FREE;

#define exclude(name,attrib)					\
    if ( kip->##name##_high ) {					\
        if ((kip->##name##_low  >= kip->main_mem_low) &&	\
	     (kip->##name##_high <= kip->main_mem_high)) {	\
	    for ( dword_t i = kip->##name##_low / PAGE_SIZE;	\
		  i < (kip->##name##_high) / PAGE_SIZE;		\
		  i++ )						\
		page_array[IDX(i)] = PAGE_##attrib; }		\
	else							\
	    printf("kip->" #name " is wrong\n"); }

    exclude(dedicated_mem0, SHARED);
    exclude(dedicated_mem1, SHARED);
    exclude(dedicated_mem2, SHARED);
    exclude(dedicated_mem3, SHARED);
    exclude(dedicated_mem4, SHARED);

    exclude(reserved_mem0, RESERVED);
    exclude(reserved_mem1, RESERVED);
    exclude(root, ROOT);
    exclude(sigma0, RESERVED);
    exclude(sigma1, RESERVED);
    exclude(kdebug, RESERVED);

#if defined(CONFIG_ARCH_X86)
    page_array[IDX(0xb8000/PAGE_SIZE)] = PAGE_SHARED;
    page_array[IDX(0x01000/PAGE_SIZE)] = PAGE_RESERVED;
#endif

    dword_t one_shot_break = 0;

/*
  macros to figure out request type
  assumption: dw0:dw1:dw2 contains the received message
 */

#define GET_ANY_PAGE (dw0 == 0xFFFFFFFC)
#define GET_KMEM_PAGES ((dw0 == 1) && ((dw1 & 0xFF) == 0))
#define GET_INFO_PAGE ((dw0 == 1) && ((dw1 & 0xFF) == 1))
#define GET_THIS_PAGE_SUPER ((dw0 & 1) && (dw1 == (SUPERPAGE_BITS << 2)))

#if defined(CONFIG_ARCH_X86)
#define GET_THIS_PAGE (dw0 < 0x40000000)
#endif
#if defined(CONFIG_ARCH_ARM_EP7211)
#define GET_THIS_PAGE (1)
#endif

    while(1)
    {
//printf("%s: do ipc_wait\n", __FUNCTION__);
	r = l4_ipc_wait(&client, NULL, &dw0, &dw1, &dw2, L4_IPC_NEVER, &result);
	
	while (1)
	{
	    if (one_shot_break)
	    {
		enter_kdebug("one_shot_break");
		one_shot_break = 0;
	    }
	    
	    msg = 2; /* usually we map an fpage */


#if 0
	    if (GET_ANY_PAGE)
	    {
		if (iskernelthread(client))
		{
		    /* grant any page */
		    printf("sigma0: s0 receives %x from kthread %x -> ",
			   dw0, client.raw);
		    while (kernel_mem > 0 &&
			   page_array[IDX(kernel_mem/PAGE_SIZE)] != PAGE_FREE)
			kernel_mem -= PAGE_SIZE;
		    
		    if (kernel_mem == 0)
		    {
			//enter_kdebug("sigma0 out of memory");
			msg = dw0 = dw1 = dw2 = 0;
		    }
		    else
		    {
			page_array[IDX(kernel_mem/PAGE_SIZE)] = PAGE_GONE;
			dw0 = kernel_mem;
			dw1 = dw0 | (PAGE_BITS << 2) | 3;
			dw2 = 0;
			
			printf(" grant: %x\n", dw0);
		    }
		}
		else
		{
#if 0
		    printf("sigma0: s0 receives %x from %p -> map any page",
			   dw0, client);
#endif
		    
		    while (free_mem < MAX_MEM && page_array[IDX(free_mem/PAGE_SIZE)] != PAGE_FREE)
			free_mem += PAGE_SIZE;
		    
		    if (free_mem >= MAX_MEM)
		    {
			//enter_kdebug("sigma0 out of memory");
			msg = dw0 = dw1 = dw2 = 0;
		    }
		    else
		    {
#if defined(CONFIG_VERSION_X0)
			page_array[IDX(free_mem/PAGE_SIZE)] = client.id.task;
#elif defined(CONFIG_VERSION_X1)
			page_array[IDX(free_mem/PAGE_SIZE)] = PAGE_GONE;
#endif
			dw0 = free_mem;
			dw1 = dw0 | (PAGE_BITS << 2) | 2;
			dw2 = 0;
			
//			printf(" map: %x\n", dw0);
			
		    }
		}
		goto done;
	    }

	    if (GET_THIS_PAGE)
	    
	    msg = 0;
	    printf("ill sigma0 request %x:%x:%x from %x",
		   dw0, dw1, dw2, client.raw);
	    enter_kdebug("ill_s0_request");
	    


#else
	    if (iskernelthread(client))
	    {
		switch (dw0)
		{
		case 0xFFFFFFFC:
		case 0xFFFFFFFE:	/* Jochen */
		    printf("sigma0: s0 receives %x from a kernel thread -> grant any page\n",
			   dw0);
		    /* grant any page */
		    while (kernel_mem > 0 && page_array[IDX(kernel_mem/PAGE_SIZE)] != PAGE_FREE)
			kernel_mem -= PAGE_SIZE;

		    if (kernel_mem == 0)
		    {
			//enter_kdebug("sigma0 out of memory");
			msg = dw0 = dw1 = dw2 = 0;
		    }
		    else
		    {
			page_array[IDX(kernel_mem/PAGE_SIZE)] = PAGE_GONE;
			dw0 = kernel_mem;
			dw1 = dw0 | (PAGE_BITS << 2) | 3;
			dw2 = 0;
			
			printf(" grant: %x:%x:%x-%x\n", dw0, dw1, dw2, msg);
		    }
		    break;
		case 0x00000001:
		    printf("sigma0: %x for number of recommended pages\n",
			   client.raw);
		    if ((dw1 & 0xFF) == 0)
		    {
			/* reply number of pages recommended for kernel use */
			msg = 0; dw0 = 0x100;
		    }
		    else
			msg = dw0 = dw1 = dw2 = 0;
		    break;
		default:
		    printf("sigma0: unknown request %x:%x:%x from %x\n",
			   dw0, dw1, dw2, client.raw);
		    msg = dw0 = dw1 = dw2 = 0;
		    break;
		}
	    }
	    else
	    {
		switch(dw0)
		{
		case 0xFFFFFFFC:
#if 0
		    printf("sigma0: s0 receives %x from %p -> map any page",
			   dw0, client);
#endif
		    
		    while (free_mem < MAX_MEM && page_array[IDX(free_mem/PAGE_SIZE)] != PAGE_FREE)
			free_mem += PAGE_SIZE;

		    if (free_mem >= MAX_MEM)
		    {
			//enter_kdebug("sigma0 out of memory");
			msg = dw0 = dw1 = dw2 = 0;
		    }
		    else
		    {
#if defined(CONFIG_VERSION_X0)
			page_array[IDX(free_mem/PAGE_SIZE)] = client.id.task;
#elif defined(CONFIG_VERSION_X1)
			page_array[IDX(free_mem/PAGE_SIZE)] = PAGE_GONE;
#endif
			dw0 = free_mem;
			dw1 = dw0 | (PAGE_BITS << 2) | 2;
			dw2 = 0;
			
//			printf(" map: %x\n", dw0);
		    }
		    break;

		case 0x00000001:
		    if ((dw1 & 0xFF) == 1)
		    {
			printf("sigma0: %x requests kernel-info page\n",
			       client.raw);
			dw0 = 0;
			dw1 = (((dword_t)kip) & PAGE_MASK) | (PAGE_BITS << 2);
			dw2 = 0;
		    }
		    else
			msg = dw0 = dw1 = dw2 = 0;
		    break;

#if defined(CONFIG_ARCH_X86)
		case 0x00000000:
		case 0x00000002 ... 0x40000000:
#elif defined(CONFIG_ARCH_ARM_EP7211) || defined(CONFIG_ARCH_ARM_BRUTUS)
		case 0xC0000000 ... 0xC0800000:
#endif
//		    printf("sigma0: s0 receives %x,%x from %x\n", dw0, dw1, client.raw);
		    if ( (dw0 & 1) && (dw1 & 0xFF) == (SUPERPAGE_BITS << 2) )
		    {
			/* map superpage writeable
			   and uncacheable !!! */
			    
			dword_t adr = dw0 & SUPERPAGE_MASK;
			dword_t i;
			for (i = 0; i < SUPERPAGE_SIZE/PAGE_SIZE; i++)
			    if (page_array[IDX(adr/PAGE_SIZE + i)] != PAGE_FREE)
			    {                   
				msg = 0;
				break;
			    }
			    
			if (msg)
			{
			    for (i = 0; i < 1024; i++)
#if defined(CONFIG_VERSION_X0)
				page_array[IDX(adr/PAGE_SIZE + i)] = client.id.task;
#elif defined(CONFIG_VERSION_X1)
			    page_array[IDX(adr/PAGE_SIZE + i)] = PAGE_GONE;
#endif
				
			    dw0 = adr;
			    dw1 = dw0 | (SUPERPAGE_BITS << 2) | 2;
			    dw2 = 0;
			    printf("sigma0: map 4M page at %x to %x\n", adr, client.raw);
			}
		    }
		    else
		    {
			if (page_array[IDX(dw0/PAGE_SIZE)] != PAGE_FREE && 
			    page_array[IDX(dw0/PAGE_SIZE)] != PAGE_SHARED
#if defined(CONFIG_VERSION_X0)
			    && page_array[IDX(dw0/PAGE_SIZE)] != client.id.task
#endif
			    )
			{
#if 1
			    printf("sigma0: page %x requested twice, old=%x, new=%x\n", dw0, page_array[IDX(dw0/PAGE_SIZE)], client.id.task);
//			    enter_kdebug("page requested twice");
#endif
			    dw0 = dw1 = dw2 = msg = 0;
			}
			else
			{
			    /* mark only free pages - ignore the shared!!! */
			    if (page_array[IDX(dw0/PAGE_SIZE)] == PAGE_FREE)
#if defined(CONFIG_VERSION_X0)
				page_array[IDX(dw0/PAGE_SIZE)] = client.id.task;
#elif defined(CONFIG_VERSION_X1)
			    page_array[IDX(dw0/PAGE_SIZE)] = PAGE_GONE;
#endif
			    
			    /* map 4k page, writeable */
			    dw0 &= PAGE_MASK;
			    dw1 = dw0 | (PAGE_BITS << 2) | 2;
			    dw2 = 0;
			}
		    }
		    break;

#if defined(CONFIG_ARCH_X86)
		case 0x40000004 ... 0xC0000000:
#elif defined(CONFIG_ARCH_ARM_EP7211)
		case 0xFFFFFFFF: /* dummy */
#endif
		{
		    printf("sigma0: s0 receives %x from %x\n",
			   dw0, client.raw);
		    /* map 4M page */
		    //enter_kdebug("s0: map superpage");
		    
		    dw0 = (dw0 & SUPERPAGE_MASK) + 0x40000000;
		    dw1 = dw0 | (SUPERPAGE_BITS << 2) | 2;	/* map superpage writeable
						   and uncacheable !!! */
		    dw2 = 0;

		}
		break;

		}
	    }
#endif




	done:
#if 1
//printf("%s: do reply_and_wait\n", __FUNCTION__);
	    r = l4_ipc_reply_and_wait(client, (void*) msg, 
				      dw0, dw1, dw2,
				      &client, NULL, 
				      &dw0, &dw1, &dw2,
				      L4_IPC_NEVER, &result);
//printf("%s: done reply_and_wait\n", __FUNCTION__);
#endif
#if 1
	    if (L4_IPC_ERROR(result))
	    {
		printf("%s: error reply_and_wait (%x)\n", __FUNCTION__, result.raw);
		break;
	    }
#endif
	}
    }
}

#endif /* !CONFIG_L4_NEWSIGMA0 */
