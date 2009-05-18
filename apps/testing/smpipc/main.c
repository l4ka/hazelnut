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
    //int cpu=5;
    l4_threadid_t tid;
    l4_msgdope_t msgdope;
    dword_t dw0, dw1, dw2;

    /* wait for migration */
    while(get_current_cpu() == 0);

    //enter_kdebug("foo migrated\n");

    l4_ipc_wait(&tid, NULL, &dw0, &dw1, &dw2, L4_IPC_NEVER, &msgdope);
    printf("foo received: %x,%x,%x\n", dw0, dw1, dw2);
    enter_kdebug("foo: received ipc");

    while(1);
}

dword_t foostack[1024];


int main(dword_t mb_magic, struct multiboot_info_t* mbi)
{
    printf("smpipc...\n");
    foostack[1023] = 0; // touch stack

    create_thread(1, foo, &foostack[1024], get_current_pager(L4_NIL_ID));
    
    l4_threadid_t dummy;
    l4_threadid_t tid = l4_myself();
    tid.id.thread++;
    l4_sched_param_t param;
    dword_t dw0, dw1, dw2;
    l4_msgdope_t msgdope;

    enter_kdebug("before");

    /* migrate partner thread */
    l4_thread_schedule(tid,
		       (l4_sched_param_t) {sched_param:0xFFFFFFFF},
		       &dummy, &dummy, &param);

    param.sp.zero = 2;
    l4_thread_schedule(tid, param, &dummy, &dummy, &param);

    //enter_kdebug("foo migration done");

    l4_ipc_call(tid, NULL, 1, 2, 3, NULL, &dw0, &dw1, &dw2, 
		L4_IPC_NEVER,&msgdope);

    enter_kdebug("main done");

}
