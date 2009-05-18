/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

/* OSKit stuff */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* L4 stuff */
#include <l4/l4.h>

/* local stuff */
#include "exec.h"
#include "globals.h"
#include "rmgr.h"
#include "init.h"

void startup(struct multiboot_info *mbi, unsigned int flag);

static exec_read_exec_func_t l4_exec_read_exec;
static vm_offset_t l4_low, l4_high;

static int
check_l4_version(char *start, char *end)
{
  char *p;

  no_pentium = 0;

  printf("check_l4_version %p to %p\n", start, end);

  for (p = start; p < end; p++)
    {
      if (memcmp(p, "L4/486 \346-Kernel", 15) == 0)
	{
	  int m4_ok;

	  printf("RMGR: detected L4/486\n");
	  no_pentium = 1;

	  /* remove old 4M event */
	  m4_ok = 0;
	  for (p=start; p<end; p++)
	    {
	      if (memcmp(p, "\314\353\0024M", 5) == 0)
		{
		  p[0] = 0x90; /* replace "int $3" by "nop" */
		  m4_ok=1;
		  break;
		}
	    }
	  if (!m4_ok)
	    printf("RMGR: 4M sequence not found in L4/486 -- that's OK.\n");
	  return VERSION_L4_V2;
	}

      if (memcmp(p, "L4/Pentium \346-Kernel", 19) == 0)
	{
	  /* it's Jochen's Pentium version */
	  printf("RMGR: detected L4/Pentium\n");	  
	  return VERSION_L4_V2;
	}

      if (memcmp(p, "Lava Nucleus (Pentium)", 22) == 0 || 
	  memcmp(p, "L4-X Nucleus (x86)", 18) == 0)
      {
	  /* it's the IBM version */
	  printf("RMGR: detected IBM LN/Pentium\n");
	  return VERSION_L4_IBM;
      }
	  
      if (memcmp(p, "DD-L4/x86 microkernel", 21) == 0)
      {
	  /* it's the Dresden version */
	  printf("RMGR: detected new-style DD-L4\n");
	  l4_version = VERSION_FIASCO;
	  return VERSION_FIASCO;
      }

      if (memcmp(p, "L4/KA", 5) == 0 ||
	  memcmp(p, "L4Ka/Hazelnut", 13) == 0)
				
      {
	  /* it's the Karlsruhe version */
	  printf("RMGR: detected L4-Karlsruhe\n");
	  return VERSION_L4_KA;
      }
      if (memcmp(p, "L4-Y", 4) == 0)
      {
	  /* it's the version Y - still X0 compatible */
	  printf("RMGR: detected L4-Y\n");
	  return VERSION_L4_IBM;
      }

      if (memcmp(p, "L4Ka/Pistachio", 14) == 0 ||
          memcmp(p, "L4Ka::Pistachio", 15) == 0)
      {
	  /* it's the Karlsruhe version */
	  printf("RMGR: detected Pistachio (L4-Karlsruhe)\n");
	  return VERSION_PISTACHIO;
      }

      if (memcmp(p, "L4Ka/Strawberry", 14) == 0)
      {
	  /* it's the Karlsruhe version */
	  printf("RMGR: detected Strawberry (L4-Karlsruhe)\n");
	  return VERSION_PISTACHIO;
      }
    }

  printf("RMGR: could not detect version of L4 -- assuming Jochen.\n");
  return VERSION_L4_V2;
}


