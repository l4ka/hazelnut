/*
 * TOPP
 */

//#define CONFIG_TIME_DEMO 1
/*
 * if enabled, the pagefault handling time will be measured via x86 MTTR
 * therefore the handling is made atomic via cli/sti
 */

#define CONFIG_TCB_DEBUG 1

#include <l4/l4.h>
#include <l4io.h>
#include <l4/helpers.h>
#include <l4/x86/kdebug.h>
#include <l4/libide.h>
#include "universe.h"
#include "sync.h"

//static dword_t pager_stack[STACK_SIZE];
static dword_t checkpointer_stack[STACK_SIZE];
static dword_t dirty_bits[MAX_MEM / PAGE_SIZE / WORD_SIZE];
static dword_t cow_bits[MAX_MEM / PAGE_SIZE / WORD_SIZE];

static volatile pa_t page_array[MAX_MEM / PAGE_SIZE];
static dword_t PAGE_COUNT;
static dword_t BITS_COUNT;
static dword_t FREE_PAGE;
static dword_t ACTIVE_CKP;

static dword_t ide_block_offset;
static dword_t ide_header_offset;
static dword_t ide_block_num;
static dword_t ide_drive;

/*
 * internal id's
 */

static l4_threadid_t pager_id, check_id;

/*
 * time measure macro
 */

#ifdef CONFIG_TIME_DEMO
#define rdtsc(x)        __asm__ __volatile__ ("rdtsc" : "=a"(x): : "edx");
#define cli             __asm__ __volatile__ ("cli":::);
#define sti             __asm__ __volatile__ ("sti":::);
#endif


#define memset(value, dest, numwords) \
      __asm__ __volatile__ ( \
                            "cld\n\t" \
                            "rep\n\t" \
                            "stosl" \
                            : "=c" (dummy), "=d" (dummy)\
			    : "a" (value), "D" (dest), "c" (numwords) )

/*
 * Some simple functions on the datastructures
 */

dword_t is_page_in (dword_t vmem, l4_threadid_t requestor)
{
  //simply searches the mapping data structures

  dword_t page1 = (vmem & PAGE_MASK | requestor.id.task);
  dword_t page2 = (vmem & PAGE_MASK | requestor.id.task) ^ (1 << 8);
  dword_t page3 = page1 ^ (1 << 11);
  dword_t page4 = page2 ^ (1 << 11);

  //if is a bit clumsy with the present bit
  for (dword_t i = 0; i < PAGE_COUNT; i++)
    if (page_array[i].vmem == page1 || page_array[i].vmem == page2 ||
	page_array[i].vmem == page3 || page_array[i].vmem == page4)
      return i;
  return INVALID_P;

}

void
dump_array ()
{
  //debug output

  dword_t k = 0;

  for (dword_t i = 0; i < PAGE_COUNT; i++)
    {

      if (page_array[i].addr.taskid != 0xff)
	{
	  printf ("%x  ", page_array[i].vmem);
	  k++;
	  if (k == 6)
	    {
	      k = 0;
	      printf ("\n");
	      printf ("%d: ", i);
	    }
	}
    }
  printf ("\n");

}

void
dump_mem (void *start, dword_t count)
{
  char *buffer2 = (char *) start;
  for (dword_t jj = 0; jj < count / 16; jj++)
    {
      printf ("%x:\t", buffer2);
      for (dword_t jjj = 0; jjj < 16; jjj++)
	{
	  printf ("%x ", *buffer2);
	  buffer2++;
	}
      printf ("\n");
    }
}

dword_t map_to (dword_t vmem, l4_threadid_t requestor)
{
  //assign a free page to a task id
  static dword_t i;
  dword_t tmp = i;
  dword_t flag = 1;
  while (1)
    {
      if (page_array[i].addr.taskid == 0xff)
	break;
      i++;
      i %= PAGE_COUNT;
      if (i == tmp)
	{
	  flag = 0;
	  break;
	}
    }
  if (flag)
    {
      page_array[i].vmem = (vmem & PAGE_MASK) | requestor.id.task;
      return i;
    }
  else
    return INVALID_P;

}

dword_t cow_page (dword_t old, l4_threadid_t requestor)
{
  //copy a page to a different location

  dword_t i = map_to ((page_array[old].vmem & PAGE_MASK), requestor);

  if (i != INVALID_P)
    {

      dword_t *to = (dword_t *) (MTAB_BASE + i * PAGE_SIZE);
      dword_t *from = (dword_t *) (MTAB_BASE + old * PAGE_SIZE);
      if (((MTAB_BASE + i * PAGE_SIZE) & PAGE_MASK) == 0x0 ||
	  ((MTAB_BASE + old * PAGE_SIZE) & PAGE_MASK) == 0x0)
	enter_kdebug ();
      memcpy (to, from, 4096);

      page_array[old].addr.taskid = 0xff;

      return i;

    }
  else
    {

      return INVALID_P;

    }
}

