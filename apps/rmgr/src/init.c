/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <flux/machine/multiboot.h>

#include <l4/l4.h>

#include "config.h"
#include "memmap.h"
#include "globals.h"
#include "exec.h"
#include "rmgr.h"
#include "cfg.h"

#include <l4/rmgr/rmgr.h>
#include "init.h"

struct multiboot_module mb_mod[MODS_MAX];
struct vbe_controller mb_vbe_cont;
struct vbe_mode mb_vbe_mode;

static void init_globals(void);
static void init_memmap(void);
#if defined(CONFIG_IO_FLEXPAGES)
static void init_iospace(void);
#endif
static void init_irq(void);
static void init_task(void);
static void init_small(void);
static void configure(void);
static void start_tasks(void);

static void setup_task(l4_threadid_t t);
static int overlaps_myself(vm_offset_t begin, vm_offset_t end);
static int alloc_exec_read_exec(void *handle,
				vm_offset_t file_ofs, vm_size_t file_size,
				vm_offset_t mem_addr, vm_size_t mem_size,
				exec_sectype_t section_type);
static int alloc_exec_read(void *handle, vm_offset_t file_ofs,
			   void *buf, vm_size_t size,
			   vm_size_t *out_actual);
static int set_small(l4_threadid_t t, int num);
static int set_prio(l4_threadid_t t, int num);


int boot_errors = 0;	/* no of boot errors */
int boot_wait = 0;	/* wait after boot? */
int error_stop = 0;	/* wait for keypress on boot error? */
int hercules = 0;	/* startup output on hercules? */
int boot_mem_dump = 0;	/* dump memory at startup time? */
int kernel_running = 0;	/* kernel already running? if yes, we use outchar */
int last_task_module = 0; /* idx of last module to be booted - can be
			     controlled via the '-mods=<n>' command
			     line parameter */
int l4_version = 0;

bootquota_t bootquota[TASK_MAX];


const char*
mb_mod_name(int i)
{
  return (i<0 || i>=MODS_MAX || !mb_mod[i].string)
    ? "[unknown]"
    : (char *)mb_mod[i].string;
}

const char*
owner_name(owner_t owner)
{
  return mb_mod_name(owner-myself.id.task-1+first_task_module);
}

/* a boot error occured */
void
boot_error(void)
{
  if (error_stop)
    {
      char c;
      printf("\r\nRMGR: boot error. "
	     "Return continues, Esc panics, \"m\" shows memory map");
      c = getchar();
      printf("\r%60s\r","");
      switch (c)
	{
	case 0x1B:
	  panic("RMGR: boot error");
	case 'm':
	case 'M':
	  show_mem_info();
	  break;
	}
    }
  boot_errors++;
}

/* a fatal boot error occured, don't allow to continue */
void
boot_fatal(void)
{
  for (;;)
    {
      char c;
      printf("\r\nRMGR: Fatal boot error. "
	     "Esc panics, \"m\" shows memory map");
      for (;;)
	{
	  c = getchar();
	  switch (c)
	    {
	    case 0x1B:
	      printf("\r%70s\r","");
	      panic("RMGR: fatal boot error");
	    case 'm':
	    case 'M':
	      printf("\r%70s\r","");
	      show_mem_info();
	      break;
	    default:
	      continue;
	    }
	  break;
	}
    }
}

static void
boot_panic(const char *fmt, ...)
{
  for (;;)
    {
      char c;
      va_list args;
      va_start(args, fmt);
      vprintf(fmt, args);
      va_end(args);
      printf("\r\nRMGR: Return panics, \"m\" shows memory map");
      c = getchar();
      printf("\r%60s\r","");
      switch (c)
	{
	case 0x0D:
	  panic("RMGR: panic");
	case 'm':
	case 'M':
	  show_mem_info();
	  break;
	}
    }
}

/* started as the L4 booter task through startup() in startup.c */
void init(void)
{
  vm_offset_t address;

  printf("RMGR: Hi there!\n");

  init_globals();

  init_memmap();

#if defined(CONFIG_IO_FLEXPAGES)
  /* Request I/O Space from sigma0 */
  init_iospace();
#endif
  init_irq();

  init_task();

  init_small();

  configure();

  setup_task(myself);
  setup_task(my_pager);

  /* start the tasks loaded as modules */
  start_tasks();

  /* add the memory used by this module and not any longer required
     (the ".init" section) to the free memory pool */
  for (address = (vm_offset_t) &_start & L4_PAGEMASK;
       address < ((vm_offset_t) &_stext & L4_PAGEMASK);
       address += L4_PAGESIZE)
    {
      check(memmap_free_page(address, O_RESERVED));
    }

  if (boot_errors || boot_wait)
    {
      char c;

      if (boot_errors)
	printf("RMGR: WARNING: there were %d boot errors -- continue?\n"
	       "               Return continues, Esc panics, "
	       "\"k\" enters L4 kernel debugger...\n",
	       boot_errors);
      else
	printf("RMGR: bootwait option was set -- please press a key\n"
	       "      Return continues, Esc panics, "
	       "\"k\" enters L4 kernel debugger...\n");
      c = getchar();

      if (c == 27)
	panic("RMGR: boot error");
      else if (c == 'k' || c == 'K')
	enter_kdebug("boot error");
    }


  /* now start the resource manager */
  rmgr();
}

