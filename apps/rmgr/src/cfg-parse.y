/* -*- indented-text -*- */

%{
#include <unistd.h>

#include <l4/rmgr/rmgr.h>

#include "quota.h"
#include "init.h"
#include "rmgr.h"
#include "cfg.h"

#define yyparse __cfg_parse

int yylex(void);
static void yyerror(const char *s);

unsigned __cfg_task;
static unsigned ctask;

static int assert_mod = -1;

static unsigned c_max = -1, c_low = 0, c_high = -1, c_mask = 0xffffffff;

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef max
#define max(x,y) ((x)>(y)?(x):(y))
#endif

#define minset(x, y) ((x) = min((x),(y)))
#define maxset(x, y) ((x) = max((x),(y)))

#define SET_DEBUG_OPT(proto, action) 					\
	do {								\
		debug_log_mask |= (1L << proto);			\
		if (action)						\
			debug_log_types = (2L | (action << 16) | 	\
					   (debug_log_types & 7));	\
	} while(0);
%}

%union {
  char *string;
  unsigned number;
  struct {
    unsigned low, high;
  } interval;
}

%token TASK MODNAME CHILD IRQ MAX IN MASK MEMORY HIMEM LOGMCP BOOTMCP BOOTPRIO
%token BOOTWAIT RMGR SIGMA0 DEBUGFLAG VERBOSE LOG
%token SMALLSIZE SMALL BOOTSMALL MODULE
%token TASK_PROTO TASK_ALLOC TASK_GET TASK_FREE 
%token TASK_CREATE TASK_DELETE TASK_SMALL
%token TASK_GET_ID TASK_CREATE_WITH_PRIO 
%token MEM_PROTO MEM_FREE MEM_FREE_FP 
%token IRQ_PROTO IRQ_GET IRQ_FREE 
%token RMGR_PROTO RMGR_PING
%token LOG_IT KDEBUG
%token <string> UNSIGNED STRING
%type <number> number
%type <interval> setspec
%type <string> string

/* grammer rules follow */
%%

file	: rules
	;

rules	: rule rules
	|
	;

rule	: taskspec constraints modules	
		{ 
		  if (ctask)
		    {
		      quota_t *q = & quota[ctask];
		      bootquota_t *b = &bootquota[ctask];
		      printf(
			"RMGR: configured task 0x%x: [ m:%x,%x,%x hm:%x,%x,%x\n"
			"      t:%x,%x,%x i:%x lmcp:%x s:%x,%x,%x\n"
			"      mcp:%x prio:%x small:%x ]\n",
			ctask, q->mem.low, q->mem.high, q->mem.max, 
			q->himem.low, q->himem.high, q->himem.max, 
			q->task.low, q->task.high, q->task.max, 
			q->irq.max, q->log_mcp,
			q->small.low, q->small.high, q->small.max,
			b->mcp, b->prio, b->small_space);
		    }
		  assert_mod = -1;
		}
					    
	| smallsizerule
	| flag
	;

smallsizerule : SMALLSIZE number{ small_space_size = $2; };

numerical_options: number number{ debug_log_mask = $1;
				  debug_log_types = $2; };

verbose_option: TASK_PROTO	{ SET_DEBUG_OPT(RMGR_TASK, 0); }
	     | TASK_ALLOC	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_ALLOC); }
	     | TASK_GET		{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_GET); }
	     | TASK_FREE	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_FREE); }
	     | TASK_CREATE	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_CREATE); }
	     | TASK_DELETE	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_DELETE); }
	     | TASK_SMALL	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_SET_SMALL); }
	     | TASK_GET_ID	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_GET_ID); }
	     | TASK_CREATE_WITH_PRIO	{ SET_DEBUG_OPT(RMGR_TASK, RMGR_TASK_CREATE_WITH_PRIO); }	     
	     | MEM_PROTO	{ SET_DEBUG_OPT(RMGR_MEM, 0); }
	     | MEM_FREE		{ SET_DEBUG_OPT(RMGR_MEM, RMGR_MEM_FREE); }
	     | MEM_FREE_FP	{ SET_DEBUG_OPT(RMGR_MEM, RMGR_MEM_FREE_FP); }
	     | IRQ_PROTO	{ SET_DEBUG_OPT(RMGR_IRQ, 0); }
	     | IRQ_GET		{ SET_DEBUG_OPT(RMGR_IRQ, RMGR_IRQ_GET); }
	     | IRQ_FREE		{ SET_DEBUG_OPT(RMGR_IRQ, RMGR_IRQ_FREE); }
	     | RMGR_PROTO	{ SET_DEBUG_OPT(RMGR_IRQ, 0); }
	     | RMGR_PING	{ SET_DEBUG_OPT(RMGR_IRQ, RMGR_RMGR_PING); }
	     | LOG_IT		{ debug_log_types |= 1; }
	     | KDEBUG		{ debug_log_types |= 4; }
	     ;

verbose_options: verbose_option verbose_options
	     | verbose_option
	     ;

log_options:   numerical_options
	     | verbose_options
	     ;

flag	: BOOTWAIT		{ boot_wait++; }
	| VERBOSE		{ verbose++; }
	| DEBUGFLAG		{ debug++; }
	| LOG log_options 
	;

modules	: module modules
	| 
	;

module	: nextmod asserts	{ bootquota[ctask].mods++; }
	;

nextmod	: MODULE		{ assert_mod++; }
	;

taskspec : TASK			{ ctask = ++__cfg_task; }
	| TASK taskname 
	| TASK RMGR		{ ctask = __cfg_task = myself.id.task;}
	| TASK SIGMA0		{ ctask = __cfg_task = my_pager.id.task; }
	| TASK number		{ ctask = __cfg_task = $2; }
	;