void
read_old ()
{
  //reads old data structures
  ide_init ();
  char buffer[512];
  ide_drive = 0;

  //searching topp-partition

  int result = ide_read (ide_drive, 0, (void *) buffer, 512);
  if (result)
    {
      enter_kdebug ("ide-read error");
    }
  partition_t *ptable = (partition_t *) (buffer + 0x000001be);
  int i = 0;
  while ((i < 4) && (ptable->sys != TOPP_SYS_CODE))
    {
      i++;
      ptable++;
#ifdef DEBUG
      printf (" %d entry: %x \n", i, ptable->sys);
#endif
    }

  if (i == 4)
    enter_kdebug ("no TOPP-Partition found");
  ide_block_num = ptable->numsec;
  ide_block_offset = ptable->start;
  printf ("found partition@%x size %x blocks\n", ide_block_offset,
	  ide_block_num);

  //reading topp-Partition

  result = ide_read (ide_drive, ide_block_offset, (void *) buffer, 512);
  partition_header_t *header = (partition_header_t *) buffer;
  if (header->magic != HEADER_MAGIC)
    {
      enter_kdebug ("uninitialised Parititon found\n");
      //nothing else to do
      return;
    }
  return;
  printf ("found %d used entries\n", header->used_entries);

  ide_header_offset = header->addr_first_block;
  if (header->used_entries < PAGE_COUNT)
    enter_kdebug ("not enough pages to load checkpoint");

  //use either full amount of entries or the first PAGE_COUNT ones
  int end_of_header = (header->used_entries < PAGE_COUNT) ?
    header->used_entries : PAGE_COUNT;
  printf ("found %d entries starting blocks at %x\n",
	  header->used_entries, header->addr_first_block);

  //reading old mappings
  int offset = sizeof (partition_header_t);
  int block_count = 0;
  int count = offset;
  for (int k = offset; k < end_of_header + offset; k++)
    {
      if (count == 128)
	{
	  count = 0;
	  block_count++;
	  result = ide_read (ide_drive, ide_block_offset + block_count,
			     (void *) buffer, 512);
	  if (result)
	    enter_kdebug ("ide-read error");
	}
      page_array[k - offset].vmem = buffer[count];
      count++;
    }

  //reset present bit to 0 on all vmem's

  for (int i = 0; i < MAX_MEM / PAGE_SIZE; i++)
    {
      if (page_array[i].addr.taskid != 0xff)
	page_array[i].addr.present = 0;
    }
}

void
write_mapping ()
{
  //writes mapping to disk
  dword_t buffer[128];
  partition_header_t *header = (partition_header_t *) buffer;
  header->used_entries = PAGE_COUNT;
  header->magic = HEADER_MAGIC;
  header->addr_first_block = ide_block_offset + HEADER_LENGTH;
  dword_t count = sizeof (partition_header_t);
  dword_t block_count = 0;
  //  dump_array();
  for (dword_t i = 0; i < PAGE_COUNT; i++)
    {
      if (count != 128)
	{
	  buffer[count] = page_array[i].vmem & (~(0xf << 8));
	  count++;
	}
      else
	{
	  count = 0;
	  ide_write (ide_drive, ide_block_offset + block_count,
		     (void *) buffer, 512);
	  block_count++;
	}
    }
  for (; count < 128; count++)
    buffer[count] = 0xffffffff;
  ide_write (ide_drive, ide_block_offset + block_count, (void *) buffer, 512);

}

inline void
read_page (dword_t i)
{
  //loads a single 4 k page from disk
  dword_t page = i * PAGE_SIZE + MTAB_BASE;
  int result = ide_read (ide_drive,
			 (i * (PAGE_SIZE / BLOCK_SIZE)) + ide_header_offset +
			 HEADER_LENGTH, (void *) page, PAGE_SIZE);
  if (result)
    enter_kdebug ("ide-read error");
  page_array[i].addr.present = 1;
}

inline void
write_page (dword_t i)
{
  //writes a single 4 k page from disk
  dword_t page = i * PAGE_SIZE + MTAB_BASE;
  int result = ide_write (ide_drive,
			  (i * (PAGE_SIZE / BLOCK_SIZE)) + ide_header_offset +
			  HEADER_LENGTH, (void *) page, PAGE_SIZE);
  if (result)
    enter_kdebug ("ide-write error");

}