/* support functions for init() */

static void configure(void)
{
  char *config, *config_end;

  cfg_init();

  /* find the config data */
  if (! (mb_info.flags & MULTIBOOT_CMDLINE))
    return;

  config = strchr((char *) mb_info.cmdline, ' ');
  if (! config)
    return;

  config++;			/* skip ' ' character */
  if (!strstr(config, "-configfile"))
    {
      /* not using a configfile -- use the cmdline */
      config_end = config + strlen(config);

      /* skip options */
      while (config < config_end)
	{
	  if (*config == ' ')	/* skip spaces */
	    {
	      config++;
	      continue;
	    }

	  if (*config != '-')	/* end of options found */
	    break;

	  config = strchr(config, ' ');	/* find end of this option */
	  if (!config)
	    return;
	}
    }
  else
    {
      /* the configfile is in the first module... */

      /* make sure the config file is not the last module */
      check(mb_info.mods_count >= first_task_module + 2);

      config = (char *) mb_mod[first_task_module].mod_start;
      config_end = (char *) mb_mod[first_task_module].mod_end;

      /* is it really a config file? */
      if (strncmp(config, "#!rmgr", 6) != 0)
	{
	  printf("RMGR: ERROR: 2nd module is not a config file "
		 "(doesn't start with \"#!rmgr\")\n");
	  boot_error();
	  return;
	}

      first_task_module++;
    }

  cfg_setup_input(config, config_end);

  if (cfg_parse() != 0)
    boot_error();

  printf("RMGR: log_mask: %x, log_types: %x\n",
	 debug_log_mask, debug_log_types);

  /* a few post-parsing actions follow... */
  if (small_space_size)
    {
      vm_offset_t s = small_space_size;

      small_space_size /= 0x400000; /* divide by 4MB */
      if (s > small_space_size * 0x400000 /* did we delete any size bits? */
	  || (small_space_size & (small_space_size - 1))) /* > 1 bit set? */
	{
	  /* use next largest legal size */
	  int count;
	  for (count = 0; small_space_size; count++)
	    {
	      small_space_size >>= 1;
	    }

	  small_space_size = 1L << count;
	}
      
      if (small_space_size > 64)
	{
	  printf("RMGR: ERROR: small_space_size 0x%08x too large\n", s);
	  small_space_size = 0;
	  boot_error();
	}

      else
	{
	  unsigned i;

	  for (i = 1; i < 128/small_space_size; i++)
	    check(small_free(i, O_RESERVED));

	  printf("RMGR: configured small_space_size = %d MB\n",
		 small_space_size * 4);
	}
    }
}

static void setup_task(l4_threadid_t t)
{
  unsigned me = t.id.task;
  bootquota_t *b = & bootquota[me];

  if (b->small_space != 0xff)
    {
      if (small_space_size)
	set_small(t, b->small_space);
      else
	b->small_space = 0xFF;
    }

  set_prio(t, b->prio);
}

static void init_globals(void)
{
  dword_t dummy;

  /* set some globals */

  /* set myself (my thread id) */
  myself = rmgr_pager_id = rmgr_super_id = l4_myself();
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;
  rmgr_super_id.id.lthread = RMGR_LTHREAD_SUPER;

  /* set my_pager */
  my_preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(myself, (dword_t) -1, (dword_t) -1, 
		    &my_preempter, &my_pager, &dummy, &dummy, &dummy);

  /* set mem size */
  mem_upper = mb_info.mem_upper;
  mem_lower = mb_info.mem_lower & ~3; /* round down to next 4k boundary */

  /* more global initialization stuff */
  small_space_size = 0;

  debug = verbose = debug_log_mask = debug_log_types = 0;

#if 0
  if (no_pentium == 1)
    printf("RMGR: running on L4/486\n");
  else if (no_pentium == 0)
    printf("RMGR: running on L4/Pentium\n");
  else {
    printf("RMGR: unknown L4 version, assuming L4/486\n");
    no_pentium = 1;
  }
#else
  printf("RMGR: running on %s\n", KERNEL_NAMES[l4_version]);
#endif
  

  if (mb_info.flags & MULTIBOOT_CMDLINE)
    {
      /* check for -nopentium option */
      if (strstr((char *) mb_info.cmdline, "-nopentium"))
	{
	  no_pentium=1;
	  printf("no l4/pentium\n");
	}
      /* check for -nocheckdpf option */
      if (strstr((char *) mb_info.cmdline, "-nocheckdpf"))
	{
	  no_checkdpf=1;
	  printf("don't check double page faults\n");
	}
      /* check for -errorstop */
      if (strstr((char *) mb_info.cmdline, "-errorstop"))
	{
	  error_stop=1;
	}
      /* check for -memdump */
      if (strstr((char *) mb_info.cmdline, "-memdump"))
	{
	  boot_mem_dump=1;
	}
    }
  quota_init();
}
#if defined(CONFIG_IO_FLEXPAGES)
/* Request I/O Space from sigma0 */
static void init_iospace(void){

	     l4_fpage_t foo_fp;
	     l4_msgdope_t res;
	     dword_t dw0;
	     
             /* the address of the receive window doesn't matter actually, but should not be 0 */
	     foo_fp = l4_fpage((int) 0xF0000000, 0, 1, 0);
	     
	     dw0 = 0xF0000040;
	     
	     l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG, 
				  dw0, dw0,
				  (void *) foo_fp.fpage, 
				  &dw0, &dw0, 
				  L4_IPC_NEVER, &res);


}
#endif