taskname: MODNAME string	
		{ 
		  char *n;
		  int i;
		  for (i = first_task_module; i < mb_info.mods_count; i++)
		    {
		      if (!(n = (char *) mb_mod[i].string))
		        {
			  ctask = ++__cfg_task;
		          printf("RMGR: WARNING: cmdlines unsupported, can't "\
			         "find modname %s; assuming task %d\n", $2,\
				 ctask);
			  goto taskname_end;
			}
		      if (strstr(n, $2))
		        {
			  ctask = __cfg_task
				= myself.id.task + 1 + (i - first_task_module);
			  assert_mod = i;
			  goto taskname_end;
			}
		    }
		  if (first_not_bmod_free_modnr >= MODS_MAX)
		    {
		      printf("RMGR: Could not configure more "\
			      "than %d tasks\n", MODS_MAX);
		      boot_error();
		      ctask = 0;
		      goto taskname_end;
		    }

		  ctask = __cfg_task = first_not_bmod_free_task++;
		  check(task_alloc(ctask, myself.id.task));
		  printf("RMGR: WARNING: couldn't find modname %s, "\
		         "only storing it's quota\n", $2);
		  strncpy(mb_mod_names[first_not_bmod_free_modnr], $2,\
			  MOD_NAME_MAX);
		  mb_mod_names[first_not_bmod_free_modnr][MOD_NAME_MAX-1] = 0;
		  first_not_bmod_free_modnr++;

		taskname_end:
		  free($2);
		};

asserts : assert asserts
	|
	;

assert	: MODNAME string	
		{ 
		  char *n;
		  if (assert_mod < 1)
		    {
		      printf("RMGR: ERROR: no boot module "\
			     "associated with modname %s\n", $2);
		      boot_error();
		      goto assert_end;
		    }
		  if (!(n = (char *)mb_mod[assert_mod].string))
		    {
		      printf("RMGR: WARNING: cmdlines "\
			     "unsupported, can't verify modname %s\n", $2);
		    }
		  else if (! strstr(n, $2))
		    {
		      printf("RMGR: ERROR: modname %s doesn't "\
			     "match cmdline %s\n", $2, n);
		      boot_error();
		    }
		assert_end:
		  free($2);
		}
	;

constraints : constraint constraints
	| 
	;

constraint : CHILD childconstraints	{ maxset(quota[ctask].task.low, c_low);
					  minset(quota[ctask].task.high, c_high);
					  minset(quota[ctask].task.max, c_max);
                                          c_low = 0; c_high = -1; c_max = -1; }
	| MEMORY memoryconstraints	{ maxset(quota[ctask].mem.low, c_low);
					  minset(quota[ctask].mem.high, c_high);
					  minset(quota[ctask].mem.max, c_max);
                                          c_low = 0; c_high = -1; c_max = -1; }
	| HIMEM memoryconstraints	{ maxset(quota[ctask].himem.low, c_low);
					  minset(quota[ctask].himem.high, c_high);
					  minset(quota[ctask].himem.max, c_max);
                                          c_low = 0; c_high = -1; c_max = -1; }
	| IRQ irqconstraints		{ unsigned mask = 0, i;
					  for (i = max(0, c_low);
					       i <= min(15, c_max);
					       i++)
					    {
                                              mask |= 1L << i;
					    }
					  quota[ctask].irq.max &= 
					    c_mask & mask;
                                          c_low = 0; c_high = -1; c_max = -1;
					  c_mask = 0xffffffff; }
	| SMALL smallconstraints	{ maxset(quota[ctask].small.low, c_low);
					  minset(quota[ctask].small.high, c_high);
					  minset(quota[ctask].small.max, c_max);
                                          c_low = 0; c_high = -1; c_max = -1; }
	| LOGMCP number			{ quota[ctask].log_mcp = $2; }
	| BOOTMCP number		{ bootquota[ctask].mcp = $2; }
	| BOOTPRIO number		{ bootquota[ctask].prio = $2; }
	| BOOTSMALL number		{ bootquota[ctask].small_space = $2; }
	;

childconstraints : numconstraints
	;

smallconstraints : numconstraints
	;

memoryconstraints : numconstraints
	;

irqconstraints : numconstraints
	| maskconstraints
	;

numconstraints : numconstraint numconstraints
	| numconstraint
	;

numconstraint : MAX number		{ c_max = min(c_max, $2); }
	| IN setspec			{ c_low = max(c_low, $2.low); 
					  c_high = min(c_high, $2.high); }
	;

setspec : '[' number ',' number ']'	{ $$.low = $2; $$.high = $4; }
	;

maskconstraints : maskconstraint
	;

maskconstraint : MASK number		{ c_mask &= $2; }
	;

number	: UNSIGNED			{ $$ = strtoul($1, 0, 0); }
	;

string	: STRING			{ $$ = strdup($1 + 1);
					  $$[strlen($$) - 1] = 0; };

%%
/* end of grammer -- misc C source code follows */

#include "cfg-scan.c"

static void yyerror(const char *s)
{
  printf("RMGR: ERROR: while parsing config file: %s\n",
	 s);
//	 "             at line %d, col %d\n", s, line, col);
}


#ifdef TEST

char cfg[] = "task modname 'foo'\n"
             "#a comment\n"
             "child in [10,30] max 100\n";

int main(void)
{
  cfg_setup_input(cfg, cfg + sizeof(cfg));

  return cfg_parse();
}

#endif /* TEST */