/*
 * pager thread
 *
 * no exception handling, yet
 */

void
pager_thread ()
{

  dword_t dw0, dw1, dw2 = 0;
  l4_threadid_t requestor, req = L4_NIL_ID;
  l4_msgdope_t result;
  l4_fpage_t fpage;
  fpage.fpage = 0;
  fpage.fp.write = 1;
  fpage.fp.size = PAGE_BITS;

  dword_t fpage_base = 0;
  dword_t snd_msg = 0;

  printf ("waiting for PF-IPC\n");
  l4_ipc_wait (&requestor, NULL, &dw0, &dw1, &dw2, L4_IPC_NEVER, &result);
#ifdef CONFIG_TIME_DEMO
  dword_t in, out;
  dword_t flag = 0;
#endif
  
  while (1)
    {
#ifdef CONFIG_TIME_DEMO
      rdtsc (in);
#endif
      
#ifdef CONFIG_TCB_DEBUG
      printf ("received pagefault from %x@%x\n", requestor, dw0);
      dump_array ();
#endif
      if (result.raw)
	{
	  printf("raw result of IPC %x\n", result.raw);
	  enter_kdebug ("error on PF-IPC: ");
	}
      
      if (requestor == check_id)
	{
	  //shutdown paging for checkpointing
	  l4_ipc_call (check_id, 0, 42, 0, 0, NULL, &dw0, &dw1, &dw2,
		       L4_IPC_NEVER, &result);
	  requestor = L4_NIL_ID;
	  printf ("waiting for PF-IPC\n");
	  l4_ipc_wait (&requestor, NULL, &dw0, &dw1, &dw2,
		       L4_IPC_NEVER, &result);
#ifdef CONFIG_TCB_DEBUG
	  printf ("received pagefault from %x@%x\n", requestor, dw0);
	  dump_array ();
#endif
	}
      else
	{
	  
	  req.raw = requestor.raw;
	  if (dw0 > TCB_AREA && dw0 < (TCB_AREA + TCB_AREA_SIZE))
	    {
	      requestor.raw = 0;
	      requestor.id.task = 0x3;
#ifdef CONFIG_TCB_DEBUG
	      printf ("map into TCB-AREA\n");
#endif
	    }
	  fpage_base = dw0 & PAGE_MASK;
	  dword_t i = is_page_in (dw0, requestor);
#ifdef CONFIG_TCB_DEBUG
	  printf ("%x: used entry: %x\n", i, page_array[i].vmem);
#endif
	  if (i != INVALID_P)
	    {
#ifdef CONFIG_TCB_DEBUG
	      printf ("found used page\n");
#endif
	      snd_msg = 2;
	      
	      //if page on disk, then read from disk
	      if (!page_array[i].addr.present)
		{
#ifdef CONFIG_TCB_DEBUG
		  printf ("page not present\n");
#endif
		  read_page (i);
		  page_array[i].addr.present = 1;
		}
	      
	      //remap page writeable  
	      if (ACTIVE_CKP
		  && (cow_bits[i / WORD_SIZE] & (1 << (i % WORD_SIZE))))
		{
		  //copy page somewhere else if currently checkpointing
#ifdef CONFIG_TCB_DEBUG
		  printf ("making page %x cow\n", i);
#endif
		  dword_t next = cow_page (i, requestor);
#ifdef CONFIG_TIME_DEMO
		  flag = 1;
#endif
		  if (next != INVALID_P)
		    {
#ifdef CONFIG_TCB_DEBUG
		      printf ("move page %x -> %x\n", i, next);
#endif
		      l4_fpage_t unmap;
		      unmap.fpage = ((i * PAGE_SIZE + MTAB_BASE) | (12 << 2));
		      while (lock_page ((void *) (page_array + i)))
			{
#ifdef CONFIG_TCB_DEBUG
			  printf ("pager caught in %x\n", i);
#endif
			  l4_yield ();
			}
		      l4_fpage_unmap (unmap, 2);
		      unlock_page ((void *) (page_array + i));
		      i = next;
		      page_array[i].addr.present = 1;
		    }
		  else
		    enter_kdebug ("no free page found");
		  
		}
	      
	      fpage.fpage = i * PAGE_SIZE + MTAB_BASE;
	      
	    }
	  else
	    {
	      //map empty page
	      
	      i = map_to (dw0, requestor);
	      page_array[i].addr.present = 1;
#ifdef CONFIG_TCB_DEBUG
	      printf ("map zero page\n");
#endif
	      if (i == INVALID_P)
		{
		  enter_kdebug ("no more free pages");
		  snd_msg = 0;
		}
	      else
		{
		  snd_msg = 2;
		  fpage.fpage = i * PAGE_SIZE + MTAB_BASE;
		  //zero fill
		  dword_t *dest = (dword_t *) fpage.fpage;
		  dword_t dummy;
		  memset (0, dest, 1024);
#ifdef CONFIG_TCB_DEBUG
		  printf ("deleting page %x\n", i);
#endif
		}
	      
	    }
	  
	  //toggle dirty bit
	  dirty_bits[i / WORD_SIZE] =
	    dirty_bits[i / WORD_SIZE] | (1 << (i % WORD_SIZE));
	  printf("dirty: %x\n", &dirty_bits[i / WORD_SIZE]);
	  fpage.fp.write = 1;
	  fpage.fp.grant = 0;
	  fpage.fp.zero = 0;
	  fpage.fp.size = PAGE_BITS;
	  if (snd_msg)
	    {
	      dw1 = fpage.fpage;
	      dw0 = fpage_base;
	    }
	  else
	    dw0 = dw1 = 0;
	  dw2 = 0;
#ifdef CONFIG_TCB_DEBUG
	  printf ("sending back fpage: %x base: %x to %x\n", dw1, dw0, req.raw);
#endif
	  
#ifdef CONFIG_TIME_DEMO
	  rdtsc (out);
	  out = (out - in) / 2;
	  if (flag)
	    printf ("%d \tCOPY!!!\n", out);
	  else
	    printf ("%d \n", out);
	  flag = 0;
#endif
	  
	  /*
	   * still missing the check whether receiving tcb is write protected
	   */
	  
	  l4_ipc_reply_and_wait (req, (dword_t *) snd_msg, dw0, dw1, dw2,
				 &requestor, NULL, &dw0, &dw1, &dw2,
				 L4_IPC_NEVER, &result);
	  while(result.raw) 
	    l4_ipc_wait (&requestor, NULL, &dw0, &dw1, &dw2, L4_IPC_NEVER, &result);
	}
    }
}