static void init_irq(void)
{
  int err, i;
  unsigned *sp;
  dword_t code, dummy;
  l4_msgdope_t result;
  l4_threadid_t t, preempter, pager;

  /* initialize to "reserved" */
  irq_init();

  printf("RMGR: attached irqs = [ ");

  /* start a thread for each irq */
  t = myself;
  
  for (i = 0; i < IRQ_MAX; i++)
    {
      t.id.lthread = LTHREAD_NO_IRQ(i);
      sp = (unsigned *)(__irq_stacks + (i + 1) * __IRQ_STACKSIZE);

      *--sp = i;		/* pass irq number as argument to thr func */
      *--sp = 0;		/* faked return address */
      preempter = my_preempter; pager = my_pager;
      l4_thread_ex_regs(t, (dword_t) __irq_thread, (dword_t) sp,
                        &preempter, &pager, &dummy, &dummy, &dummy);

      /* handshake and status check */
      err = l4_i386_ipc_receive(t, L4_IPC_SHORT_MSG, &code, &dummy,
                                L4_IPC_NEVER, &result);

      assert(err == 0);
      
      if (i == 2)
        {
          /* enable IRQ2 */
          __asm__("inb $0x21,%al\n"\
                  "jmp 1f\n"\
                  "1:\tjmp 1f\n"\
                  "1:\tandb $0xfb,%al\n"\
                  "outb %al,$0x21\n");
        }
      
      if (code == 0)
        {
          if (i != 2) 
	    irq_free(i, O_RESERVED); /* free irq, except if irq2 */
          printf("%x ", i);
        }
      else
	printf("<!%x> ", i);
    }
  printf("]\n");
}

static void init_task(void)
{
  unsigned i;

  task_init();

  for (i = myself.id.task + 1; i < TASK_MAX; i++)
    __task[i] = O_FREE;
  
  first_not_bmod_free_modnr = mb_info.mods_count;
  first_not_bmod_free_task  = first_not_bmod_task
    = myself.id.task + 1
    + (mb_info.mods_count-first_task_module);
}

static void init_small(void)
{
  small_init();
}


