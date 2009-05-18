/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     init.h
 * Description:   Declarations of initialization functions.
 *                
 * @LICENSE@
 *                
 * $Id: init.h,v 1.17 2002/05/13 13:04:29 stoess Exp $
 *                
 ********************************************************************/
#ifndef __INIT_H__
#define __INIT_H__


/* From kmemory.c */
void kmem_init(void) L4_SECT_INIT;

/* From mapping.c */
void mdb_init(void) L4_SECT_INIT;
void mdb_buflist_init(void) L4_SECT_INIT;

/* From interrupts.c */
void interrupts_init() L4_SECT_INIT;

/* architecture specific initialization */
void init_arch_1() L4_SECT_INIT;
void init_arch_2() L4_SECT_INIT;
void init_arch_3() L4_SECT_INIT;



void setup_excp_vector() L4_SECT_INIT;

tcb_t * create_idle_thread() L4_SECT_INIT;


/* starts sigma 0 */
void create_sigma0();

/* starts root task */
void create_root_task();

void create_nil_tcb(void);

/* creates the thread that communicates with sigma0 */
void create_kmemthread();




/* the banner that gets printed through KDB on startup */
extern const char * kernel_version;


#endif /* __INIT_H__ */
