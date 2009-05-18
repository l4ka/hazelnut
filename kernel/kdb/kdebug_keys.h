/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     kdebug_keys.h
 * Description:   Keystroke Definitions for the Kernel Debugger.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug_keys.h,v 1.8 2001/12/12 23:12:52 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __KDB__KDEBUG_KEYS_H__
#define __KDB__KDEBUG_KEYS_H__

#if defined(CONFIG_DEBUGGER_NEW_KDB)

//to make the tracebuffers readable...
#define YES               1
#define NO                0
#define TRACING           1
#define DISPLAY           0
#define THREADS_IN_LIST   1
#define ALL_OTHER_THREADS 0

/*
 * List of all keystrokes for the Kernel Debugger sorted by define. 
 * (Plese keep this  list sorted alphabetically.)
 */
#define K_CONFIG_STUFF         'o'
#define K_CONFIG_CACHE         'c'
#define K_CONFIG_DUMP_NAMES    'K'
#define K_CONFIG_KNOWN_NAMES   'n'
#define K_CONFIG_NAMES         's'

#define K_DUMP_TRACE           'T'
#define K_FRAME_DUMP           ' '

#define K_CPU_BREAKPOINT       'b'
#define K_CPU_DISASSEMBLE      'U'
#define K_CPU_PERFCTR          'e'
#define K_CPU_PROFILE          'c'
#define K_CPU_PROFILE_DUMP     'C'
//#define K_CPU_SINGLE_STEPPING  'T'
#define K_CPU_STATE            'A'
#define K_CPU_STUFF            'c'

#define K_GO                   'g'
#define K_HELP                 '?'
#define K_HELP2                'h'

#define K_KERNEL_EXCPTN_TRACE  'x'
#define K_KERNEL_INTERRUPT     'I'
#define K_KERNEL_IPC_TRACE     'i'
#define K_KERNEL_QUEUES        'Q'
#define K_KERNEL_STATS         '#'
#define K_KERNEL_STUFF         'k'
#define K_KERNEL_TASK_DUMP     'k'
#define K_KERNEL_TCB_DUMP      't'
#define K_KERNEL_TRACEPOINT    'r'

#define K_LEAVE_MENU           'q'

#define K_MEM_DUMP             'd'
#define K_MEM_DUMP_OTHER       'D'
#define K_MEM_MDB_DUMP         'm'
#define K_MEM_MDB_TRACE        'M'
#define K_MEM_PF_TRACE         'f'
#define K_MEM_PF_TRACE2        'P'
#define K_MEM_PTAB_DUMP        'p'
#define K_MEM_STUFF            'm'

#define K_RESET                '^'
#define K_RESET2               '6'
#define K_RESTRICT_TRACE       'r'
#define K_STEP_BLOCK           'S'
#define K_STEP_INSTR           's'



#else /* CONFIG_DEBUGGER_KDB */
/*
 * List of all keystrokes for the Kernel Debugger.  (Please keep this
 * list sorted alphabetically.)
 */

#define K_CPUSTATE		'A'
#define K_BREAKPOINT		'b'
#define K_PROFILE		'c'
#define K_PROFILE_DUMP		'C'
#define K_MEM_DUMP		'd'
#define K_MEM_OTHER_DUMP	'D'
#define K_PERFMON		'e'
#define K_PF_TRACE		'f'
#define K_GO			'g'
#define K_HELP2			'h'
#define K_IPC_TRACE		'i'
#define K_INTERRUPTS		'I'
#define K_TASK_DUMP		'k'
#define K_MDB_DUMP		'm'
#define K_MDB_TRACE		'M'
#define K_PTAB_DUMP		'p'
#define K_PF_TRACE2		'P'
#define K_QUEUES		'q'
#define K_TRACEPOINT		'r'
#define K_STEP_INSTR		's'
#define K_STEP_BLOCK		'S'
#define K_TCB_DUMP		't'
#define K_SINGLE_STEPPING	'T'
#define K_LAST_IPC		'u'
#define K_DISASSEMBLE		'U'
#define K_SWITCH_TRACE		'w'
#define K_SWITCH_TRACE_DUMP	'W'
#define K_SMP_INFO		'x'
#define K_SMP_SWITCH_CPU	'X'
#define K_TRACE_DUMP            'y'
#define K_TRACE_FLUSH           'Y'
#define K_FRAME_DUMP		' '
#define K_RESET2		'6'
#define K_STATISTICS		'#'
#define K_HELP			'?'
#define K_RESET			'^'