void* _scan_kernel(const void* sequence, const int length, const int line);
void* _scan_kernel(const void* sequence, const int length, const int line)
{
    /* checker */
    unsigned char* find_p;
    void* found_p = 0;
    int found_count = 0;
    for(find_p = (unsigned char *) l4_low;
	find_p < (unsigned char *) l4_high;
	find_p++)
	/* check for some code */
	if (memcmp(find_p, sequence, length) == 0)
	{ found_p = (void*) find_p; found_count++; };
    switch (found_count)
    {
    case 0:
	printf("RMGR: sequence in line %d not found\n", line);
	break;
    case 1:
	break;
    default:
	printf("RMGR: sequence in line %d found more than once\n", line);
	found_p = 0;
	break;
    };
    return found_p;
};
#define scan_kernel(sequence) \
_scan_kernel(##sequence, sizeof(sequence)-1, __LINE__)

extern vm_offset_t mem_high;
extern vm_offset_t mem_phys;

static void init_jochen_dd_version(l4_kernel_info_t* l4i, exec_info_t* exec_info) 
{
    unsigned i;
    
    /* assertion: the entry point is at the beginning of the kernel info
       page */
    l4i = (l4_kernel_info_t *) exec_info->entry;
    assert(0x00001000 <= l4i->sigma0_eip && l4i->sigma0_eip < 0x000a0000);
    /* XXX can't think of more sanity checks that l4i really points to
       the L4 kernel info page :-( */

    /* XXX undocumented */
    l4i->reserved1.low = l4i->reserved1.high = 0;
    for (i = 2; i < 4; i++)
	l4i->dedicated[i].low = l4i->dedicated[i].high = 0;
    
    if (mb_mod[0].string)	/* do we have a command line for L4? */
    {
	/* more undocumented stuff: process kernel options */
	if (strstr((char *) mb_mod[0].string, "hercules"))
	{
	    unsigned char *kd_init = 
		(char *)((vm_offset_t)l4i - 0x1000 + l4i->init_default_kdebug);
		    
	    assert(kd_init[0] == 0xb0 && kd_init[1] == 'a');
	    kd_init[1] = 'h';
	}
	    
	if (strstr((char *) mb_mod[0].string, "nowait"))
	{
	    unsigned char *kd_debug;
	    int nowait_ok = 0;

	    for(kd_debug = (unsigned char *) l4_low;
		kd_debug < (unsigned char *) l4_low + (l4_high - l4_low);
		kd_debug++)
	    {
		/* check for machine code 
		   "int $3; jmp 0f; .ascii "debug"; 0:" */
		if (memcmp(kd_debug, "\314\353\005debug", 8) == 0)
		{
		    kd_debug[0] = 0x90; /* replace "int $3" by "nop" */
		    nowait_ok = 1;
		    break;
		}
	    }
	    assert(nowait_ok);
	}

	/* all those serial port related options */
	{
	    unsigned short portaddr = 0x02F8;	/* This is COM2, the default */
	    unsigned short divisor  = 1;	/* 115200 bps */
	    unsigned char* ratetable = 0;

	    if (strstr((char *) mb_mod[0].string, " -comport"))
	    {
		unsigned port;
		unsigned char* p_adr;
		
		const unsigned char port_adr_hi[] = {0x00, 0x3F, 0x2F, 0x3E, 0x2E};
		
		port = strtoul((strstr((char *) mb_mod[0].string, " -comport")
				+ strlen(" -comport")), NULL, 10);
		
		if (port)
		{
		    p_adr = scan_kernel("\270\003\207\057\000");
		    if (p_adr)
		    {
			printf("RMGR: Setting COM%d as L4KD serial port\n", port);
			p_adr[3] = port_adr_hi[port];
			portaddr = (port_adr_hi[port] << 4) | 0x8;
		    }
		}
		else
		    printf("RMGR: INVALID serial port\n");
	    }
	    
	    if (strstr((char *) mb_mod[0].string, " -comspeed"))
	    {
		unsigned rate;
	      
		rate = strtoul((strstr((char *) mb_mod[0].string, " -comspeed")
				+ strlen(" -comspeed")), NULL, 10);
	      
		if (rate)
		{
		    /* check for rate table
		       dw 192, dw 96, dw 48, dw 24, ...
		       the 7th entry is used by default */
		    ratetable = scan_kernel("\300\000\140\000\060\000\030\000");
		    if (ratetable)
		    {
			printf("RMGR: Setting L4KD serial port rate to %d bps\n", rate);
			((short*) ratetable)[7] = 115200/rate;
			divisor = 115200/rate;
		    }
		}
		else
		    printf("RMGR: INVALID serial rate\n");
	    }
	    
	    if (strstr((char *) mb_mod[0].string, " -VT"))
	    {
		
		if (!ratetable)
		    ratetable = scan_kernel("\300\000\140\000\060\000\030\000");
		
		if (ratetable)
		{
		    printf("RMGR: Enabling serial terminal at %3x, %d bps\n", portaddr, 115200/divisor);
		    /* Uaah, the word before the rate table holds the port address for remote_outchar */
		    ((short*) ratetable)[-1] = portaddr;
		    
		    /* initialize the serial port */
		    asm("mov $0x83,%%al;add $3,%%dx;outb %%al,%%dx;"
			"sub $3,%%dx;mov %%bx,%%ax;outw %%ax,%%dx;add $3,%%dx;"
			"mov $0x03,%%al;outb %%al,%%dx;dec %%dx;inb %%dx,%%al;"
			"add $3,%%dx;inb %%dx,%%al;sub $3,%%dx;inb %%dx,%%al;"
			"sub $2,%%dx;inb %%dx,%%al;add $2,%%dx;inb %%dx,%%al;"
			"add $4,%%dx;inb %%dx,%%al;sub $4,%%dx;inb %%dx,%%al;"
			"dec %%dx;xor %%al,%%al;outb %%al,%%dx;mov $11,%%al;"
			"add $3,%%dx;outb %%al,%%dx\n"
			:
			/* no output */
			:
			"d" (portaddr),
			"b" (divisor)
			:
			"eax"
			);

		    if (strstr((char *) mb_mod[0].string, " -VT+"))
		    {
			unsigned char* p;
			if ((p = scan_kernel("\203\370\377\165\034\350")))
			{
			    p[-6] = 1;
			    printf("RMGR: -VT+ mode enabled\n");
			};
		    }
		}
		
	    }
	    
	    if (strstr((char *) mb_mod[0].string, " -I+"))
	    {
		
		/* idea:
		   in init_timer the idt_gate is set up for the timer irq handler.
		   There we install the kdebug_timer_intr_handler.
		   in kdebug_timer... timer_int is called (saved location of timer_int) */
		
		unsigned long timer_int, kdebug_timer_intr_handler;
		void* real_location;
		void* tmp_location;
		unsigned long relocation;
		
		unsigned short* new;
		void* p;
		void* dest;
		
		/* find remote_info_port */
		tmp_location = scan_kernel("\300\000\140\000\060\000\030\000");
		if (tmp_location)
		    tmp_location -= 2;
		
		/* find reference to remote_info_port */
		real_location = scan_kernel("\146\201\342\377\017");
		if (real_location)
		    real_location = (void*) ((unsigned long) (((unsigned short*) real_location)[4]));
		
		relocation = tmp_location - real_location;
		
		/* find the place where the timer_int is set up */
		p = scan_kernel("\000\000"	/* this is a bit illegal !!!*/
				"\303"		/* c3		*/
				"\263\050"	/* b3 28	*/
				"\267\000"	/* b7 00	*/
				"\270"		/* b8		*/
		    );
		if (p)
		{
		    timer_int = ((unsigned long*)p)[2];
		    
		    /* find kdebug_timer_intr_handler */
		    dest = scan_kernel("\152\200\140\036\152\010"
				       "\037\314\074\014\074\033");
		    /* no more comments */
		    if (dest)
		    {
			kdebug_timer_intr_handler = (((unsigned long) dest) - relocation);
			((unsigned long*)p)[2] = kdebug_timer_intr_handler;
			dest = scan_kernel("\015\260\055\314"
			    );
			if (dest)
			{
			    new = (unsigned short*) ((((unsigned short*)dest)[-5]) + relocation);
			    *new = timer_int;
			    printf("RMGR: I+ enabled\n");
			};
		    };
		};
	    };
	}

	/*
	  This option disables the patch that eliminates the miscalculation
	  of string offsets in messages containing indirect strings

	  The patch changes calculation of the first string's offset in the
	  message buffer (in ipc_strings).
	  This offset was calculated

	  addr(msgbuffer) + offset(dword_2) + 4*number_of_dwords

	  this is changed to
	  addr(msgbuffer) + offset(dword_0) + 4*number_of_dwords
	  ^^^^^^^
	*/
	if (strstr((char *) mb_mod[0].string, " -disablestringipcpatch"))
	    printf("RMGR: string IPC patch disabled\n");
	else
	{
	    unsigned char* patchme;
	    patchme = scan_kernel("\024\213\126\004\301\352\015\215\164\226\024");
	    if (patchme)
	    {
		patchme[0] = 0x0c;
		patchme[10] = 0x0c;
		printf("RMGR: string IPC patch applied\n");
	    }
	    else
		printf("RMGR: string IPC patch not applied - signature not found\n");
	};
	  
	/* heavy undocumented stuff, we simply patch the kernel to get
	   access to irq0 and to be able to use the high resolution timer */
	if (strstr((char *) mb_mod[0].string, "irq0"))
	{
	    unsigned char *kd_debug;
	    int irq0_ok = 0;

	    for(kd_debug = (unsigned char *) l4_low;
		kd_debug < (unsigned char *) l4_low + (l4_high - l4_low);
		kd_debug++)
	    {
		/* check for byte sequence 
		   0xb8 0x01 0x01 0x00 0x00 0xe8 ?? ?? ?? ?? 0x61 0xc3 */
		if (memcmp(kd_debug, "\270\001\001\000\000\350", 6) == 0)
		{
		    if ((kd_debug[10] == 0141) && (kd_debug[11] == 0303)) 
		    {
			kd_debug[1] = '\0'; /* enable irq0 */
			irq0_ok = 1;
			break;
		    }
		}
	    }
	    assert(irq0_ok);
	}

	/* boot L4 in real-mode (for old versions) */
	if (strstr((char *) mb_mod[0].string, "boothack"))
	{
	    /* find "cmpl $0x2badb002,%eax" */
	    vm_offset_t dest 
		= (vm_offset_t) scan_kernel("\075\002\260\255\053");
	      
	    if (dest)
		exec_info->entry = dest;
	}
    }
}

static void 
init_nucleus(l4_kernel_info_t* l4i) {
    unsigned i;

    // for consistency with Dresden we always enter the kernel debugger...
    l4i->kdebug_config = 0x100; 

    /* assertion: the entry point is at the beginning of the kernel info
       page */

    assert(0x00001000 <= l4i->sigma0_eip && l4i->sigma0_eip < 0x000a0000);
    /* XXX can't think of more sanity checks that l4i really points to
       the L4 kernel info page :-( */

    l4i->reserved1.low = l4i->reserved1.high = 0;
    for (i = 2; i < 4; i++)
	l4i->dedicated[i].low = l4i->dedicated[i].high = 0;
    
    if (mb_mod[0].string)	/* do we have a command line for L4? */
    {
	/* more undocumented stuff: process kernel options */
	if (strstr((char *) mb_mod[0].string, "hercules"))
	{
	    printf("IBM's Nucleus does not support hercules as setting\n");
	}
	    
	if (strstr((char *) mb_mod[0].string, "nowait"))
	{
	    l4i->kdebug_config &= ~0x100;
	}

	/* all those serial port related options */
	{
	    if (strstr((char *) mb_mod[0].string, " -comport"))
	    {
		unsigned port;
		const unsigned char port_adr_hi[] = {0x00, 0x3F, 0x2F, 0x3E, 0x2E};
		
		port = strtoul((strstr((char *) mb_mod[0].string, " -comport")
				+ strlen(" -comport")), NULL, 10);
		
		if (port)
		{
		    printf("RMGR: Setting COM%d as L4KD serial port\n", port);
		    l4i->kdebug_config  |= ((port_adr_hi[port] << 4 | 0x8) << 20) & 
			0xfff00000;
		}
		else
		    printf("RMGR: INVALID serial port\n");
	    }
	    
	    if (strstr((char *) mb_mod[0].string, " -comspeed"))
	    {
		unsigned rate;
	      
		rate = strtoul((strstr((char *) mb_mod[0].string, " -comspeed")
				+ strlen(" -comspeed")), NULL, 10);
	      
		if (rate)
		{
		    l4i->kdebug_config |= ((115200/rate) << 16) & 0xf0000;
		}
		else
		    printf("RMGR: INVALID serial rate\n");
	    }

	    if (strstr((char *) mb_mod[0].string, " -VT"))
	    {
		// set to serial
		if (strstr((char *) mb_mod[0].string, " -VT+"))
		{
		    printf("RMGR: VT-option does not exist - use comspeed/port instead\n");
		}
	    }
	    
	} // end of comport-related stuff


	if (strstr((char *) mb_mod[0].string, "-tracepages"))
	{
	    unsigned tracepages;
	    
	    tracepages = strtoul((strstr((char *) mb_mod[0].string, " -tracepages")
				  + strlen(" -tracepages")), NULL, 10);
	    if (tracepages) {
		l4i->kdebug_config |= (tracepages & 0xff);
		printf("RMGR: debugger configured with %d trace pages\n", 
		       tracepages);
	    }
	    else 
		printf("RMGR: invalid tracepage number\n");
	    
	}

	if (strstr((char *) mb_mod[0].string, "-lastdebugtask"))
	{
	    unsigned ldtask;
	    
	    ldtask = strtoul((strstr((char *) mb_mod[0].string, " -lastdebugtask")
			      + strlen(" -lastdebugtask")), NULL, 10);
	    if (ldtask) {
		l4i->kdebug_permission |= (ldtask & 0xff);
		printf("RMGR: debugger can be invoked by task 0 to %d",
		       ldtask);
	    }
	    else 
		printf("RMGR: invalid debug task number\n");
	    
	}

	if (strstr((char *) mb_mod[0].string, "-d-no-mapping"))
	{
	    l4i->kdebug_config &= ~0x100;
	}

	if (strstr((char *) mb_mod[0].string, "-d-no-registers"))
	{
	    l4i->kdebug_config &= ~0x200;
	}

	if (strstr((char *) mb_mod[0].string, "-d-no-memory"))
	{
	    l4i->kdebug_config &= ~0x400;
	}

	if (strstr((char *) mb_mod[0].string, "-d-no-modif"))
	{
	    l4i->kdebug_config &= ~0x800;
	}
	
	if (strstr((char *) mb_mod[0].string, "-d-no-io"))
	{
	    l4i->kdebug_config &= ~0x1000;
	}

	if (strstr((char *) mb_mod[0].string, "-d-no-protocol"))
	{
	    l4i->kdebug_config &= ~0x1000;
	}

	

	
    }
    else 
	printf("no configuration string for the nucleus\n");

    printf("RMGR: kernel-debug config: %x, %x\n", 
	   l4i->kdebug_config, l4i->kdebug_permission);
}

extern void pci_reloc(void);

/* started from crt0.S */
void
startup(struct multiboot_info *mbi, unsigned int flag)
{
    l4_kernel_info_t *l4i;
    static struct grub_multiboot_info l4_mbi;
    exec_info_t exec_info;
    int exec_ret;

    int i;

    vm_offset_t sigma0_low = 0, sigma0_high = 0, sigma0_start = 0, 
	sigma0_stack = 0; /* zero-initialized to avoid spurious warnings */
    int have_sigma0 = 0;
    struct exec_task e;

    vm_offset_t roottask_low = 0, roottask_high = 0, roottask_start = 0, 
	roottask_stack = 0; /* zero-initialized to avoid spurious warnings */
    int have_roottask = 0;

    /* we're not an L4 task yet -- we're just a GRUB-booted kernel */

    assert(flag == MULTIBOOT_VALID);	/* we need to be multiboot-booted */

    assert(mbi->flags & MULTIBOOT_MODS);	/* we need at least two boot modules: */
    assert(mbi->mods_count >= 2);	/* 1 = L4 kernel, 2 = first user task */

    assert(mbi->mods_count <= MODS_MAX);

    /* print RMGR messages on hercules screen? */
    if ((mbi->flags & MULTIBOOT_CMDLINE)
	&& strstr((char *) mbi->cmdline, " -hercules")
	&& have_hercules())
	hercules = 1;
    
    if ((mbi->flags & MULTIBOOT_CMDLINE)
	&& strstr((char *) mbi->cmdline, "-maxmem"))
    {
	mbi->mem_upper = 1024*(strtoul((strstr((char *) mbi->cmdline, " -maxmem") + strlen(" -maxmem")), NULL, 10) - 1); 
	printf("RMGR: limiting memory to %d MB (%d KB)\n", mbi->mem_upper/1024+1, mbi->mem_upper);
    }

    /* Guess the amount of installed memory from the bootloader. We already
     * need that value here because we have to check for a valid L4 kernel.
     * Later (in init.c), mem_high is determined by the sigma0 server. */
    mem_high = mem_phys = 64*1024*1024;
    if (mbi->flags & MULTIBOOT_MEMORY)
	mem_high = mem_phys = 1024 * (mbi->mem_upper + mbi->mem_lower);

    /* copy Multiboot data structures we still need to a safe place
       before playing with memory we don't own and starting L4 */

    /* rmgr.c defines variables holding copies */
    if (mbi->flags & MB_INFO_VIDEO_INFO)
      /* extended VIDEO information available */
      {
	struct grub_multiboot_info *gmbi = (struct grub_multiboot_info*)mbi;

	printf("RMGR: Using extended multiboot info\n");

	/* copy extended multiboot info structure */
	memcpy(&mb_info, mbi, sizeof(struct grub_multiboot_info));

	/* copy VBE mode info structure */
	if (gmbi->vbe_mode_info)
	  {
	    memcpy(&mb_vbe_mode, (void*)(gmbi->vbe_mode_info),
		   sizeof(struct vbe_mode));
	    mb_info.vbe_mode_info = (vm_offset_t) &mb_vbe_mode;
	  }
	
	/* copy VBE controller info structure */
	if (gmbi->vbe_control_info)
	  {
	    memcpy(&mb_vbe_cont, (void*)(gmbi->vbe_control_info),
		   sizeof(struct vbe_controller));
	    mb_info.vbe_control_info = (vm_offset_t) &mb_vbe_cont;
	  }

	mb_info.flags &= (MB_INFO_MEMORY | MB_INFO_BOOTDEV |
			  MB_INFO_CMDLINE | MB_INFO_MODS |
			  MB_INFO_AOUT_SYMS | MB_INFO_ELF_SHDR |
			  MB_INFO_MEM_MAP | MB_INFO_VIDEO_INFO);
      }

    else
      /* extended VIDEO information not available, perhaps we use the old GRUB
       * so only copy the reliable information */
      {
	/* copy simple (old) multiboot structure */
	memcpy(&mb_info, mbi, sizeof(struct multiboot_info));
	mb_info.flags &= (MULTIBOOT_MEMORY | MULTIBOOT_BOOT_DEVICE |
			  MULTIBOOT_CMDLINE | MULTIBOOT_MODS |
			  MULTIBOOT_AOUT_SYMS | MULTIBOOT_ELF_SHDR |
			  MULTIBOOT_MEM_MAP);
      }
    
    /* copy module descriptions */
    memcpy(mb_mod, (void *) mbi->mods_addr, 
	   mbi->mods_count * sizeof (struct multiboot_module));
    mb_info.mods_addr = (vm_offset_t) mb_mod;

    for (i = 0; i < mbi->mods_count; i++)
      {
	char *name = (char *) mb_mod[i].string;

	strncpy(mb_mod_names[i], name ? name : "[unknown]", MOD_NAME_MAX);
	mb_mod_names[i][MOD_NAME_MAX-1] = '\0'; /* terminate string */
	if (name)
	  mb_mod[i].string = (vm_offset_t) mb_mod_names[i];
      }

    if (mb_info.flags & MULTIBOOT_CMDLINE)
      {
	strncpy(mb_cmdline, (char *) (mb_info.cmdline), sizeof(mb_cmdline));
	mb_cmdline[sizeof(mb_cmdline) - 1] = 0;
	mb_info.cmdline = (vm_offset_t) mb_cmdline;
      }                              
    /* shouldn't touch original Multiboot parameters after here */

    /* compute range for modules which we must not overwrite during
       booting the startup tasks */
    mod_exec_init(mb_mod, 0, mb_info.mods_count);

    /* lets look if we have a user-specified sigma0 */
    if ((mb_info.flags & MULTIBOOT_CMDLINE)
	&& strstr((char *) mb_info.cmdline, " -sigma0"))
    {
	/* we define a small stack for sigma0 here -- it is used by L4
	   for parameter passing to sigma0.  however, sigma0 must switch
	   to its own private stack as soon as it has initialized itself
	   because this memory region is later recycled in init.c */
	static char sigma0_init_stack[64]; /* XXX hardcoded */

	l4_low = 0xffffffff;
	l4_high = 0;

	assert(mb_info.mods_count >= (first_task_module + 2)); /* need one more module -- sigma0 */

	printf("RMGR: loading %s\n", 
	       mb_mod[1].string ? (char *) mb_mod[1].string : "[SIGMA0]");

	e.mod_start = (void *)(mb_mod[first_task_module].mod_start);
	e.task_no = first_task_module;
	e.mod_no = first_task_module;
	exec_load(mod_exec_read, l4_exec_read_exec, 
		  &e, &exec_info);

	sigma0_low = l4_low & L4_PAGEMASK;
	sigma0_high = (l4_high + (L4_PAGESIZE-1)) & L4_PAGEMASK;
	sigma0_start = exec_info.entry;
	sigma0_stack = 
	    (vm_offset_t)(sigma0_init_stack + sizeof(sigma0_init_stack));

	have_sigma0 = 1;
	first_task_module++;
    }

    /* lets look if we want to use rmgr just for loading and configuring the kernel */
    if ((mb_info.flags & MULTIBOOT_CMDLINE)
	&& strstr((char *) mb_info.cmdline, "-roottask"))
    {
	/* we define a small stack for roottask here -- it is used by L4
	   for parameter passing to roottask.  however, roottask must switch
	   to its own private stack as soon as it has initialized itself
	   because this memory region is later recycled in init.c */
	static char roottask_init_stack[64]; /* XXX hardcoded */

	l4_low = 0xffffffff;
	l4_high = 0;

//	assert(mb_info.mods_count >= (first_task_module + 2)); /* need one more module -- roottask */

	printf("RMGR: loading %s\n", 
	       mb_mod[first_task_module].string ? (char *) mb_mod[first_task_module].string : "[ROOTTASK]");

	e.mod_start = (void *)(mb_mod[first_task_module].mod_start);
	e.task_no = first_task_module;
	e.mod_no = first_task_module;
	exec_ret = exec_load(mod_exec_read, l4_exec_read_exec, 
			     &e, &exec_info);
	if (exec_ret)
	    exec_ret = exec_load_pe(mod_exec_read, l4_exec_read_exec, 
				    (void *)(mb_mod[first_task_module].mod_start), &exec_info);

	roottask_low = l4_low & L4_PAGEMASK;
	roottask_high = (l4_high + (L4_PAGESIZE-1)) & L4_PAGEMASK;
	roottask_start = exec_info.entry;
	roottask_stack = 
	    (vm_offset_t)(roottask_init_stack + sizeof(roottask_init_stack));

	have_roottask = 1;
	first_task_module++;
    }
    
    if ((mb_info.flags & MULTIBOOT_CMDLINE)
	&& strstr((char *) mb_info.cmdline, " -symbols"))
      {
	printf("RMGR: loading %s\n", 
	       mb_mod[first_task_module].string 
               ? (char *) mb_mod[first_task_module].string : "[SYMBOLS]");

	e.mod_start = (void *)mb_mod[first_task_module].mod_start;
	e.task_no = 0;
	e.mod_no = first_task_module;
	exec_load(mod_exec_read, l4_exec_read_exec, 
		  &e, &exec_info);
	
	fiasco_symbols     = mb_mod[first_task_module].mod_start;
	fiasco_symbols_end = (mb_mod[first_task_module].mod_end 
			      + L4_PAGESIZE - 1) & L4_PAGEMASK;
	
	first_task_module++;
      }

    if ((mb_info.flags & MULTIBOOT_CMDLINE)
	&& strstr((char *) mb_info.cmdline, " -mods="))
      {
	last_task_module = atoi (strstr((char *) mb_info.cmdline, " -mods=") + 7);
      }

    l4_low = 0xffffffff;
    l4_high = 0;

    /* the first module is the L4 microkernel; load it */
    e.mod_start = (void *)mb_mod[0].mod_start;
    e.task_no = 0;
    e.mod_no = 0;
    exec_load(mod_exec_read, l4_exec_read_exec, 
	      &e, &exec_info);
  
    /* XXX we don't look at the Multiboot header of the L4 kernel image */

    /* check for L4 version */
    l4_version = check_l4_version((unsigned char *) l4_low, 
				  (unsigned char *) l4_low + (l4_high - l4_low));
    
    if ((l4_version != VERSION_FIASCO) && fiasco_symbols)
    {
      printf("RMGR: L4 version is not Fiasco -- disabling Fiasco symbols\n");
      fiasco_symbols = 0;
    }

    switch (l4_version) {
    case VERSION_L4_V2:
	l4i = (l4_kernel_info_t *) exec_info.entry;
	init_jochen_dd_version(l4i, &exec_info); break;

    case VERSION_L4_IBM:
	l4i = (l4_kernel_info_t *) exec_info.entry;
	init_nucleus(l4i); break;

    case VERSION_PISTACHIO:
    case VERSION_L4_KA:
	{
	    unsigned char * p;
	    //no_pentium = 1;
	    l4i = 0;
	    printf("find kernel info page...\n");
	    for (p = (unsigned char *) (l4_low & 0xfffff000); 
		 p < (unsigned char *) l4_low + (l4_high - l4_low); 
		 p += 0x1000)
	    {
		dword_t magic = L4_KERNEL_INFO_MAGIC;
		if (memcmp(p, &magic, 4) == 0)
		{
		    l4i = (l4_kernel_info_t *) p;
		    printf("found kernel info page at %p\n", p);
		    break;
		}
	    }
	    assert(l4i);
	    l4i->reserved0.low = l4_low;
	    l4i->reserved0.high = l4_high;
	}
	break;

    case VERSION_FIASCO: 
    {
	static char cmdline[256];
	int l;

	/* use an info page prototype allocated from our address space;
	   this will be copied later into the kernel's address space */
	static char proto[L4_PAGESIZE];

	memset(proto, 0, L4_PAGESIZE);
	l4i = (l4_kernel_info_t *) proto;

	/* append address of kernel info prototype to kernel's command
	   line */
	if (mb_mod[0].string)
	{
	    l = strlen((char *) mb_mod[0].string);
	    /* make sure it fits into cmdline[] */
	    assert(l < sizeof(cmdline) - 40);
	    strcpy(cmdline, (char *) mb_mod[0].string);
	}
	else
	    l = 0;

	sprintf(cmdline + l, " proto=0x%x", (vm_offset_t) proto);
	mb_mod[0].string = (vm_offset_t) cmdline;
    } break;
    default:
	l4i = 0;
	printf("RMGR: could not identify version of the kernel\n");
	getchar();
    }

    /* setup the L4 kernel info page before booting the L4 microkernel:
       patch ourselves into the booter task addresses */
    l4i->root_esp = (dword_t) &_stack;
    l4i->root_eip = (dword_t) init;
#if 0
    l4i->root_memory.low = l4i->dedicated[0].low = (dword_t) &_start;
    l4i->root_memory.high = l4i->dedicated[0].high = (dword_t) &_end;
#else
    l4i->root_memory.low = ((dword_t) &_start) & L4_PAGEMASK;
    l4i->root_memory.high = (((dword_t) &_end) + (L4_PAGESIZE-1)) & L4_PAGEMASK;
#endif    

    /* set up sigma0 info */
    if (have_sigma0)
    {
	printf("configuring s0: (%p, %p), start: %p\n", sigma0_low, sigma0_high, sigma0_start);

	l4i->sigma0_eip = sigma0_start;
	l4i->sigma0_memory.low = sigma0_low;
	l4i->sigma0_memory.high = sigma0_high;

	printf("s0 config: %p,%p,%p,%p\n", l4i->sigma0_eip, l4i->sigma0_esp, 
	       l4i->sigma0_memory.low, l4i->sigma0_memory.high);

	/* XXX UGLY HACK: Jochen's kernel can't pass args on a sigma0
	   stack not within the L4 kernel itself -- therefore we use the
	   kernel info page itself as the stack for sigma0.  the field
	   we use is the task descriptor for the unused "ktest2" task */
	switch (l4_version) {
	case VERSION_L4_V2:
	case VERSION_L4_IBM:
	    l4i->sigma0_esp = 0x1060;
	    break;
	case VERSION_FIASCO:
	    l4i->sigma0_esp = sigma0_stack;
	    break;
	}
    }

    /* set up roottask info */
    if (have_roottask)
    {
	printf("configuring roottask: (%p, %p), start: %p\n", roottask_low, roottask_high, roottask_start);

	l4i->root_eip = roottask_start;
	l4i->root_memory.low = roottask_low;
	l4i->root_memory.high = roottask_high;
	printf("roottask config: %p,%p,%p,%p\n", l4i->root_eip, l4i->root_esp, 
		l4i->root_memory.low, l4i->root_memory.high);
    }

    /* reserve memory for the modules */
    mod_exec_init(mb_mod, 1, mb_info.mods_count);

    printf("RMGR: reserve modules memory range: %x to %x\n", 
	   mod_range_start, mod_range_end);

    l4i->dedicated[1].low = mod_range_start;
    l4i->dedicated[1].high = mod_range_end;

    l4i->main_memory.low = 0;
    l4i->main_memory.high = (mb_info.mem_upper + 1024) * 1024;
    l4i->dedicated[0].low = mb_info.mem_lower * 1024;
    l4i->dedicated[0].high = 1024*1024;

    /* now boot the L4 microkernel in a multiboot-compliant way */

    if (l4_version == VERSION_PISTACHIO)
    {
	/* Pistachio has a slightly different way of starting
	 * tasks. The boot loader has a dedicated field in the 
	 * kernel interface page to boot the root task. 
	 */
	((dword_t*)l4i)[46] = (dword_t)&mb_info;
	
	/* clear the memory-info field */
	((dword_t*)l4i)[21] &= 0xffff0000;
    }
  
    l4_mbi = mb_info;
    assert(l4_mbi.flags & MULTIBOOT_MEMORY);
    l4_mbi.flags = MULTIBOOT_MEMORY | MULTIBOOT_MODS;
    if (mb_mod[0].string)
    {
	l4_mbi.cmdline = mb_mod[0].string;
	l4_mbi.flags |= MULTIBOOT_CMDLINE;
    }
  
    printf("RMGR: starting %s @ %p\n", 
	   mb_mod[0].string ? (char *) mb_mod[0].string : "[L4]", exec_info.entry);

//printf("RMGR: pci_reloc\n");
//pci_reloc();
    kernel_running = 1;

    asm volatile
	("pushl $" SYMBOL_NAME_STR(exit) "
	 pushl %2
	 ret"
	 :				/* no output */
	 : "a" (MULTIBOOT_VALID), "b" (&l4_mbi), "r" (exec_info.entry));

    /*NORETURN*/

}


static int l4_exec_read_exec(void *handle,
			     vm_offset_t file_ofs, vm_size_t file_size,
			     vm_offset_t mem_addr, vm_size_t mem_size,
			     exec_sectype_t section_type)
{
    if (! (section_type & EXEC_SECTYPE_ALLOC))
	return 0;
  
    if (mem_addr < l4_low) l4_low = mem_addr;
    if (mem_addr + mem_size > l4_high) l4_high = mem_addr + mem_size;

    return mod_exec_read_exec(handle, file_ofs, file_size, 
			      mem_addr, mem_size, section_type);
}