static void init_memmap(void)
{
  l4_msgdope_t result;
  l4_snd_fpage_t sfpage;
  vm_offset_t address, a;
  vm_size_t free_size = 0, reserved_size = 0;
  int error;
  int ignore_high_pages = 0;

  /* initialize to "reserved" */
  memmap_init();

  /* invalidate the ``last page fault'' array */
  for (a = 0; a < TASK_MAX; a++)
    {
      last_pf[a].pfa = -1;
      last_pf[a].eip = -1;
    }

  mem_high = 0;

  if (!no_pentium) 
    {
      /* get as much super (4MB) pages as possible, starting with
	 superpage 1 (0x400000) */
      for (address = L4_SUPERPAGESIZE; address < MEM_MAX; 
	   address += L4_SUPERPAGESIZE)
	{
	  error = l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
				   address | 1, L4_LOG2_SUPERPAGESIZE << 2,
				   L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
				   &sfpage.snd_base, &sfpage.fpage.fpage,
				   L4_IPC_NEVER, &result);
	  /* XXX should i check for errors? */

	  if (! (l4_ipc_fpage_received(result) 
		 && l4_ipc_is_fpage_writable(sfpage.fpage)
		 && sfpage.fpage.fp.size == L4_LOG2_SUPERPAGESIZE))
	    {
	      vm_size_t size;

	      if (!l4_ipc_fpage_received(result))
		break;

	      size = 1L << sfpage.fpage.fp.size;
	      assert(size == (size & L4_PAGEMASK));

	      for (; size > 0; size -= L4_PAGESIZE, address += L4_PAGESIZE)
		{
		  free_size += L4_PAGESIZE;
		  memmap_free_page(address, O_RESERVED);
		}

	      break;
	    }

	  /* kewl, we've received an super awsum page */
	  free_size += L4_SUPERPAGESIZE;

	  for (a = address; a < address + L4_SUPERPAGESIZE; a += L4_PAGESIZE)
	    {
	      memmap_free_page(a, O_RESERVED);
	    }

	  if (mem_high < (a - L4_PAGESIZE)) /* a == end of superpage */
	    mem_high = a;

	  assert(memmap_owner_superpage(address) == O_FREE);
	}
    }
  /* page in BIOS data area page explicitly.  like adapter space it will be
     marked "reserved" and special-cased in the pager() in memmap.c */
  error = l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
			   0, 0,
			   L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
			   &sfpage.snd_base, &sfpage.fpage.fpage,
			   L4_IPC_NEVER, &result);
  /* XXX should i check for errors? */

  /* now add all memory we can get from L4 to the memory map as free
     memory */
  for (;;)
    {
      error = l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
			       0xfffffffc, 0,
			       L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
			       &sfpage.snd_base, &sfpage.fpage.fpage,
			       L4_IPC_NEVER, &result);
      /* XXX should i check for errors? */

      if (! sfpage.fpage.fpage)
	break;			/* out of memory */

      if (sfpage.snd_base >= MEM_MAX)
	{
	  if (!ignore_high_pages)
	    {
	      ignore_high_pages = 1;
	      printf("RMGR: WARNING: all memory above %ld MB is wasted!\n",
		     MEM_MAX/(1024*1024));
	      boot_error();
	    }
	  continue;
	}

      if (mem_high < sfpage.snd_base + L4_PAGESIZE) 
	mem_high = sfpage.snd_base + L4_PAGESIZE;

      if (memmap_owner_page(sfpage.snd_base) == O_FREE)
	{
	  printf("RMGR: WARNING: we've been given page 0x%08x twice!\n",
		 sfpage.snd_base);
	  continue;
	}

      free_size += L4_PAGESIZE;
      memmap_free_page(sfpage.snd_base, O_RESERVED);
    }

  assert(mem_high > 0x200000);	/* 2MB is the minimum for any useful work */

  /* page in memory we own and we reserved before booting; we need to
     do this explicitly as the L4 kernel won't hand it out voluntarily
     (see L4 rerefence manual, sec. 2.7).
     don't care for overlaps with our own area here because we will
     reserve our memory below */
  for (address = mod_range_start & L4_PAGEMASK;
       address <= mod_range_end;
       address += L4_PAGESIZE)
    {
      /* check if we really can touch the memory */
      if (address > mem_high)
	{
	  printf("RMGR: Can't touch memory at %08x for boot modules. ", 
                 address);
	  if (address > mem_phys)
	    printf("There is no RAM\n" 
		   "      at this address.\n");
	  else
	    {
	      printf("This error occurs because\n"
		     "      the RAM at this address is owned by the kernel.");
	      if (l4_version == VERSION_FIASCO)
		printf(" Fiasco occupies 20%% of the\n"
		       "      physical memory per default.");
	      printf("\n");
	    }
	  boot_fatal();
	  break;
	}
      
      /* force write page fault */
      /* * (volatile int *) address |= 0;  */
      asm volatile ("orl $0, (%0)" 
		    : /* nothing out */
		    : "r" (address)
		    );
      if (memmap_owner_page(address) != O_FREE)
	{
	  memmap_free_page(address, O_RESERVED);
	  free_size += L4_PAGESIZE;
	}
    }

  /* page in adapter space explicitly.  the adapter space will be
     marked "reserved" and special-cased in the pager() in memmap.c */
  for (address = mem_lower * 1024L;
       address < 0x100000L;
       address +=  L4_PAGESIZE)
    {
      error = l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
			       address, 0,
			       L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
			       &sfpage.snd_base, &sfpage.fpage.fpage,
			       L4_IPC_NEVER, &result);
      /* XXX should i check for errors? */
    }

  /* reserve memory for ourselves; page in our own memory en-passant */
  for (address = ((vm_offset_t) (&_start)) & L4_PAGEMASK;
       address <= (vm_offset_t) &_end;
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, O_RESERVED);
      reserved_size += L4_PAGESIZE;

      /* force write page fault */
      /* * (volatile int *) address |= 0;  */
      asm volatile ("orl $0, (%0)" 
		    : /* nothing out */
		    : "r" (address)
		    );
    }

  /* Reserve memory for Fiasco symbols.
   * Move it to the upper memory limit to prevent memory fragmentation */
  if (fiasco_symbols)
    {
      vm_size_t size = fiasco_symbols_end - fiasco_symbols;
      int i = 0, pages = size / L4_PAGESIZE;
      
      for (address = mem_high; address > 0 && i < pages; address -= L4_PAGESIZE)
	{
	  if (memmap_owner_page(address) != O_FREE)
	    i=0;
	  else
	    if (++i >= pages)
	      break;
	}
      
      if (!address)
	{
	  printf("No space for symbols found, disabling Fiasco symbols\n");
	  fiasco_symbols = 0;
	}
      else
	{
	  memcpy((void*)address, (void*)fiasco_symbols, size);
	  fiasco_symbols     = address;
	  fiasco_symbols_end = address+size;
	  for (i=0; i<pages; i++, address += L4_PAGESIZE)
	    {
	      memmap_alloc_page(address, O_RESERVED);
	      reserved_size += L4_PAGESIZE;
	    }
	  asm ("int $3 ; cmpb $30,%%al" 
               : : "a"(fiasco_symbols) /* offset symbols */, 
	       "b"(0)              /* task number */,
	       "c"(1)              /* 1=symbols, 2=lines */);
	}
    }

  /* reserve address 0x1000 for the kernel info page */
  /* we assume the L4 kernel is loaded to 0x1000, creating a virtual
     memory area which doesn't contain mapped RAM so we can use it as
     a scratch area.  this is at least consistent with the L4
     reference manual, appendix A.  however, we flush the page anyway
     before attempting to map something in there, just in case. */
  l4_info = (l4_kernel_info_t *) 0x1000;
  l4_fpage_unmap(l4_fpage((dword_t) l4_info, L4_LOG2_PAGESIZE, 0, 0),
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
  error = l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
			   1, 1,
			   L4_IPC_MAPMSG((dword_t) l4_info, L4_LOG2_PAGESIZE),
			   &sfpage.snd_base, &sfpage.fpage.fpage,
			   L4_IPC_NEVER, &result);
  if (error)
    panic("RMGR: can't map kernel info page: IPC error 0x%x\n", error);
  memmap_alloc_page((dword_t) l4_info, O_RESERVED);
  reserved_size += L4_PAGESIZE;

  assert(l4_info->magic == L4_KERNEL_INFO_MAGIC);

  /* the non-RAM high-memory superpages are all free for allocation */
  for (address = 0x8000; address < 0x10000;
       address += L4_SUPERPAGESIZE/0x10000) /* scaled by 0x10000**-1 to
					       prevent overflow */
    {
      memmap_free_superpage(address * 0x10000, O_RESERVED);
    }


  printf("RMGR: total RAM size = %6ld KB (reported by bootloader)\n"
	 "              received %6ld KB RAM from sigma0\n"
	 "                       %6ld KB reserved for RMGR\n", 
	 mb_info.mem_upper + mb_info.mem_lower,
	 free_size/1024L, reserved_size/1024L);

  return;
}

