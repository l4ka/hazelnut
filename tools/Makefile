
/tmp/dump_tcb: /tmp/dump_tcb.c /tmp/_thread.h /tmp/thread.c
	@gcc -I../kernel/include --save-temps -D__ARCH__=x86 -DASSEMBLY -x c++ -o $@ /tmp/dump_tcb.c
	@$@
	@rm $@ $^

/tmp/thread.c:
	@( cat ../kernel/include/tcb.h | awk 'BEGIN{printme=0} /} tcb_t;/ { printme = 0} { if (printme == 1 ) { print  } } /typedef struct tcb_t {/ {printme = 1 } ' ) | cpp -P | tr '*[;' '   ' | sed -e 's/[ \t\\]*/ /' | awk -F' ' '{print $$2}' | grep -v '^$$' | awk '{ print "D("$$1");" } ' > $@

/tmp/_thread.h:
	@( cat ../kernel/include/tcb.h | awk 'BEGIN{printme=1} /typedef struct tcb_t {/ {printme = 1 } { if (printme == 1 ) { print $0 } } /} tcb_t;/ { printme = 0}' ; echo "#endif" ) > $@

/tmp/dump_tcb.c:
	@printf '\
#include "config.h"						\n\
#include "macros.h"						\n\
#include INC_ARCH(config.h)					\n\
#include "types.h"						\n\
#include "thread.h"						\n\
#define notify_procedure(x...)					\n\
#include "ipc.h"						\n\
#include "/tmp/_thread.h"					\n\
								\n\
#include <stdio.h>						\n\
								\n\
tcb_t tcb;							\n\
								\n\
#define OFS(x) (((dword_t)&tcb.##x) - (dword_t)&tcb)		\n\
								\n\
#define D(x)	printf("%%-20s:%%10x%%10d\\n", #x, OFS(x), OFS(x));	\n\
								\n\
int main() 							\n\
{								\n\
    #include "/tmp/thread.c"					\n\
}								\n' > $@
