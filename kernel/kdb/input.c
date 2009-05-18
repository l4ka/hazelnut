/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     input.c
 * Description:   Various kdebug input functions.
 *                
 * @LICENSE@
 *                
 * $Id: input.c,v 1.14 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include "kdebug.h"

#if defined(CONFIG_DEBUGGER_NEW_KDB)

void enter_string(char* str);
dword_t resolve_name(char* name);

//name table
struct {
    dword_t tid;
    char name[9];
} name_tab[32];           

extern int name_tab_index;

#endif

tcb_t *kdebug_get_thread(tcb_t *current)
{
    l4_threadid_t tid;

    printf("Thread no [current]: ");

    tid.raw = kdebug_get_hex((dword_t) current, "current");
    if ( tid.raw < TCB_AREA ) tid.raw = (dword_t) tid_to_tcb(tid);
    else tid.raw = (tid.raw & L4_TCB_MASK);

    return (tcb_t *) tid.raw;
}


tcb_t *kdebug_get_task(tcb_t *current)
{
    l4_threadid_t tid;

    printf("Task no [current]: ");

    tid.raw = kdebug_get_hex((dword_t) current, "current");
    if ( tid.raw < (1 << L4_X0_TASKID_BITS) )
	tid = (l4_threadid_t) { x0id: {0, 0, tid.raw, 0} };
    if ( tid.raw < TCB_AREA ) tid.raw = (dword_t) tid_to_tcb(tid);
    tid.x0id.thread = 0;
    tid.raw = (tid.raw & L4_TCB_MASK);

    return (tcb_t *) tid.raw;
}

ptr_t kdebug_get_pgtable(tcb_t *current, char *str)
{
    tcb_t *tcb;
    ptr_t pgtab;
    int first_time;

    if ( str )
	printf("%s", str);
    
    pgtab = (ptr_t) kdebug_get_hex((dword_t) get_current_pgtable(), "current");

    if ( (dword_t) pgtab < (1 << L4_X0_TASKID_BITS) )
    {
	/* Seems like a task id.  Search for valid thread. */
	for ( tcb = current, first_time = 1;
	      tcb != current || first_time;
	      tcb = tcb->present_next, first_time = 0 )
	    if ( tcb->myself.x0id.task == (dword_t) pgtab)
	    {
		pgtab = tcb->space->pagedir_phys ();
		break;
	    }
    }
    else if ( (dword_t) pgtab < TCB_AREA )
    {
	/* Might be a thread id.  Search for match. */
	for ( tcb = current, first_time = 1;
	      tcb != current || first_time;
	      tcb = tcb->present_next, first_time = 0 )
	    if ( tcb->myself.raw == (dword_t) pgtab)
	    {
		pgtab = tcb->space->pagedir_phys ();
		break;
	    }
    }
    else if ( ((dword_t) pgtab >= TCB_AREA &&
	       (dword_t) pgtab < (TCB_AREA + TCB_AREA_SIZE)) )
	/* Pointer into TCB area. */
	pgtab = ((tcb_t *) ((dword_t) pgtab & L4_TCB_MASK))->space->pagedir_phys ();
    else
	/* Hopefully a valid page table address. */
	;

    return pgtab;
}

dword_t kdebug_get_hex(dword_t def, const char *str)
{
    dword_t val = 0;
#if !defined(CONFIG_DEBUGGER_NEW_KDB)
    char t, c;
#endif
    int i = 0;

#if defined(CONFIG_DEBUGGER_NEW_KDB)
    char name[8];

    enter_string(name);
    if (name[0] != '\0') { 
	val = resolve_name(name);
	i = 1;
    }
#else
    while ( (t = c = getc()) != '\r' )
    {
	switch(c)
	{
	case '0' ... '9': c -= '0'; break;
	case 'a' ... 'f': c -= 'a' - 'A';
	case 'A' ... 'F': c = c - 'A' + 10; break;
	case 8:  val >>= 4; i--; putc(8); putc(' '); putc(8); continue;
	case 'x': if ( val == 0 && i == 1 ) { i--; putc('x'); }
	default: continue;
	}
	val <<= 4;
	val += c;
	putc(t);
	if ( ++i == 8 )
	    break;
    }
#endif
    if ( i == 0 )
    {
	/* No value given.  Use default. */
	val = def;
	if ( str ) printf("%s", str);
	else printf("%x", val);
    }

    putc('\n');
    return val;
}


dword_t kdebug_get_dec(dword_t def, const char *str)
{
    dword_t val = 0, no_input = 1;
    char c;

    while ( (c = getc()) != '\r' )
    {
	switch(c)
	{
	case '0' ... '9':
	    val = val * 10 + (c - '0');
	    break;
	default:
	    continue;
	}
	putc(c);
	no_input = 0;
    }

    if ( no_input )
    {
	/* No value given.  Use default. */
	val = def;
	if ( str ) printf("%s", str);
	else printf("%d", val);
    }

    putc('\n');
    return val;
}