#define alloc_from_page(pageptr, size)				\
({								\
  vm_offset_t ret;						\
  vm_offset_t new_pageptr = ((pageptr) + (size) + 3) & ~3;	\
  if (((pageptr) & L4_PAGEMASK) != (new_pageptr & L4_PAGEMASK))	\
    ret = 0;							\
  else								\
    {								\
      ret = (pageptr);						\
      (pageptr) = new_pageptr;					\
    }								\
  ret;								\
})

static void start_tasks(void)
{
  unsigned i,i2;
  l4_threadid_t t;
  exec_info_t exec_info;
  int exec_ret;
  struct exec_task e;
  const char *name;
  vm_offset_t address;
  struct grub_multiboot_info *mbi;
  struct vbe_mode *mbi_vbe_mode;
  struct vbe_controller *mbi_vbe_cont;
  dword_t *sp, entry;

  t = myself;
/*   t.id.nest++; */
  t.id.chief = t.id.task;
  
  if (last_task_module == 0)
    last_task_module = mb_info.mods_count;
    
  printf ("RMGR: will load modules %d to %d (can be limited via '-mods=<n>')\n", first_task_module, (last_task_module - 1));

  i2 = first_task_module;
  for (i = first_task_module; i < last_task_module; i++, i2++)
    {
      t.id.task = myself.id.task + 1 + (i - first_task_module);
      check(task_alloc(t.id.task, myself.id.task));

      mod_exec_init(mb_mod, i, mb_info.mods_count);

      e.mod_start = (void *)(mb_mod[i].mod_start);
      e.task_no = t.id.task;
      e.mod_no = i;

      name = mb_mod_name(i);

      /* register name and id for the RMGR_TASK_GET_ID function */
      mb_mod_ids[i2] = t;

      printf("RMGR: loading task %s from 0x%08x-0x%08x\n      to [ ", 
	     name, mb_mod[i].mod_start, mb_mod[i].mod_end);
      exec_ret = exec_load(alloc_exec_read, alloc_exec_read_exec, 
			   &e, &exec_info);
      if (exec_ret)
	/* if it failed, try loading it as an PE file */
	exec_ret = exec_load_pe(alloc_exec_read, alloc_exec_read_exec, &e, &exec_info);

      printf("]\n");
      
      if (exec_ret)
	{
	  printf("RMGR: ERROR: exec_load() failed with code %d\n", exec_ret);
	  boot_error();
	  continue;
	}

      /* pass multiboot info to new task in extra page */
      for (address = 0x3000;	/* skip page 0, 1 and 2 */
	   address < mem_high; 
	   address += L4_PAGESIZE)
	{
	  if (! quota_check_mem(t.id.task, address, L4_PAGESIZE))
            continue;
	  if (memmap_owner_page(address) != O_FREE)
	    continue;
	  if (memmap_alloc_page(address, t.id.task))
	    break;
	}
      assert(address < mem_high);
	  
      /* copy mb_info */
      check((mbi = (struct grub_multiboot_info *)
	     alloc_from_page(address, sizeof(struct grub_multiboot_info))));

      *mbi = mb_info;
      mbi->flags &= MULTIBOOT_MEMORY|MULTIBOOT_BOOT_DEVICE|MB_INFO_VIDEO_INFO;
      
      /* copy mb_info->vbe_mode and mb_info->vbe_controller */
      if (mb_info.flags & MB_INFO_VIDEO_INFO)
	{
	  vm_offset_t old_address = address;
	  if (mb_info.vbe_mode_info)
	    {
	      if ((mbi_vbe_mode = (struct vbe_mode*)
		   alloc_from_page(address, sizeof(struct vbe_mode))) &&
		  (mbi_vbe_cont = (struct vbe_controller*)
		   alloc_from_page(address, sizeof(struct vbe_controller))))
		{
		  *mbi_vbe_mode = mb_vbe_mode;
		  *mbi_vbe_cont = mb_vbe_cont;
		  mbi->vbe_mode_info = (vm_offset_t)mbi_vbe_mode;
		  mbi->vbe_control_info = (vm_offset_t)mbi_vbe_cont;
		}
	      else
		{
		  printf("RMGR: ERROR: can't pass VBE video info "
			 "(needs %d bytes)\n",
			 sizeof(struct vbe_controller)+
			 sizeof(struct vbe_mode));
		  boot_error();
		  /* remove video information from multiboot info */
		  address = old_address;
		  mbi->flags &= ~MB_INFO_VIDEO_INFO;
		}
	    }
	}
      
      /* copy mb_info->cmdline */
      if (mb_mod[i].string)
	{
	  char *string;
	  vm_size_t len = strlen(name) + 1;

	  if (! (string = (char *) alloc_from_page(address, len)))
	    {
	      printf("RMGR: ERROR: command line too long -- truncating\n");
	      boot_error();

	      len = L4_PAGESIZE - 0x120;
	      check(string = (char *) alloc_from_page(address, len));
	    }
	  
	  mbi->flags |= MULTIBOOT_CMDLINE;
	  mbi->cmdline = (vm_offset_t) string;
	  strncpy(string, name, len);
	  *(string + len - 1) = 0; /* zero-terminate string */
	}

      /* before we start allocating extra mem, first allocate stack space */
      check((sp = (dword_t *) alloc_from_page(address, 0x100)));

      sp = (dword_t *) (0x100 + (vm_offset_t) sp);

      /* if any modules have been passed on to this task, copy module info */
      if (bootquota[t.id.task].mods)
	{
	  struct multiboot_module *m;
	  unsigned mod_i = i + 1;
	  unsigned j = bootquota[t.id.task].mods;

	  i += j;
	  if (i >= mb_info.mods_count)
	    {
	      printf("RMGR: ERROR: task configured to use %d modules, "
		     "but there are only %ld modules\n"
		     "             left for loading -- not starting %s\n",
		     j, mb_info.mods_count - (i - j) - 1, name);
	      boot_error();
	      break;
	    }

	  if (! (m = (struct multiboot_module *) 
		 alloc_from_page(address, 
				 j * sizeof(struct multiboot_module))))
	    {
	      printf("RMGR: ERROR: out of boot-info memory space --\n"
		     "             won't start %s",
		     name);
	      boot_error();
	      continue;
	    }
	  
	  mbi->mods_addr = (vm_offset_t) m;
	  mbi->mods_count = j;
	    
	  for (; j--; m++, mod_i++)
	    {
	      vm_offset_t mod_mem;
	      *m = mb_mod[mod_i];

	      if (m->string)
		{
		  vm_size_t len = strlen((char *) (m->string)) + 1;
		  char *string = (char *) alloc_from_page(address, len);

		  if (! string)
		    {
		      printf("RMGR: ERROR: not enough room for module string"
			     " --\n"
			     "      not passing string \"%s\"\n", 
			     (char *) (m->string));
		      boot_error();

		      m->string = 0;
		      continue;
		    }

		  strcpy(string, (char *) (m->string));
		  m->string = (vm_offset_t) string;
		}

	      printf("RMGR: passing module %s [ 0x%x-0x%x ]\n", 
		     mb_mod_name(mod_i), m->mod_start, m->mod_end);

	      for (mod_mem = m->mod_start & L4_PAGEMASK;
		   mod_mem < m->mod_end;
		   mod_mem += L4_PAGESIZE)
		{
		  if (! quota_check_mem(t.id.task, address, L4_PAGESIZE))
		    {
		      printf("RMGR: ERROR: cannot allocate page at 0x%08x "
			     "for this task (not in quota)\n"
			     "      skipping rest of module\n",
			     mod_mem);
		      m->mod_end = mod_mem;
		      boot_error();
		      break;
		    }

		  if ( ! memmap_alloc_page(mod_mem, t.id.task))
		    {
		      printf("RMGR: ERROR: cannot allocate page at 0x%08x "
			     "for this task (owned by 0x%x)\n"
			     "      skipping rest of module\n",
			     mod_mem, memmap_owner_page(mod_mem));
		      m->mod_end = mod_mem;
		      boot_error();
		      break;
		    }
		}

	      i2++;
	      mb_mod_ids[i2].lh.low = 0;
	      mb_mod_ids[i2].lh.high = 0;
	    }

	  mbi->flags |= MULTIBOOT_MODS;
	}

      /* copy task_trampoline PIC code to new task's stack */
      sp -= 1 + ((vm_offset_t) task_trampoline_end
		 - (vm_offset_t) task_trampoline) / sizeof(*sp);
      memcpy(sp, task_trampoline, 
	     (vm_offset_t) task_trampoline_end 
	     - (vm_offset_t) task_trampoline);
      entry = (dword_t) sp;

      /* setup stack for task_trampoline() */
      *--sp = (dword_t) mbi;
      *--sp = exec_info.entry;
      *--sp = 0;		/* fake return address */

      printf("RMGR: starting task %s (#0x%x)\n"
	     "      at entry 0x%x via trampoline page code 0x%x\n", 
	     name,
	     t.id.task,
	     exec_info.entry, entry);
      
      t = l4_task_new(t, bootquota[t.id.task].mcp, 
		      (dword_t) sp, entry, myself);
      
      setup_task(t);
    }
}