/*
 * checkpoint thread
 */

void
checkpoint_thread ()
{
  dword_t mtab_bit = PAGE_BITS;
  dword_t dw0, dw1, dw2;
  l4_msgdope_t result;
  while (((dword_t) (0x00000001 << mtab_bit)) < ((PAGE_SIZE * PAGE_COUNT)))
    mtab_bit++;
  l4_fpage_t fpage_all;
  fpage_all.fpage = MTAB_BASE | (mtab_bit << 2);
#ifdef CONFIG_TCB_DEBUG
  int k = 0;
#endif

  while (1)
    {
      //l4_ipc_sleep(255, 5); // ~4min
      l4_sleep (mills (1000));
      printf ("stopping pager\n");
      l4_ipc_call (pager_id, 0, 0, 0, 0, NULL, &dw0, &dw1, &dw2,
		   L4_IPC_NEVER, &result);
      printf ("pager stoped\n");
      ACTIVE_CKP = 0xffffffff;

#ifdef CONFIG_TCB_DEBUG
      printf ("start checkpointing ...\n");
      dump_array ();
      //      enter_kdebug ();
#endif

      // unmap all pages
      l4_fpage_unmap (fpage_all, 0);	//affect only other AS's

      // copy and reset dirty_bits
      for (dword_t i = 0; i < PAGE_COUNT / WORD_SIZE; i++)
	{
	  cow_bits[i] = dirty_bits[i];
	  dirty_bits[i] = 0;
	}

      //write back mappings
      write_mapping ();

      //wake up pager
      printf ("waking up pager\n");
      l4_ipc_send (pager_id, 0, dw0, dw1, dw2, L4_IPC_TIMEOUT_NULL, &result);
      printf ("woke up pager\n");
#ifdef CONFIG_TIME_DEMO
      //sleep for measurements
      l4_sleep (mills (1));
#else
      //write back dirty pages
      printf ("write now used pages\n");
      dword_t p_c = 0;
      dword_t p_s = 0;
      for (dword_t i = 0; i < PAGE_COUNT / WORD_SIZE; i++)
	{
	  if (cow_bits[i])
	    {
	      dword_t j = 0;
	      dword_t jj = 0;
	      do
		{
		  if (cow_bits[i] & (0x00000001 << j))
		    {
		      jj = i * WORD_SIZE + j;
		      while (lock_page ((void *) (page_array + jj)))
			{
			  printf ("checkpointer caught in %x", jj);
			  p_s++;
			  l4_yield ();
			}
		      write_page (jj);
		      p_c++;
		      unlock_page ((void *) (page_array + jj));
		      cow_bits[i] = cow_bits[i] & (~(1 << j));
		    }
		  j++;
		}
	      while (cow_bits[i]);
	    }
	}
#endif
      //clean up
      ACTIVE_CKP = 0;
#ifdef CONFIG_TCB_DEBUG
      printf ("Checkpoint on disk: %d pages %d locks\n", p_c, p_s);
#endif
    }
}