static char *get_key_string(char key, const char *choices) L4_SECT_KDEBUG;

char kdebug_get_choice(const char *str, const char *choices,
		       char def, int no_newline)
{
    char *p, c;

    printf("%s (%s)", str, choices);
    if ( (p = get_key_string(def, choices)) )
    {
	printf(" [");
	for (; *p != '/' && *p != 0; p++ )
	    putc(*p >= 'A' && *p <= 'Z' ? (*p + ('a'-'A')) : *p);
	putc(']');
    }
    printf(": ");

    for (;;)
    {
	c = getc();
	p = get_key_string(c == '\r' ? def : c, choices);
	for (; p && *p != '/' && *p != 0; p++ )
	    putc(*p >= 'A' && *p <= 'Z' ? (*p + ('a'-'A')) : *p);
	if ( p || c == '\r' )
	    break;
    }

    if ( ! no_newline )
	putc('\n');

    return (c == '\r') ? def : c;
}

static char *get_key_string(char key, const char *choices)
{
    const char *p;

    if ( key == 0 ) return NULL;

    for ( p = choices; *p; p++ )
    {
	if ( (*p >= 'A' && *p <= 'Z') && (*p + ('a'-'A')) == key )
	    break;
	if ( (p == choices || p[-1] == '/') && (p[1] == 0 || p[1] == '/') &&
	     (*p == key) )
	    break;
    }

    if ( *p == 0 ) return NULL;
    while ( p > choices && p[-1] != '/' ) p--;
    return (char *) p;
}


#if defined(CONFIG_DEBUGGER_NEW_KDB)

dword_t to_hex(char* name);

void strcpy(char *dest, char *src) {
    
    int i = 0;
    while (src[i]) {
	dest[i] = src[i];
	i++;
    }
    dest[i] = '\0';
}

//other return value as known!
int strcmp(char *str1, char *str2) {
    
    int i = 0;
    
    while (str1[i] || str2[i]) {
	if (str1[i] != str2[i])
	    return 0;
	i++;
    }   
    return 1;
}

//gives the tid of a thread
dword_t resolve_name(char* name) {
    for (int i=0; i<32; i++) 
	if (strcmp(name_tab[i].name, name))
	    return name_tab[i].tid;
    //the thread has no name
    return to_hex(name);
}

//to use this in other functions too (e.g. exception monitoring)
//rest of restrict
//takes a string and converts it into a dword;
dword_t to_hex(char* name) {
    char c;
    dword_t res = 0;
    
    for (int i=0; name[i]; i++) {//until string not ended
	c = name[i];
	if (c >= 48 &&c <= 57) c = c - 48;       //0,..,9
	else if (c >= 97 && c <= 102) c = c - 87;//a,..,f
	else {
	    printf("!!unknown name!!\n");
	    return 0xffffffff;
	}
	if (i>=8) printf("warning: overrun in returned dword! first bits are lost!");
	res = res << 4;//works, because only the lowest 4 bits of c are used
	res = res | c;
    }
    return res;
}

void dump_names() {
    putc('\n');
    //check, if the adress is in the pagetable of the thread!!!
    for (int i=0; i<name_tab_index ; i++)
	printf("%x is %s \n", name_tab[i].tid, name_tab[i].name);
}

/**
 * name resolution
 **/
char* get_name(dword_t tid, char* res) {
    
    int c;
    int i;
    
    for (i=0; i<32; i++) 
	if (name_tab[i].tid == tid) {
	    strcpy(res, name_tab[i].name);
	    return res;
	}
    //the thread has no name
    for (i=0; i<8; i++) {
	c = tid & 0x0000000f;
	if (c < 10) c = c+48;//0,..,9
	else c = c+87;       //a,..,f
	res[7-i] = c;
	tid = tid >> 4;
    }
    res[8] = '\0';
    return res;
}

//backspace dosn't work...
void enter_string(char *str) {
    char c;
    int i = 0;

    while ( (c = getc()) != '\r' ) {
	str[i] = c;
	putc(c);
	if (c == 0x08) i--; //backspace
	if (++i == 8) { //to be able to delete the last character
	    if ( (c = getc()) == 0x08 ) {
		putc(c);
		i--;
	    }
	    else break;
	}
    }
    str[i] = '\0';
    putc(' ');
}

/**
 * name a thread
 **/
void set_name() {
    
    int tmp = -1;
    dword_t tid;
    
    if (name_tab_index > 31) {
	enter_kdebug("too many names");
	return;
    }
    
    printf("thread id: ");
    tid = kdebug_get_hex(0xffffffff, "invalid");

    //rename or new name?
    for (int i=0; i<name_tab_index; i++) {
	if (name_tab[i].tid == tid)
	    tmp = i;
    }
    
    //a new name
    if (tmp == -1) {
	tmp = name_tab_index;
	name_tab[name_tab_index].tid = tid;
	name_tab_index ++;
    }
    
    printf(" name: ");
    enter_string(name_tab[tmp].name);    
    putc('\n');
}
#endif