/* support for loading tasks from boot modules and allocating memory
   for them */

static int alloc_exec_read_exec(void *handle,
				vm_offset_t file_ofs, vm_size_t file_size,
				vm_offset_t mem_addr, vm_size_t mem_size,
				exec_sectype_t section_type)
{
  vm_offset_t address;
  struct exec_task *e = (struct exec_task *) handle;

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;
  
  assert(e->task_no <= O_MAX);

  printf("0x%08x-0x%08x ", mem_addr, mem_addr + mem_size);
  
  for (address = mem_addr & L4_PAGEMASK;
       address <= mem_addr + mem_size;
       address += L4_PAGESIZE)
    {
      if (! quota_check_mem(e->task_no, address, L4_PAGESIZE))
	{
	  boot_panic("\r\nRMGR: cannot allocate page at 0x%08x: not in quota",
		     address);
	}

      if (! memmap_alloc_page(address, e->task_no))
	{
	  boot_panic("\r\nRMGR: cannot allocate page at 0x%08x: owned by 0x%x",
		     address, memmap_owner_page(address));
	}
    }

  return mod_exec_read_exec(handle, file_ofs, file_size, 
			    mem_addr, mem_size, section_type);
}

static int alloc_exec_read(void *handle, vm_offset_t file_ofs,
			   void *buf, vm_size_t size,
			   vm_size_t *out_actual)
{
  return mod_exec_read(handle, file_ofs, buf, size, out_actual);
}

