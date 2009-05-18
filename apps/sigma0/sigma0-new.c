/*********************************************************************
 *                
 * Copyright (C) 2001-2002,  Karlsruhe University
 *                
 * File path:     sigma0/sigma0-new.c
 * Description:   Implementation of new generic sigma0 protocol.
 *                
 * @LICENSE@
 *                
 * $Id: sigma0-new.c,v 1.12 2002/06/07 17:36:30 skoglund Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4/arch/kernel.h>
#include <l4/sigma0.h>
#include <l4io.h>

#if 1
extern "C" void *memset(void *b, int c, int size)
{
    char *p = (char *) b;

    while ( size-- )
	*(p++) = c;

    return b;
}
#endif


#define iskernelthread(x) (x.raw < myself.raw)
#define NO_MAP	(~0UL)

typedef byte_t space_id_t;

l4_kernel_info_t *kip;


#if defined(CONFIG_ARCH_ARM)

/*
 * ARM configuration.
 */

#define DEFAULT_SIZE	(12)
#define PAGE_BITS	(12)
#define PAGE_SIZE	(1 << PAGE_BITS)
#define MAX_MEM		(2*256L*1024L*1024L)
#define PAGE_SIZES	(3)

space_id_t page_array[MAX_MEM >> PAGE_BITS];
dword_t page_shifts[PAGE_SIZES+1] = { 12, 16, 20, 32 };
dword_t page_sizes[PAGE_SIZES] = { (1<<12), (1<<16), (1<<20) };

#elif defined(CONFIG_ARCH_X86)

/*
 * X86 configuration.
 */
#if defined(CONFIG_IO_FLEXPAGES)

#define IOFP_LO_BOUND 0xF0000000
#define IOFP_HI_BOUND 0xFFFFF000

#define PORT_FREE 0

#define IOFP_PORT(x) ((dword_t) (((x) >> 12) & 0x0000FFFF))
#define IOFP_LD_SIZE(x) ((dword_t) (((x) >>  2) & 0x0000003F))
#define IOFP_SIZE(x) ((dword_t) (1 << IOFP_LD_SIZE(x)))
#define MIN(a,b) (a<b?a:b)

#define X86_IO_SPACE_SIZE (1<<16)
static space_id_t io_space[X86_IO_SPACE_SIZE];
dword_t allocate_io_space(space_id_t space, dword_t addr);
void free_io_space(space_id_t space, dword_t addr);
#endif /* CONFIG_IO_FLEXPAGES */

#define DEFAULT_SIZE	(12)
#define PAGE_BITS	(12)
#define PAGE_SIZE	(1 << PAGE_BITS)
#define MAX_MEM		(256L*1024L*1024L)
#define PAGE_SIZES	(2)

space_id_t page_array[MAX_MEM >> PAGE_BITS];
dword_t page_shifts[PAGE_SIZES+1] = { 12, 22, 32 };
dword_t page_sizes[PAGE_SIZES] = { (1<<12), (1<<22) };

#else
#error Page size specifications are lacking for this architecture.
#endif


/*
**
** Sigma0 protocol.
**
*/

void init_space(l4_kernel_info_t *kip);
void free_space(space_id_t space, dword_t addr, dword_t size);
dword_t allocate_space(space_id_t space, dword_t addr, dword_t size);
dword_t allocate_space(space_id_t space, dword_t size);


