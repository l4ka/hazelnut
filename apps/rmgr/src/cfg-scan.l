/* -*- indented-text -*- */

%{
#include <string.h>

#include "cfg.h"

int yylex(void);
static int yywrap(void);

/* we use a memory-based input buffer */

static int my_yyinput(char *buf, int max_size);

#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = my_yyinput(b, ms))

#define YY_NEVER_INTERACTIVE 1

static unsigned line = 1, col = 1;

/* make sure we don't use file I/O */

#undef ECHO
#define ECHO

#undef fprintf
#define fprintf(stream, args...) printf(args)

%}

%start COMMENT

/* shortcut definitions */

DIGIT		[0-9]
HEXDIGIT	[a-fA-F0-9]
SPACE		[\f\r\t\032 ]
NEWLINE 	\n

/* lexing rules follow */
/* ^#.*$		; */
%%

#		BEGIN COMMENT;
<COMMENT>.*$	BEGIN 0;
rmgr		{ col += 4; return RMGR; }
sigma0		{ col += 6; return SIGMA0; }
task		{ col += 4; return TASK; }
module		{ col += 6; return MODULE; }
end		{ col += 3; return 0; }	/* end token */
modname		{ col += 7; return MODNAME; }
child		{ col += 5; return CHILD; }
irq		{ col += 3; return IRQ; }
max		{ col += 3; return MAX; }
in		{ col += 2; return IN; }
mask		{ col += 4; return MASK; }
memory		{ col += 6; return MEMORY; }
high_memory	{ col += 11; return HIMEM; }
log_mcp		{ col += 7; return LOGMCP; }
boot_mcp	{ col += 8; return BOOTMCP; }
boot_priority	{ col += 13; return BOOTPRIO; }
small		{ col += 5; return SMALL; }
boot_small	{ col += 10; return BOOTSMALL; }
bootwait	{ col += 8; return BOOTWAIT; }
small_space_size	{ col += 16; return SMALLSIZE; }
debug		{ col += 5; return DEBUGFLAG; }
debug_log	{ col += 9; return LOG; }
verbose		{ col += 7; return VERBOSE; }
task_proto	{ col += 10; return TASK_PROTO; }
task_alloc	{ col += 10; return TASK_ALLOC; }
task_get	{ col += 8; return TASK_GET; }
task_free	{ col += 9; return TASK_FREE; }
task_create	{ col += 11; return TASK_CREATE; }
task_delete	{ col += 11; return TASK_DELETE; }
task_create_with_prio	  { col += 21 ; return TASK_CREATE_WITH_PRIO; }
mem_proto	{ col += 9 ; return MEM_PROTO; }
mem_free	{ col += 8 ; return MEM_FREE; }
mem_free_fp	{ col += 11 ; return MEM_FREE_FP; }
irq_proto	{ col += 9 ; return IRQ_PROTO; }
irq_get		{ col += 7; return IRQ_GET; }
irq_free	{ col += 8; return IRQ_FREE; }
rmgr_proto	{ col += 10; return RMGR_PROTO; }
rmgr_rmgr_ping	{ col += 14; return RMGR_RMGR_PING; }
log		{ col += 3; return LOG_IT; }
kdebug		{ col += 6; return KDEBUG; }
(0[xX]{HEXDIGIT}+|{DIGIT}+)	{ col += strlen(yytext);
		  yylval.string = yytext; return UNSIGNED; }
('[^']*'|\"[^\"]*\")	{ col += strlen(yytext);
			  yylval.string = yytext; return STRING; }
{SPACE}		{ col++; }
{NEWLINE}	{ line++; col = 1; }
.		{ col++; return yytext[0]; }

%%
/* end of lexing rules -- general C stuff follows */

/* from John R. Levine, Tony Mason, Doug Brown; Lex&Yacc; UNIX
   Programming Tools; O'Reilly&Associates, Inc; Second Edition;
   October 1992; ISBN 1-56592-000-7., "Input from Strings" pp. 156: */

static const char *myinput, *myinputptr, *myinputlim;

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

static int my_yyinput(char *buf, int max_size)
{
  int n = min(max_size, myinputlim - myinputptr);

  if (n > 0) {
    memcpy(buf, myinputptr, n);
    myinputptr += n;
  }
  return n;
}

void cfg_setup_input(const char *cfg_buffer, const char *cfg_buffer_end)
{
  myinput = myinputptr = cfg_buffer;
  myinputlim = cfg_buffer_end;
}

static int yywrap(void)		/* called when lexing is finished */
{
  return 1;
}
