/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     tracepoint_list.h
 * Description:   List of available kernel tracepoints.
 *                
 * @LICENSE@
 *                
 * $Id: tracepoint_list.h,v 1.13 2002/05/13 13:04:29 stoess Exp $
 *                
 ********************************************************************/
#if defined(DEFINE_TP)

DEFINE_TP(SYS_IPC, 		"sys_ipc(src=%x, dest=%x)")
DEFINE_TP(SYS_SCHEDULE,		"sys_schedule")
DEFINE_TP(SYS_THREAD_SWITCH, 	"sys_thread_switch(dest=%x)")
DEFINE_TP(SYS_LTHREAD_EX_REGS,	"sys_lthread_ex_regs")
DEFINE_TP(SYS_TASK_NEW,		"sys_task_new(tid=%x, pager=%x)")
DEFINE_TP(SYS_ID_NEAREST, 	"sys_id_nearest(tid=%x, uip=%x)")
DEFINE_TP(SYS_FPAGE_UNMAP, 	"sys_fpage_unmap(fpage=%x, mask=%x)")

DEFINE_TP(SET_PRIO,		"set_prio")
DEFINE_TP(GET_KERNEL_VERSION,	"get_kernel_version")
DEFINE_TP(EXTENDED_TRANSFER,	"extended_transfer")
DEFINE_TP(LONG_SEND_IPC,	"long_send_ipc")
DEFINE_TP(LONG_LONG_IPC,	"long_long_ipc")
DEFINE_TP(KERNEL_USER_PF,	"kernel_user_pf(fault=%x, ip=%x)")
DEFINE_TP(STRING_COPY_IPC,	"string_copy_ipc")
DEFINE_TP(STRING_COPY_PF,	"string_copy_pf")
DEFINE_TP(UNWIND_IPC,		"unwind_ipc")
DEFINE_TP(SAVE_RESOURCES,	"save_resources")
DEFINE_TP(LOAD_RESOURCES,	"load_resources")
DEFINE_TP(PURGE_RESOURCES,	"purge_resources")
DEFINE_TP(SWITCH_TO_IDLE,	"switch_to_idle")
DEFINE_TP(HANDLE_TIMER_INTERRUPT, "handle_timer_interrupt")

#if defined(CONFIG_ARCH_X86)
DEFINE_TP(IDT_FAULT,		"idt_fault")
DEFINE_TP(FPU_VIRTUALIZATION,	"fpu_virtualization")
DEFINE_TP(SEGREG_RELOAD,	"segreg_reload")
DEFINE_TP(KDB_OPERATION,	"kdb_operation")
DEFINE_TP(INTERRUPT,		"interrupt(#%d)")
DEFINE_TP(INSTRUCTION_EMULATION,"instruction_emulation")
#endif

DEFINE_TP(SYNC_TCB_AREA,	"sync_tcb_ares")
DEFINE_TP(MAP_NEW_TCB,		"map_new_tcb")
DEFINE_TP(MAP_ZERO_PAGE,	"map_zero_page")

#if defined(CONFIG_ENABLE_SMALL_AS)
DEFINE_TP(SET_SMALL_SPACE,	"set_small_space")
DEFINE_TP(SET_SMALL_SPACE_FAILED,"set_small_space_failed")
DEFINE_TP(ENLARGE_SPACE, 	"enlarge_space")
DEFINE_TP(RESTART_IPC,		"restart_ipc")
DEFINE_TP(SMALL_SPACE_RECEIVER,	"small_space_receiver")
DEFINE_TP(SMALL_SPACE_SENDER,	"small_space_sender")
DEFINE_TP(COPY_SMALL_PDIRENT,	"copy_small_pdirent")
#endif


#endif /* DEFINE_TP */