extern "C" void sigma0_main(l4_kernel_info_t *_kip)
{
    l4_threadid_t client;
    l4_msgdope_t result;
    l4_fpage_t fp;
    dword_t dw0, dw1;
    dword_t msg, sz;
    int r;

    kip = _kip;

    l4_threadid_t myself = l4_myself();

    printf("sigma0: kernel_info_page is at %p\n", kip);

    if ( (((char *) kip)[0] != 'L')  | (((char *) kip)[1] != '4') |
	 (((char *) kip)[2] != 0xE6) | (((char *) kip)[3] != 'K') )
	enter_kdebug("sigma0: invalid KIP");

    init_space(kip);

    for (;;)
    {
	r = l4_ipc_wait(&client, NULL,
			&dw0, &dw1, &fp.raw,
			L4_IPC_NEVER, &result);

	for (;;)
	{
//	    printf("sigma0: tid=%08x  dw0=%08x  dw1=%08x  dw2=%08x (rcvd)\n",
//		   client.raw, dw0, dw1, fp.raw);

	    /*
	     * Handle compatibility with pagefault protocol and older
	     * versions of sigma0 RPC.
	     */

	    if ( dw0 == 0xfffffffc )
		/* Map arbitrary 4K page (old s0 compitability). */
		fp = l4_fpage(0, 12, 0, 1);
	    else if ( (fp.raw == 0 && dw0 != 0x1) || /* (PF compitability) */
		      ((dw0 & 0x3) == 0) ) /* (old s0 compitability) */
	    {
		/* Map indicated page frame. */
#if defined(CONFIG_ARCH_X86)
		if ( dw0 > (1024*1024*1024) )
#if defined(CONFIG_IO_FLEXPAGES)
		    if (dw0 >= IOFP_LO_BOUND && dw0 <= IOFP_HI_BOUND)
			fp.raw = dw0 + 0x3;
		    else
#endif    
		    fp = l4_fpage(dw0 + (1024*1024*1024), 22, 1, 1);
		else
#endif
		    fp = l4_fpage(dw0, DEFAULT_SIZE, 1, 1);
	    }
#if defined(CONFIG_ARCH_X86)
	    else if ( ((dw0 & 0x3) == 0x1) && (dw1 == (22 << 2)) )
		/* Map indicated page frame (old s0 compitability). */
		fp = l4_fpage(dw0, 22, 1, 1);
#endif

	    if ( dw0 != 0x1 )
		dw0 = 0x3;

//	    printf("sigma0: tid=%08x  dw0=%08x  dw1=%08x  dw2=%08x (ok)\n",
//		   client.raw, dw0, dw1, fp.raw);


	    msg = 2; /* We usually do a snd_fpage IPC. */
	    sz = fp.fp.size;


	    if ( dw0 == 0x1 )
	    {
		if ( iskernelthread(client) )
		    /* Recomended amount of in-kernel data. */
		    msg = 0, dw0 = 0x100 << PAGE_BITS;
		else
		{
		    /* Kernel-info-page request. */
		    dw0 = (dword_t) kip;
		    fp = l4_fpage(dw0, 12, 0, 0);
		}
	    }
	    else if ( fp.raw & 0x1 )
	    {
		if ( fp.raw & 0x2 )
		    /* Map the specified fpage. */
		    dw0 = allocate_space(client.id.task, fp.raw, sz);
		else
		    /* Map fpage from an arbitrary location. */
		    dw0 = allocate_space(client.id.task, sz);

		if ( dw0 != NO_MAP )
		{
#if defined(CONFIG_IO_FLEXPAGES)
		    if (fp.raw >= IOFP_LO_BOUND && fp.raw <= IOFP_HI_BOUND)
			fp.raw = dw0 = dw0 & ~0x3;
		    else
#endif 
		    {
			l4_fpage_t old_fp = fp;
			fp = l4_fpage(dw0, sz, 1, iskernelthread(client));
			if (! iskernelthread(client))
			{
			    fp.fp.uncacheable = old_fp.fp.uncacheable;
			    fp.fp.unbufferable = old_fp.fp.unbufferable;
			}
		    }
		}
		else
		    msg = dw0 = fp.raw = 0;
	    }
	    else
	    {
		/* Free up space.  Do not send reply. */
		free_space(client.id.task, fp.raw, sz);
		break;
	    }

//	    printf("sigma0: reply         msg=%08x  dw0=%08x  dw1=%08x\n",
//		   msg, dw0, fp.raw);

	    r = l4_ipc_reply_and_wait(client, (void *) msg, 
				      dw0, fp.raw, 0,
				      &client, NULL, 
				      &dw0, &dw1, &fp.raw,
				      L4_IPC_NEVER, &result);

	    if ( L4_IPC_ERROR(result) )
	    {
		printf("sigma0: error reply_and_wait (%x)\n", result.raw);
		break;
	    }
	}
    }
}


/*
**
** Space management.
**
*/

#define PAGE_RESERVED	0xFF
#define PAGE_FREE	0xFE
#define PAGE_SHARED	0xFC
#define PAGE_ROOT	0x04