/* module loading support -- also used by startup.c to load the L4
   kernel -- therefore exported from here */

vm_offset_t mod_range_start, mod_range_end;
struct multiboot_module *mod_range_start_mod, *mod_range_end_mod;

void mod_exec_init(struct multiboot_module *mod, 
		   unsigned mod_first, unsigned mod_count)
{
  unsigned i;

  assert(mod_first < mod_count);

  /* compute range for modules which we must not overwrite during
     booting the startup tasks */
  mod_range_start = mod[mod_first].mod_start;
  mod_range_end = mod[mod_first].mod_end;
  for (i = mod_first + 1; i < mod_count; i++)
    {
      if (mod[i].mod_start < mod_range_start)
	mod_range_start = mod[i].mod_start;
      if (mod[i].mod_end > mod_range_end)
	mod_range_end = mod[i].mod_end;
    }

  mod_range_start_mod = mod + mod_first;
  mod_range_end_mod = mod + mod_count;
}

static void
check_inside_myself(void *handle, vm_offset_t begin, vm_offset_t end)
{
  struct exec_task *e = (struct exec_task *) handle;
 
  if (!(begin >= (vm_offset_t)&_etext && begin <= (vm_offset_t)&_end &&
	end   >= (vm_offset_t)&_etext && end   <= (vm_offset_t)&_end))
    {
      char *name;
      char namebuf[40];

      namebuf[sizeof(namebuf)-1] = 0;
      name = strncpy(namebuf, mb_mod_name(e->mod_no), sizeof(namebuf)-1);
      
      boot_panic("\r\n"
		 "RMGR: not inside: %08x - %08x: %s\n"
                 "            RMGR: %08x - %08x\n",
                 begin, end, name,
		 (vm_offset_t) &_start, (vm_offset_t) &_end);
    }
}

