#include <flux/machine/multiboot.h>
#include <l4/compiler.h>

#include "exec.h"

/* this function is mapped into a new tasks address space to load the
   registers in a multiboot-compliant way before starting the task's
   real code */
void task_trampoline(vm_offset_t entry, struct grub_multiboot_info *mbi)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile("movl %%edx,%%ebx
                call *%%ecx
                .globl " SYMBOL_NAME_STR(_task_trampoline_end) "\n"
	        SYMBOL_NAME_STR(_task_trampoline_end) ":"
	       : "=c" (dummy1), "=d" (dummy2), "=a" (dummy3)
	       : "0" (entry), "1" (mbi), "2" (MULTIBOOT_VALID) /* in */
	       );

  /* NORETURN */
}