#define PRINT_KIP

#if defined(PRINT_KIP)
#define printkip(name, attrib)					\
    printf(" kip: %s  \t<%08x,%08x> (%s)\n", #name,		\
	   kip->##name##.low, kip->##name##.high, #attrib)
#else
#define printkip(name, attrib)
#endif

void init_space(l4_kernel_info_t *kip)
{
    dword_t i;

    for ( i = 0; i < (MAX_MEM >> PAGE_BITS); i++ )
	page_array[i] = PAGE_RESERVED;

#define setmem(name, attrib)						\
    if ( kip->##name##.high )						\
    {									\
	printkip(name, attrib);						\
        if ( (kip->##name##.low  >= kip->main_memory.low) &&		\
	     (kip->##name##.high <= kip->main_memory.high) )		\
	{								\
	    for ( i = kip->##name##.low >> PAGE_BITS;			\
		  i < (kip->##name##.high) >> PAGE_BITS;		\
		  i++ )							\
									\
		page_array[i - (kip->main_memory.low >> PAGE_BITS)] =	\
		    PAGE_##attrib;					\
	}								\
	else								\
	    printf("kip->" #name " is wrong\n");			\
    }

    setmem(main_memory, FREE);
    setmem(dedicated[0], SHARED);
    setmem(dedicated[1], SHARED);
    setmem(dedicated[2], SHARED);
    setmem(dedicated[3], SHARED);
    setmem(dedicated[4], SHARED);

    setmem(reserved0, RESERVED);
    setmem(reserved1, RESERVED);
    setmem(root_memory, ROOT);
    setmem(sigma0_memory, RESERVED);
    setmem(sigma1_memory, RESERVED);
    setmem(kdebug_memory, RESERVED);

#if defined(CONFIG_IO_FLEXPAGES)
    for (dword_t idx = 0; idx < X86_IO_SPACE_SIZE; idx++)
	io_space[idx] = PORT_FREE;
#endif
}


dword_t allocate_space(space_id_t space, dword_t addr, dword_t size)
{
    if (addr < kip->main_memory.low)
    {
	printf("s0: %x < %x", addr, kip->main_memory.low);
	return NO_MAP;
    }
#if defined(CONFIG_IO_FLEXPAGES)
    if (addr >= IOFP_LO_BOUND && addr <= IOFP_HI_BOUND)
	return allocate_io_space(space,  addr);
	
#endif
    if ( size < PAGE_BITS )
	return NO_MAP;

    addr &= ~((1 << size)-1);

    /* Check if pages can be allocated. */
    dword_t tmp = addr - kip->main_memory.low;
    dword_t num = 1 << (size - PAGE_BITS);
    dword_t idx = tmp >> PAGE_BITS;

    for ( ; tmp < MAX_MEM && num--; idx++, tmp += PAGE_SIZE )
    {
	if ( ! ((page_array[idx] == PAGE_FREE) ||
		(page_array[idx] == PAGE_SHARED) ||
		(page_array[idx] == space)) )
	    return NO_MAP;
    }

    if ( tmp >= MAX_MEM )
	return NO_MAP;

    /* Mark pages as allocated. */
    tmp = addr - kip->main_memory.low;
    num = 1 << (size - PAGE_BITS);
    idx = tmp >> PAGE_BITS;

    for ( ; num--; idx++ )
	if ( page_array[idx] == PAGE_FREE )
	    page_array[idx] = space;

    return addr;
}


dword_t allocate_space(space_id_t space, dword_t size)
{
    dword_t addr, tmp, idx, num;

    if ( size < PAGE_BITS )
	return NO_MAP;

    /* Search for free location. */
    for ( addr = 0; addr < MAX_MEM; addr += (1 << size) )
    {
	tmp = addr;
	num = (1 << (size - PAGE_BITS)) + 1;
	idx = tmp >> PAGE_BITS;

	for ( ; tmp <= MAX_MEM && --num; idx++, tmp += PAGE_SIZE )
	    if ( page_array[idx] != PAGE_FREE )
		break;

	if ( num == 0 )
	{
	    /* Mark pages as allocated. */
	    tmp = addr;
	    num = 1 << (size - PAGE_BITS);
	    idx = tmp >> PAGE_BITS;

	    for ( ; num--; idx++ )
		if ( page_array[idx] == PAGE_FREE )
		    page_array[idx] = space;

	    return kip->main_memory.low + addr;
	}
	else if ( tmp >= MAX_MEM )
	    return NO_MAP;
    }

    return NO_MAP;
}


void free_space(space_id_t space, dword_t addr, dword_t size)
{
    dword_t snum, plus, tmp, num, idx, sz, endaddr;

#if defined(CONFIG_IO_FLEXPAGES)
    if (addr >= IOFP_LO_BOUND && addr <= IOFP_HI_BOUND){
	 free_io_space(space,  addr);
	 return;
    }
#endif
    if ( size < PAGE_BITS )
	return;

    addr &= ~((1 << size)-1);
    endaddr = addr + (1 << size) - PAGE_SIZE;

    /*
     * Freeing pages is tricky because we can not free a smaller page
     * if we have mapped a larger page to the task.  E.g., we can not
     * free a 4KB page residing within an already mapped 4MB page.
     */
    for ( sz = PAGE_SIZES; sz-- > 0; )
    {
	snum = size > page_shifts[sz] ? (1 << (size - page_shifts[sz])) : 1;
	plus = 0;

	for ( ; snum--; plus += page_sizes[sz] )
	{
	    tmp = (addr & ~(page_sizes[sz]-1)) + plus;
	    num = 1 << (page_shifts[sz] - PAGE_BITS);
	    idx = tmp >> PAGE_BITS;

	    /*
	     * Check if task owns all pages within a larger page for
	     * this region.  E.g., for x86 we need to check if a task
	     * owns all 4KB pages within a 4MB region.
	     */
	    while ( (page_array[idx] == space) && --num ) idx++;

	    if ( num == 0 )
	    {
		/*
		 * Task does indeed own all pages within the larger
		 * region.  We unmap the whole region to make sure
		 * that the page is properly unmapped.
		 */
		l4_fpage_unmap(l4_fpage(tmp, page_shifts[sz], 0, 0),
			       (L4_FP_OTHER_SPACES|L4_FP_FLUSH_PAGE));

		/*
		 * We now free up the actual unallocated pages.  It
		 * might be that we have flushed too many pages above,
		 * but this should be handled afterwards by the
		 * underlying application (i.e., its pager).
		 */
		num = 1 << (page_shifts[sz] - PAGE_BITS);
		idx = tmp >> PAGE_BITS;
		for ( ; num--; idx++, tmp += PAGE_SIZE )
		    if ( tmp >= addr && tmp <= endaddr &&
			 page_array[idx] == space )
			page_array[idx] = PAGE_FREE;
	    }
	}
    }
}
#if defined(CONFIG_IO_FLEXPAGES)

dword_t allocate_io_space(space_id_t space, dword_t addr){

    dword_t size = IOFP_SIZE(addr);
    dword_t port = IOFP_PORT(addr);
    

    if (port + size > X86_IO_SPACE_SIZE)
	return NO_MAP;
    
    for (dword_t idx = port; idx < port + size; idx++){
	if (io_space[idx] != PORT_FREE && io_space[idx] != space)
	    return NO_MAP;
    }
    
    for (dword_t idx = port; idx < port + size; idx++){
	io_space[idx] = space;
	}

    return addr;
}

void free_io_space(space_id_t space, dword_t addr){
    
    dword_t size = IOFP_SIZE(addr);
    dword_t port = IOFP_PORT(addr);

    if (port + size > X86_IO_SPACE_SIZE)
	return;
    
    for (dword_t idx = port; idx < port + size; idx++){
	if (io_space[idx] != space && io_space[idx] != PORT_FREE)
	    return;
    }
    
    for (dword_t idx = port; idx < port + size; idx++){
	io_space[idx] = PORT_FREE;
	}

    printf("sigma0: unmap IO-Page port = %x, size=%x, fp=%x\n", port, size, addr);
  l4_fpage_unmap( ( (l4_fpage_t){raw: addr}), 0);

}
#endif /* CONFIG_IO_FLEXPAGES */
