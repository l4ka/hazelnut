/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     kdebug.h
 * Description:   Declarations of functions located within kernel
 *                debugger.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug.h,v 1.20 2001/12/04 20:37:30 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __KDEBUG_H__
#define __KDEBUG_H__

/* usually implemented in arch/kdebug.c */
void putc(char c) L4_SECT_KDEBUG;
char getc() L4_SECT_KDEBUG;

/* kdebug.c */
int printf(const char* format, ...) L4_SECT_KDEBUG;
void panic(const char *msg) L4_SECT_KDEBUG;
int kdebug_entry(exception_frame_t* frame) L4_SECT_KDEBUG;

void kdebug_init() L4_SECT_KDEBUG;


extern int __kdebug_mdb_tracing;
extern int __kdebug_pf_tracing;

/* arch/kdebug.c */
void kdebug_dump_frame(exception_frame_t *frame) L4_SECT_KDEBUG;
void kdebug_dump_pgtable(ptr_t pgtable) L4_SECT_KDEBUG;
void kdebug_dump_map(dword_t pgtable) L4_SECT_KDEBUG;
void kdebug_mdb_stats(dword_t start, dword_t end) L4_SECT_KDEBUG;
void kdebug_dump_tcb(tcb_t* tcb) L4_SECT_KDEBUG;
void kdebug_pf_tracing(int state) L4_SECT_KDEBUG;
void kdebug_hwreset() L4_SECT_KDEBUG;
void kdebug_check_breakin() L4_SECT_KDEBUG;
void kdebug_dump_tracebuffer() L4_SECT_KDEBUG;
void kdebug_flush_tracebuffer() L4_SECT_KDEBUG;


#define TRACE() printf("%s: %d\n", __FUNCTION__, __LINE__)


#include INC_ARCH(kdebug.h)

#endif /* __KDEBUG_H__ */
