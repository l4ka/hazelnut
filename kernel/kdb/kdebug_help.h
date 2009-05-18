/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     kdebug_help.h
 * Description:   Help definitions for kdebug keystrokes.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug_help.h,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#if !defined(__KDEBUG_HELP__)
#define __KDEBUG_HELP__

#include <universe.h>
#include "kdebug_keys.h"
/*
 * Help strings for the different keystrokes.
 */

struct kdb_help_t {
    char  key;
    char  alternate_key;
    char *help;
} kdb_general_help[] = {
    { K_GO, 0,                         "Continue" },
    { K_RESET, K_RESET2,               "Restart the Computer" },
    { K_CONFIG_STUFF, 0,               "Debugger configuration" },
    { K_FRAME_DUMP, 0,                 "Dump frame" },
    { K_MEM_STUFF, 0,                  "Memory part" },
    { K_CPU_STUFF, 0,                  "CPU dependent part" },
    { K_KERNEL_STUFF, 0,               "Kernel things" },
    { K_STEP_INSTR, 0,                 "Step Instruction" },
    { K_DUMP_TRACE, 0,                 "Dump Trace" },
    { K_STEP_BLOCK, 0,                 "Step Block" },
    { 0, 0, NULL } 
}, 
kdb_cpu_help[] = {
    { K_CPU_STATE, 0,                  "CPU state" },
    { K_CPU_PERFCTR, 0,                "Performance counter" },
#if defined(CONFIG_DEBUG_DISAS)
    { K_CPU_DISASSEMBLE, 0,            "Disassembler" },
#endif
#if defined(CONFIG_ENABLE_PROFILING)   
    { K_CPU_PROFILE, 0,                "Profiling" },
    { K_CPU_PROFILE_DUMP, 0,           "Dump Profile" },
#endif
    { K_CPU_BREAKPOINT, 0,             "set/clear breakpoints" },
//    { K_CPU_SINGLE_STEPPING, 0,        "Single stepping" },
    { 0, 0, NULL }  
}, 
kdb_kernel_help[] = {
    { K_KERNEL_STATS, 0,               "Dump statistics" },
    { K_KERNEL_QUEUES, 0,              "List Priority Queues" },
    { K_KERNEL_IPC_TRACE, 0,           "IPC tracing" },
    { K_KERNEL_EXCPTN_TRACE, 0,        "Exception tracing" },
    { K_KERNEL_INTERRUPT, 0,           "Interrupt Association" },
    { K_KERNEL_TASK_DUMP, 0,           "Dump Task" },
    { K_KERNEL_TCB_DUMP, 0,            "Dump TCB" },
#if defined(CONFIG_ENABLE_TRACEPOINTS)
    { K_KERNEL_TRACEPOINT, 0, 		"Set/Clear Tracepoints" },
#endif
    { 0, 0, NULL } 
}, 
kdb_mem_help[] = {
    { K_MEM_DUMP, 0,                   "Dump Memory" },
    { K_MEM_DUMP_OTHER, 0,             "Dump Memory (other space)" },
    { K_MEM_MDB_DUMP, 0,               "Dump Mapping DB" },
#if defined(CONFIG_DEBUG_TRACE_MBD)
    { K_MEM_MDB_TRACE, 0,              "Trace Mapping DB" },
#endif
    { K_MEM_PTAB_DUMP, 0,              "Dump Pagetable" },
    { K_MEM_PF_TRACE, K_MEM_PF_TRACE2, "Pagefault tracing" },
    { 0, 0, NULL } 
},
kdb_config_help[] = {
    { K_CONFIG_CACHE, 0,                "Enable/disable cache while in debugger. Starting disabled." },
    { K_CONFIG_DUMP_NAMES, 0,           "List of Nicknames" },
    { K_CONFIG_KNOWN_NAMES, 0,          "Set some known names" },
    { K_CONFIG_NAMES, 0,                "Name a thread" },
    { 0, 0, NULL } 
};

#endif
