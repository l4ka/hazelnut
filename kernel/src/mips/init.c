/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/init.c
 * Description:   MIPS specific initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.4 2001/11/22 13:27:53 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mips/sgialib.h>

/* Master romvec interface. */
struct linux_romvec *romvec;
struct linux_promblock *sgi_pblock;
int prom_argc;
char **prom_argv, **prom_envp;
unsigned short prom_vers, prom_rev;


void init_arch_1()
{
    //printf(__FUNCTION__);
    init_paging();
}

void init_arch_2()
{
}

void init_arch_3()
{
    printf(__FUNCTION__);
    enter_kdebug();
}

#define DEF_EXCEPT(name) extern "C" void* name##();extern "C" void* name##__end();
#define EXCEPT_LEN(name) (((dword_t)(##name##__end)) - (dword_t)(name))

DEF_EXCEPT(exception_handler);
DEF_EXCEPT(exception_cache_error);
DEF_EXCEPT(tlb_refill);

void setup_excp_vector()
{
    zero_memory((dword_t*)EXCEPTION_VECTOR_BASE, 0x200);

    if (EXCEPT_LEN(tlb_refill) > 128 || EXCEPT_LEN(exception_handler) > 128 ||
	EXCEPT_LEN(exception_cache_error) > 128)
	panic("invalid exception handler size!\n");

    memcpy((dword_t*)EXCEPTION_TLB_REFILL, 
	   (dword_t*)tlb_refill, 
	   EXCEPT_LEN(tlb_refill));

    memcpy((dword_t*)EXCEPTION_XTLB_REFILL, 
	   (dword_t*)tlb_refill, 
	   EXCEPT_LEN(tlb_refill));


    memcpy((dword_t*)EXCEPTION_OTHER, 
	   (dword_t*)exception_handler, 
	   EXCEPT_LEN(exception_handler));

    memcpy((dword_t*)EXCEPTION_CACHE_ERROR, 
	   (dword_t*)exception_cache_error, 
	   EXCEPT_LEN(exception_cache_error));

    printf(__FUNCTION__);
    enter_kdebug();
}

void tlb_refill(dword_t val1, dword_t val2)
{
    printf("tlb refill: %x, %x)\n", val1, val2);
    panic("tlb");
}

void exception(dword_t cause, dword_t ip, dword_t status)
{
    printf("exception cause: %x (err: %d), ip: %x, status: %x\n", cause, 
	   (cause >> 2) & 0x1f, ip, status);
    enter_kdebug();
}

void mips_cache_error()
{
    panic(__FUNCTION__);
}