// lthread0
int
main (void)
{
  l4_threadid_t mypagerid, myid, preempt_id;
  dword_t dw0, dw1, dw2;
  l4_msgdope_t result;
  unsigned int dummyint;

  // get id of my pager  

  myid = l4_myself ();
  preempt_id = mypagerid = L4_INVALID_ID;
  l4_thread_ex_regs (myid, 0xffffffff, 0xffffffff, &preempt_id, &mypagerid,
		     &dummyint, &dummyint, &dummyint);

  // get kernel info page

  l4_ipc_call (mypagerid, 0, 1, 1, 0, (void *) (INFO_BASE + 0x32),
	       &dw0, &dw1, &dw2, L4_IPC_NEVER, &result);
  kernel_info_page_t *infopage = (kernel_info_page_t *) INFO_BASE;
  if (infopage->magic[0] != 'L' ||
      infopage->magic[1] != '4' ||
      infopage->magic[2] != 0xe6 || 
      infopage->magic[3] != 'K')
    enter_kdebug ("unreadable kernel info page");
  printf ("%dMB Memory reported by kernel\n",
	  (infopage->main_mem_high / 1048576));
  dword_t pmem_base =
    ((infopage->main_mem_high / PAGE_SIZE) * (100 - PERCENTAGE)) / 100 *
    PAGE_SIZE;
  //((infopage->main_mem_high/100)*(100-PERCENTAGE))&PAGE_MASK;
  PAGE_COUNT = (infopage->main_mem_high - pmem_base) / PAGE_SIZE;
  pmem_base = infopage->main_mem_high - PAGE_COUNT * PAGE_SIZE;
  BITS_COUNT = PAGE_COUNT / WORD_SIZE;
  FREE_PAGE = 0;

  // init the paging structures
  for (dword_t k = 0; k < BITS_COUNT; k++)
    {
      dirty_bits[k] = cow_bits[k] = 0;
    }

  // get the uppermost PERCENTAGE of pages from sigma0

  dword_t pmap = pmem_base;
  dword_t mapbase = (MTAB_BASE | 0x32);
#ifdef CONFIG_TCB_DEBUG
  printf ("grabing %x pages from s0\n", PAGE_COUNT);
  printf ("start address %x\n", pmem_base);
#endif
  for (dword_t k = 0; k < PAGE_COUNT; k++)
    {
      // request arbitrary pages one after another pmap+0x2
      l4_ipc_call (mypagerid, 0, 0xfffffffc, 0x00400300, 0,
		   (void *) mapbase, &dw0, &dw1, &dw2, L4_IPC_NEVER, &result);
      // if no page returned
      if (!dw0 & !dw1)
	page_array[k].vmem = 0x0;
      else
	{
	  page_array[k].vmem = 0xFFFFFFFF;
	  FREE_PAGE++;
	  mapbase += PAGE_SIZE;
	}
      pmap += PAGE_SIZE;
    }

  // loading of old datastructures from disk
#ifndef CONFIG_TIME_DEMO
  read_old ();
#endif

  // start pager thread
  pager_id.raw = myid.raw;
  //  pager_id.id.thread++;
  l4_threadid_t tmp = mypagerid;
  enter_kdebug ();
  //  l4_thread_ex_regs(pager_id, (dword_t) pager_thread, 
  //                (dword_t) &(pager_stack[STACK_SIZE-1]), 
  //                &preempt_id, &mypagerid, &dummyint, &dummyint, &dummyint);

  // start checkpoint thread
  check_id.raw = myid.raw;
  check_id.id.thread += 1;
  l4_thread_ex_regs (check_id, (dword_t) checkpoint_thread,
		     (dword_t) & (checkpointer_stack[STACK_SIZE - 1]),
		     &preempt_id, &mypagerid, &dummyint, &dummyint,
		     &dummyint);
  mypagerid = tmp;
  pager_thread ();
  //  while(1) l4_yield();
  return 1;
  //checkpoint_thread();
}