/*
 * Help strings for the different keystrokes.
 */

struct kdb_help_t {
    char  key;
    char  alternate_key;
    char *help;
} kdb_help[] = {
    { K_GO, 0,			"Continue" },
    { K_RESET, K_RESET2,	"Reset" },
    { K_FRAME_DUMP, 0,		"Dump Frame" },
    { K_TCB_DUMP, 0,		"Dump TCB" },
    { K_TASK_DUMP, 0,		"Dump Task" },
    { K_PTAB_DUMP, 0,		"Dump Page Table" },
    { K_MEM_DUMP, 0,		"Dump Memory" },
    { K_MEM_OTHER_DUMP, 0,	"Dump Memory (other space)" },
    { K_MDB_DUMP, 0,		"Dump Mapping DB" },
    { K_MDB_TRACE, 0,		"Trace Mapping DB" },
    { K_STATISTICS, 0,		"Dump Statistics" },
    { K_QUEUES,	 0,		"List Priority Queues" },
    { K_INTERRUPTS, 0,		"Interrupt Association" },
    { K_CPUSTATE, 0,		"CPU State" },

#if defined(CONFIG_DEBUG_TRACE_UPF) || defined(CONFIG_DEBUG_TRACE_KPF)
    { K_PF_TRACE, K_PF_TRACE2,	"Trace pagefaults" },
#endif

#if defined(CONFIG_DEBUG_TRACE_IPC)
    { K_IPC_TRACE, 0,		"Trace IPCs" },
    { K_LAST_IPC, 0,		"Show Last IPC" },
#endif

#if defined(CONFIG_ENABLE_SWITCH_TRACE)
    { K_SWITCH_TRACE, 0,	"Trace thread switches" },
    { K_SWITCH_TRACE_DUMP, 0,	"Dump thread switch trace" },
#endif

#if defined(CONFIG_DEBUG_TRACE_MBD)
    { K_MDB_TRACE, 0,		"Trace Mapping DB" },
#endif

#if defined(CONFIG_TRACEBUFFER)
    { K_TRACE_DUMP, 0,		"Dump tracebuffer" },
    { K_TRACE_FLUSH, 0,         "Flush tracebuffer" },
#endif

#if defined(CONFIG_ENABLE_TRACEPOINTS)
    { K_TRACEPOINT, 0, 		"Set/Clear Tracepoints" },
#endif

#if defined(CONFIG_ENABLE_PROFILING)   
    { K_PROFILE, 0,		"Profiling" },
    { K_PROFILE_DUMP, 0,	"Dump Profile" },
#endif

#if defined(CONFIG_PERFMON)
    { K_PERFMON, 0,		"Performance Monitoring" },
#endif

    { K_BREAKPOINT, 0,		"Set/Clear Breakpoints" },
    { K_SINGLE_STEPPING, 0,	"Single Stepping" },
    { K_STEP_INSTR, 0, 		"Step Instruction" },
    { K_STEP_BLOCK, 0,		"Step Block" },

#if defined(CONFIG_DEBUG_DISAS)
    { K_DISASSEMBLE, 0, 	"Disassemble" },
#endif

#if defined(CONFIG_SMP)
    { K_SMP_INFO, 0,		"SMP Info" },
    { K_SMP_SWITCH_CPU, 0,	"Switch to other CPU" },
#endif

    { K_HELP, K_HELP2,		"This Help Message" },

    { 0, 0, NULL } 
};


#endif /* CONFIG_DEBUGGER_NEW_KDB */

#endif /* !__KDB__KDEBUG_KEYS_H__ */
