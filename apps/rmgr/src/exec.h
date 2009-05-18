#ifndef EXEC_H
#define EXEC_H

#include <flux/machine/types.h>
#include <flux/machine/multiboot.h>
#include <flux/exec/exec.h>

#include "grub_mb_info.h"
#include "grub_vbe_info.h"

/* services in exec.c */
exec_read_func_t simple_exec_read;
exec_read_exec_func_t simple_exec_read_exec;

/* pe-loader pe.c */
int exec_load_pe(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
		 void *handle, exec_info_t *out_info);

/* trampoline.c */
void task_trampoline(vm_offset_t entry, struct grub_multiboot_info *mbi);

extern char _task_trampoline_end;
#define task_trampoline_end ((void *) &_task_trampoline_end)

#endif
