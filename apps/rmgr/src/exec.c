#include <string.h>
#include <flux/exec/exec.h>

#include "globals.h"
#include "rmgr.h"

/* the interface we implement */

#include "exec.h"

/* header read functions */

int simple_exec_read(void *handle, vm_offset_t file_ofs,
		     void *buf, vm_size_t size,
		     vm_size_t *out_actual)
{
  struct exec_task *e = (struct exec_task *) handle;
  memcpy(buf, (char*)(e->mod_start) + file_ofs, size);

  *out_actual = size;
  return 0;
}

/* read_exec functions */

int simple_exec_read_exec(void *handle,
			  vm_offset_t file_ofs, vm_size_t file_size,
			  vm_offset_t mem_addr, vm_size_t mem_size,
			  exec_sectype_t section_type)
{
  struct exec_task *e = (struct exec_task *) handle;
  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  memcpy((void *) mem_addr, (char*)(e->mod_start) + file_ofs, file_size);
  if (file_size < mem_size)
    memset((void *) (mem_addr + file_size), 0, mem_size - file_size);

  return 0;
}
