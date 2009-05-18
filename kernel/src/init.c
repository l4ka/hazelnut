/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     init.c
 * Description:   Architecture independent initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.26 2001/11/22 15:10:32 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <init.h>

#if defined(NEED_FARCALLS)
#include INC_ARCH(farcalls.h)
#endif

void init(void) L4_SECT_INIT;
void init_startup_servers(void) L4_SECT_INIT;
void setup_excp_vector(void) L4_SECT_INIT;

ptr_t __zero_page = NULL;

ptr_t kernel_arg;

/*
 * Function init ()
 *
 *    Main initialization function for kernel.  When the function is
 *    invoked, the MMU is enabled with the whole memory space mapped
 *    idempotently.
 *
 */
void init(void)
{  
    /* first init kdebug to see what's going on */
    if (kernel_info_page.kdebug_init)
	kernel_info_page.kdebug_init();

    printf(kernel_version);

    /* basic architecture initialization */
    init_arch_1();

    /* Initialize kernel memory allocator */
    kmem_init();

    /* map and initialize exception vector */
    /* so it is waterproof for everything afterwards :-) */
    setup_excp_vector();

    /* Initialize mapping database */
    mdb_init();

    /* reset the internal clock */
    kernel_info_page.clock = 0;

    /* Initialize interrupts */
    interrupts_init();

#if defined(CONFIG_ENABLE_PROFILING)
    /* Should be initialized before entering kdebug below. */
    void kdebug_init_profiling(void) L4_SECT_KDEBUG;
    kdebug_init_profiling();
#endif

    /* We create the idle thread here so that we have something to
       fall back into when entering kdebug. */
    tcb_t *idle = create_idle_thread();

    /* Architcture initialization needed before kdebug entry. */
    init_arch_2();

#if defined CONFIG_DEBUG_KDB_ONSTART
    enter_kdebug("startup");
#endif

    /* now switch to idle to start the other guys */
    switch_to_initial_thread(idle);

    /* should never return */
    panic("switch_to_idle returned\n");
}

