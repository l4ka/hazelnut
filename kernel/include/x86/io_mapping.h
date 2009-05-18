/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     x86/io_mapping.h
 * Description:   Declarations for mapping database for IO fpages
 *                
 * @LICENSE@
 *                
 * $Id: io_mapping.h,v 1.1 2002/05/13 14:04:52 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __X86_IO_MAPPING_H__
#define __X86_IO_MAPPING_H__

#include <universe.h>

#if defined(CONFIG_IO_FLEXPAGES)

#define IOFP_LO_BOUND    0xF0000000
#define IOFP_HI_BOUND    0xFFFFF043

typedef struct {
    unsigned grant:1;
    unsigned none:1;
    unsigned size:6;
    unsigned zero:4;
    unsigned port:16;
    unsigned f:4;
} io_fpage_struct_t;

typedef union {
    dword_t raw;
    io_fpage_struct_t fpage;
} io_fpage_t;


inline io_fpage_t io_fpage(word_t port, word_t ld_size, byte_t grant)
{
  return ((io_fpage_t){fpage:{grant, 0 , ld_size, 0, port, 0xf}});
}

int map_io_fpage(tcb_t *from, tcb_t *to, fpage_t io_fp);

void unmap_io_fpage(tcb_t *from, fpage_t io_fp, dword_t mapmask);

void io_mdb_task_new(tcb_t *current, tcb_t *task);

void io_mdb_delete_iospace(tcb_t *tcb);

void io_mdb_init_sigma0(void);	

#endif /*  defined(CONFIG_IO_FLEXPAGES) */

#endif /* __X86_IO_MAPPING_H__ */