static inline
int overlaps_range(vm_offset_t tst_begin, vm_offset_t tst_end, 
                   vm_offset_t mem_begin, vm_offset_t mem_end)
{
  return !((tst_begin <= mem_begin && tst_end <= mem_begin)
	   || (tst_begin >= mem_end   && tst_end >= mem_end));
}

static int
overlaps_myself(vm_offset_t begin, vm_offset_t end)
{
  struct multiboot_module *mod;
  
  if (overlaps_range(begin, end, (vm_offset_t)&_start, (vm_offset_t)&_end))
    return 1;
  
  for (mod = mod_range_start_mod; mod != mod_range_end_mod; mod++)
    if (overlaps_range(begin, end, mod->mod_start, mod->mod_end))
      return 1;

  return 0;
}

static void
check_overlaps_myself(void *handle, vm_offset_t begin, vm_offset_t end)
{
  struct exec_task *e = (struct exec_task *) handle;
  int overlaps;
  
  if ((overlaps=overlaps_myself(begin, end)) 
      || (begin >= mem_high) || (end >= mem_high))
    {
      int i;
      char *name, *errstr;
      char namebuf[45];
      
      namebuf[sizeof(namebuf)-1] = 0;
      name = strncpy(namebuf, mb_mod_name(e->mod_no), sizeof(namebuf)-1);
      
      errstr = (overlaps) ? "overlaps" : "> memory";
      printf("\nRMGR: %s: %08x-%08x: %s\n"
             "          RMGR: %08x-%08x\n",
	     errstr, begin, end, name,
	     (vm_offset_t) &_start, (vm_offset_t) &_end);
      
      for (i=first_task_module;  i<mb_info.mods_count; i++)
	{
	  name = strncpy(namebuf, mb_mod_name(i), sizeof(namebuf)-1);
	  printf("     module %02d: %08x-%08x: %s\n", 
		 i, mb_mod[i].mod_start, mb_mod[i].mod_end, name);
	}
      panic("RMGR: overlapping modules");
    }
}

int mod_exec_read(void *handle, vm_offset_t file_ofs,
		  void *buf, vm_size_t size,
		  vm_size_t *out_actual)
{
  /* because this function is used for reading "metadata" of the executable,
   * we have to check that the access lays inside of our data/text segement */
  check_inside_myself(handle, (vm_offset_t) buf, (vm_offset_t) buf + size);

  return simple_exec_read(handle, file_ofs, buf, size, out_actual);
}

int mod_exec_read_exec(void *handle,
		       vm_offset_t file_ofs, vm_size_t file_size,
		       vm_offset_t mem_addr, vm_size_t mem_size,
		       exec_sectype_t section_type)
{
  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;

  /* this function is used to read executable code and data to be copied
   * to the loaded program image, so we have to check that we don't overwrite
   *   a) ourself
   *   b) the loaded modules by GRUB */
  check_overlaps_myself(handle, mem_addr, mem_addr + mem_size);

  return simple_exec_read_exec(handle, file_ofs, file_size, mem_addr, mem_size,
			       section_type);
}

static int set_small(l4_threadid_t t, int num)
{
  if (num < SMALL_MAX && small_alloc(num, t.id.task))
    {
      l4_sched_param_t sched;
      l4_threadid_t s;

      s = L4_INVALID_ID;
      l4_thread_schedule(t, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
      sched.sp.small = small_space_size 
	| (num * small_space_size * 2);
      s = L4_INVALID_ID;
      l4_thread_schedule(t, sched, &s, &s, &sched);
    }
  else
    {
      printf("RMGR: ERROR: can't set small space %d for task 0x%x\n",
	     num, t.id.task);
      boot_error();

      return 0;
    }

  return 1;
}

static int set_prio(l4_threadid_t t, int num)
{
  l4_sched_param_t sched;
  l4_threadid_t s;
  
  s = L4_INVALID_ID;
  l4_thread_schedule(t, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
  sched.sp.prio = num;
  /* don't mess up anything else than the thread's priority */
  sched.sp.zero = 0;
  sched.sp.small = 0;
  s = L4_INVALID_ID;
  l4_thread_schedule(t, sched, &s, &s, &sched);
  
  return 1;
}

