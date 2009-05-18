#include <stdio.h>
#include <unistd.h>

#include <l4/l4.h>

#include "globals.h"

#include <l4/rmgr/rmgr.h>
#include "init.h"
#include "memmap.h"
#include "memmap_lock.h"

owner_t __memmap[MEM_MAX/L4_PAGESIZE];
__superpage_t __memmap4mb[SUPERPAGE_MAX];

vm_offset_t mem_high;
vm_offset_t mem_phys;

l4_kernel_info_t *l4_info;

last_pf_t last_pf[TASK_MAX];

int memmap_lock = -1;

static void find_free(dword_t *d1, dword_t *d2, owner_t owner);

void
pager(void)
{
  dword_t d1, d2, pfa;
  void *desc;
  int err;
  l4_threadid_t t;
  l4_msgdope_t result;

  /* we (the main thread of this task) will serve as a pager for our
     children. */

  /* now start serving the subtasks */
  for (;;)
    {
      err = l4_i386_ipc_wait(&t, 0, &d1, &d2, L4_IPC_NEVER, &result);

      while (!err)
	{
	  /* we must synchronise the access to memmap functions */
	  enter_memmap_functions(RMGR_LTHREAD_PAGER, rmgr_super_id);
             
	  /* we received a paging request here */
	  /* handle the sigma0 protocol */
	  
	  desc = L4_IPC_SHORT_MSG;

	  if (t.id.task > O_MAX)
	    {
	      /* OOPS.. can't map to this sender. */
	      d1 = d2 = 0;
	    }
	  else			/* valid requester */
	    {
	      pfa = d1 & 0xfffffffc;

	      if (d1 == 0xfffffffc)
		{
		  /* map any free page back to sender */
		  find_free(&d1, &d2, t.id.task);
		  
		  if ((d2 != 0) && memmap_alloc_page(d1, t.id.task))
		    desc = L4_IPC_SHORT_FPAGE;
		  else d1 = d2 = 0;
		}
	      else if (d1 == 1 && (d2 & 0xff) == 1)
		{
		  /* kernel info page requested */
		  d1 = 0;
		  d2 = l4_fpage((dword_t) l4_info, L4_LOG2_PAGESIZE,
				L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
		  desc = L4_IPC_SHORT_FPAGE;
		}
	      else if (pfa < 0x40000000)
		{
		  /* check if address lays inside our memory. we do that
		   * here because the memmap_* functions do not check
		   * about that */
		  if (pfa >= mem_high)
		    /* yes, ugly, but easy... */
		    goto access_violation;
		  
		  if (pfa < L4_PAGESIZE)
		    /* from now on, we do not implicit map page 0. To
		     * map page 0, do it explicit by rmgr_get_page0() */
		    goto access_violation;
		  
		  /* map a specific page */
		  if ((d1 & 1) && (d2 == (L4_LOG2_SUPERPAGESIZE << 2)))
		    {
		      /* superpage request */
		      if (!no_pentium && 
			  memmap_alloc_superpage(pfa, t.id.task))
			{
			  d1 &= L4_SUPERPAGEMASK;
			  d2 = l4_fpage(d1, L4_LOG2_SUPERPAGESIZE,
					L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
			  desc = L4_IPC_SHORT_FPAGE;

			  /* flush the superpage first so that
                             contained 4K pages which already have
                             been mapped to the task don't disturb the
                             4MB mapping */
			  l4_fpage_unmap(l4_fpage(d1, L4_LOG2_SUPERPAGESIZE,
						  0, 0),
					 L4_FP_FLUSH_PAGE|L4_FP_OTHER_SPACES);

			  goto reply;
			}
		    }
		  /* failing a superpage allocation, try a single page
                     allocation */
		  if (memmap_alloc_page(d1, t.id.task))
		    {
		      d1 &= L4_PAGEMASK;
		      d2 = l4_fpage(d1, L4_LOG2_PAGESIZE,
				    L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		  else if (pfa >= mem_lower * 1024L && pfa < 0x100000L)
		    {
		      /* adapter area, page faults are OK */
		      d1 &= L4_PAGEMASK;
		      d2 = l4_fpage(d1, L4_LOG2_PAGESIZE,
				    L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		  else 
		    
        access_violation:
		  if ((d2!=(L4_LOG2_SUPERPAGESIZE<<2)) && !no_checkdpf)
		    {
		      /* check for double page fault */
		      last_pf_t *last_pfp = last_pf + t.id.task;
                      if ((d1==last_pfp->pfa) && (d2==last_pfp->eip))
                        {
			  static char errmsg[100];
                          d1 &= L4_PAGEMASK;
			  sprintf(errmsg,"\r\n"
				  "RMGR: thread 0x%x at 0x%x is trying "
				  "to get page 0x%x",
				  t.raw, d2, d1);
			  outstring(errmsg);
			  
                         if (d1 < mem_high)
                           {
                             owner_t owner = memmap_owner_page(d1);
                             if (owner == O_RESERVED)
                               sprintf(errmsg, " which is reserved");
                             else
                               sprintf(errmsg," allocated by task %x", owner);
                             outstring(errmsg);
                             
                             if (!quota_alloc_mem(t.id.task, d1, L4_PAGESIZE))
                               outstring("\r\n      (Cause: quota exceeded)");
                           }
                         else
                           {
                             sprintf(errmsg," (memhigh=%08x)", mem_high);
                             outstring(errmsg);
			   }

                         outstring("\r\n");

                         enter_kdebug("double page fault");
                       }
                     last_pfp->pfa = d1;
                     last_pfp->eip = d2;
                     d1 = d2 = 0;
                   }
		}
	      else if (pfa >= 0x40000000 && pfa < 0xC0000000 && !(d1 & 1))
		{
		  /* request physical 4 MB area at pfa + 0x40000000 */

		  /* XXX UGLY HACK! */

		  static vm_offset_t scratch = 0x40000000; /* XXX hardcoded */

		  pfa = (pfa & L4_SUPERPAGEMASK);

		  if (memmap_alloc_superpage(pfa + 0x40000000, t.id.task))
		    {
		      /* map the superpage into a scratch area */
		      l4_fpage_unmap(l4_fpage(scratch, L4_LOG2_SUPERPAGESIZE, 
					      0,0),
				     L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
		      l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
				       pfa, 0,
				       L4_IPC_MAPMSG(scratch, 
						     L4_LOG2_SUPERPAGESIZE),
				       &d1, &d2, L4_IPC_NEVER, &result);

		      /* grant the superpage to the subtask */
		      d1 = pfa;
		      d2 = l4_fpage(scratch, L4_LOG2_SUPERPAGESIZE, 
				    L4_FPAGE_RW, L4_FPAGE_GRANT).fpage;
		      desc = L4_IPC_SHORT_FPAGE;
		    }
		}
#if defined(CONFIG_IO_FLEXPAGES)
	      else if (pfa >= 0xF0000000 && pfa < 0xFFFFF043){
		  /* Map the whole IO Space */
		  d1 = d2 = pfa;
		  desc = L4_IPC_SHORT_FPAGE;
	      }

#endif
	      else 
		{
		  /* unknown request */
		  d1 = d2 = 0;
		}
	    }

	reply:
		  
	  leave_memmap_functions(RMGR_LTHREAD_PAGER, rmgr_super_id);

	  /* send reply and wait for next message */
	  err = l4_i386_ipc_reply_and_wait(t, desc, d1, d2,
					   &t, 0, &d1, &d2,
					   L4_IPC_TIMEOUT(0,1,0,0,0,0), 
					   /* snd timeout = 0 */
					   &result);

	  /* send error while granting?  flush fpage! */
	  if (err == L4_IPC_SETIMEOUT
	      && desc == L4_IPC_SHORT_FPAGE
	      && (d2 & 1))
	    {
	      l4_fpage_unmap((l4_fpage_t) d2, 
			     L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
	    }
	}
    }
}

static void
find_free(dword_t *d1, dword_t *d2, owner_t owner)
{
  /* XXX this routine should be in the memmap_*() interface because we
     don't know about quotas here.  we can easily select a free page
     which we later can't allocate because we're out of quota. */

  vm_offset_t address;

  for (address = 0; address < mem_high; address += L4_SUPERPAGESIZE)
    {
      if (memmap_nrfree_superpage(address) == 0)
	continue;

      for (;address < mem_high; address += L4_PAGESIZE)
	{
	  if (! quota_check_mem(owner, address, L4_PAGESIZE))
	    continue;
	  if (memmap_owner_page(address) != O_FREE)
	    continue;
	  
	  /* found! */
	  *d1 = address;
	  *d2 = l4_fpage(address, L4_LOG2_PAGESIZE,
			 L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
	  
	  return;
	}
    }

  /* nothing found! */
  *d1 = *d2 = 0;
}


vm_offset_t
reserve_range(unsigned int size_and_align, owner_t owner,
	      vm_offset_t range_low, vm_offset_t range_high)
{
#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
  vm_offset_t address, min_address, max_address;
  unsigned int have_pages;
  unsigned int size  = size_and_align & L4_PAGEMASK;
  unsigned int align = size_and_align & ~L4_PAGEMASK & ~RMGR_MEM_RES_FLAGS_MASK;
  unsigned int flags = size_and_align & ~L4_PAGEMASK & RMGR_MEM_RES_FLAGS_MASK;
  unsigned int need_pages = size / L4_PAGESIZE;
  
  /* suggestive round alignment */
  if (align < L4_LOG2_PAGESIZE)
    align = L4_LOG2_PAGESIZE;
  else if (align > 28)
    align = 28;
  align = ~((1 << align) - 1);

  max_address = mem_high;
  
  if (range_high != 0)
    max_address = min(max_address, range_high);

  if (flags & RMGR_MEM_RES_DMA_ABLE)
    max_address = min(max_address, 16*1024*1024);

  /* round down to next L4 page */
  max_address &= L4_PAGEMASK;

  /* round up to next L4 page */
  min_address = (range_low + ~L4_PAGEMASK) & L4_PAGEMASK;

  /* some sanity checks */
  if (max_address-min_address < size)
    return -1;

  if (flags & RMGR_MEM_RES_UPWARDS)
    {
      /* search upwards min_address til max_address */
      max_address -= size;
      
      for (address = min_address; ; )
       {
	 /* round up to next proper alignend chunk */
	 address = (address + ~align) & align;

	 if (address > max_address)
	   return -1;

	 for (have_pages = 0; ; address += L4_PAGESIZE)
	   {
	     if (!quota_check_mem(owner, address, L4_PAGESIZE)
		 || memmap_owner_page(address) != O_FREE)
	       break;

	     if (++have_pages >= need_pages)
	       {
		 address -= size - L4_PAGESIZE;
		 goto found;
	       }
	   }

	 address += L4_PAGESIZE;
       }
    }
  else
    {
      /* search downwards mem_high-0 */
      for (address = max_address; address >= min_address + size; )
       {
	 /* round down to next proper aligned chunk */
	 address = ((address-size) & align) + size;

	 for (have_pages = 0;; )
	   {
	     address -= L4_PAGESIZE;
	 
	     if (!quota_check_mem(owner, address, L4_PAGESIZE)
		 || memmap_owner_page(address) != O_FREE)
	       break;

	     if (++have_pages >= need_pages)
	       {
		 /* we have found a chunk which is big enough */
		 int ret, a;
       found:

		 for (a=address; a<address+size; a+=L4_PAGESIZE)
		   {
		     ret = memmap_alloc_page(a, owner);
		     assert(ret);
		   }
	     
		 return address;
	       }
	   }
       }
    }

  /* nothing found! */
  return -1;
}

extern unsigned fiasco_symbols_end;
void
show_mem_info(void)
{
#define ROUND_PAGE(x) (((x)+L4_PAGESIZE-1)&L4_PAGEMASK)
  extern struct grub_multiboot_info mb_info;
  unsigned bor, eor, owner=O_MAX;
  unsigned long eor_reserved0 = ROUND_PAGE(l4_info->reserved0.high);
  unsigned long eor_sigma0    = ROUND_PAGE(l4_info->sigma0_memory.high);
  static char ownerstr[44];
  
  printf("RMGR: Memory Dump (%ldkB total RAM, %ldkB reserved for L4 kernel)\n", 
	mb_info.mem_upper + 1024, mb_info.mem_upper+1024-mem_high/1024);
  for (bor=0, eor=0; eor<=mem_high; eor+=L4_PAGESIZE)
    {
      if ((memmap_owner_page(eor) != owner)
	 || (eor >= mem_high)
	 || (eor == 0x00001000)
	 || (eor == 0x00002000)
	 || (eor == eor_reserved0)
	 || (eor == eor_sigma0))
       {
	 if (bor != eor)
	   {
	     switch (owner)
	       {
	       case O_RESERVED:
		 if (eor == 0x00100000)
		   sprintf(ownerstr, "[BIOS adapter area]");
		 else if (bor==(unsigned)&_stext)
		   strcpy(ownerstr, "[rmgr]");
		 else if (eor==0x00001000)
		   strcpy(ownerstr, "[BIOS data area]");
		 else if (eor==0x00002000)
		   strcpy(ownerstr, "[kernel info page]");
		 else if (eor==eor_reserved0)
		   strcpy(ownerstr, "[L4 kernel]");
		 else if (eor==eor_sigma0)
		   strcpy(ownerstr, "[sigma0]");
		 else if (eor==fiasco_symbols_end)
		   strcpy(ownerstr, "[Fiasco symbols]");
		 else
		   strcpy(ownerstr, "reserved");
		 break;
	       case O_FREE:
		 strcpy(ownerstr, "free");
		 break;
	       default:
		   {
		     int i, j;
		     i = sizeof(ownerstr);
		     sprintf(ownerstr, "0x%x ", owner);
		     j = strlen(ownerstr);
		     i-=j;
		     strncpy(ownerstr+j, owner_name(owner), i);
		   }
		 break;
	       }
	     ownerstr[sizeof(ownerstr)-1]='\0';
	     printf("	   %08x-%08x (%7dkB): %s\n",
		    bor, eor, (eor-bor)/1024, ownerstr);
	     bor = eor;
	   }
	 owner = memmap_owner_page(eor);
       }
    }
}

