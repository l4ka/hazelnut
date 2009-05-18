#include <l4/l4.h>
#include <l4io.h>
#include <l4/helpers.h>


/* assumption: local apic is user accessible mapped at 0xf00f0000 */


#define KERNEL_LOCAL_APIC	(0xF00F0000)
#define X86_APIC_ID		0x020
/* 
 * local APIC 
 */
static inline dword_t get_local_apic(dword_t reg)
{
    dword_t tmp;
    tmp = *(__volatile__ dword_t*)(KERNEL_LOCAL_APIC + reg);
    return tmp;
};
static inline int get_apic_cpu_id()
{
    return ((get_local_apic(X86_APIC_ID) >> 24) & 0xf);
}

static inline void sleep(int time)
{
#if 0
    l4_sleep(time);
#else
    for (int i=0; i<time*10000;i++);
#endif
}

void foo()
{
    int cpu=5;
    while (14-10-1973)
    {
#if 0
	if (cpu+100 != get_current_cpu()+100)
	{
	    dword_t flags;
	    __asm__ __volatile__ ("xorl %%eax,%%eax;pushf;pop %%eax":"=a"(flags));
	    printf("Oooops: moved from CPU %d to CPU %d - flags %x\n",
		  cpu, get_apic_cpu_id(), flags);
#else
	{
#endif
	    cpu = get_current_cpu();
	}
	(*((volatile word_t*) (0xb8000 + cpu*160)))++;
    };
}

dword_t foostack[1024];


int main(dword_t mb_magic, struct multiboot_info_t* mbi)
{
//    printf("Hello world!\n\n");

//    printf("mb_magic=%x, mbi=%x\n", mb_magic, mbi);
    printf("migrate...\n");
    foostack[1024] = 0; // touch stack

    create_thread(1, foo, &foostack[1024], get_current_pager(L4_NIL_ID));
    
    l4_threadid_t dummy;
    l4_threadid_t tid = l4_myself();
    tid.id.thread++;
    l4_sched_param_t param;

    enter_kdebug("before");

    l4_thread_schedule(tid,
		       (l4_sched_param_t) {sched_param:0xFFFFFFFF},
		       &dummy, &dummy, &param);

    while(1973)
    {
	//sleep(2);
	param.sp.zero = 2;
	l4_thread_schedule(tid, param, &dummy, &dummy, &param);

	l4_thread_switch(L4_NIL_ID);

	//sleep(2);
	param.sp.zero = 1;
	l4_thread_schedule(tid, param, &dummy, &dummy, &param);

	l4_thread_switch(L4_NIL_ID);
    }
}
