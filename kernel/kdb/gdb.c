/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     gdb.c
 * Description:   GDB interface for the kernel debugger.
 *                
 * @LICENSE@
 *                
 * $Id: gdb.c,v 1.3 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_DEBUGGER_GDB)
#include "kdebug.h"

dword_t __kdebug_ipc_tracing = 0;
dword_t __kdebug_ipc_tr_mask = 0;
dword_t __kdebug_ipc_tr_thread = 0;
dword_t __kdebug_ipc_tr_dest = 0;
dword_t __kdebug_ipc_tr_this = 0;
int __kdebug_pf_tracing = 0;

#if defined(CONFIG_DEBUG_TRACE_MDB)
int __kdebug_mdb_tracing = 0;
#endif

static char * hexchars = "0123456789abcdef";
l4_threadid_t gdb_current_thread;

struct last_ipc_t
{
    l4_threadid_t myself;
    l4_threadid_t dest;
    dword_t snd_desc;
    dword_t rcv_desc;
    dword_t ipc_buffer[3];
    dword_t ip;
} last_ipc;

void panic(const char *msg)
{
    printf("panic: %s", msg);
    enter_kdebug("panic");
    while(1);
}

static int hex(char c)
{
  if ((c >= 'a') && (c <= 'f')) return (c-'a'+10);
  if ((c >= '0') && (c <= '9')) return (c-'0');
  if ((c >= 'A') && (c <= 'F')) return (c-'A'+10);
  return (-1);
}

static void int2hex(int val, unsigned char * buf)
{
    buf[0] = hexchars[(val >> 4) & 0xf];
    buf[1] = hexchars[(val >> 0) & 0xf];
}

static int mem2hex(unsigned char * addr, unsigned char * buf, int count)
{
    while(count)
    {
	int2hex(*addr, buf);
	addr++;
	buf+=2;
	count--;
    }
    *buf = 0;
    return 1;
}

static int string_to_int(unsigned char * str)
{
    int val = 0;
    unsigned char * tmp = str;
    while(*tmp)
    {
	switch(*tmp)
	{
	case '0' ... '9':
	    val = val * 16 + (*tmp - '0'); break;
	case 'a' ... 'f':
	    val = val * 16 + (*tmp - 'a' + 10); break;
	case 'A' ... 'F':
	    val = val * 16 + (*tmp - 'A' + 10); break;
	case '-':
	    break;
	default:
	    return val;
	}
	tmp++;
    }

    if (str[0] == '-') 
	return 0 - val;
    else
	return val;
}

int strncmp(const char * s1, const char * s2, int len)
{
    for (int i = 0; i  < len; i++) 
    {
	if (s1[i] < s2[i]) return -1;
	if (s1[i] > s2[i]) return 1;
    }
    return 0;
}

void serial_putc(const char c);
char serial_getc(void);

#define GDBSPIN do { spin1(23); } while(0)
#define GDB_BUF_SIZE	128
#define GDB_GETC	serial_getc
#define GDB_PUTC	serial_putc
static unsigned char gdb_buffer[GDB_BUF_SIZE];

void gdb_send_response(char * msg = NULL)
{
    unsigned char checksum;

    /*  $<packet info>#<checksum>. */
    do {
	GDB_PUTC('$');
	checksum = 0;
	unsigned char * buf = msg ? (unsigned char*)msg : gdb_buffer;

	while ( *buf )
	{
	    GDB_PUTC( *buf );
	    checksum += *buf;
	    buf++;
	}
	
	GDB_PUTC('#');
	GDB_PUTC(hexchars[checksum >> 4]);
	GDB_PUTC(hexchars[checksum % 16]);
    } while ( GDB_GETC() != '+' );
}

static unsigned char * gdb_receive_request()
{
    unsigned char checksum;
    unsigned char rcv_checksum;
    int count;
    char c;


    while(1) 
    {
	// wait for start character
	while ( GDB_GETC() != '$' ) GDBSPIN;

    restart:
	checksum = 0;
	count = 0;
	
	// now receive message into buffer
	while ( count < GDB_BUF_SIZE )
	{
	    c = GDB_GETC();
	    GDBSPIN;
	    if (c == '$')
		goto restart;

	    if (c == '#')
	    {
		// mark end of message
		gdb_buffer[count] = 0;

		rcv_checksum = ( hex(GDB_GETC()) << 4 ) + hex(GDB_GETC());
		if ( rcv_checksum != checksum)
		{
		    // invalid checksum
		    GDB_PUTC('-');
		    break;
		}
		else
		{
		    GDB_PUTC('+');
		    return &gdb_buffer[0];
		}
	    }

	    // special handling for 0x7d!!!
	    if (c == 0x7d)
	    {
		checksum = checksum + c;
		c = GDB_GETC();
	    }

	    // now the default handling - add char to buffer
	    checksum = checksum + c;
	    gdb_buffer[count] = c;
	    count = count + 1;
	}
    }
}

static void gdb_send_registers(exception_frame_t * frame)
{
    struct {
	dword_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
	dword_t eip, flags;
	dword_t cs, ss, ds, es, fs, gs;
    } registers = {
	frame->eax, frame->ecx, frame->edx, frame->ebx, frame->esp, 
	frame->ebp, frame->esi, frame->edi,
	frame->fault_address, frame->eflags,
	frame->cs, frame->ss, frame->ds, frame->es, 0 , 0};
    mem2hex((unsigned char*)&registers, gdb_buffer, sizeof(registers));
    gdb_send_response();
}
    
static void gdb_send_mem(unsigned char * addr, int len)
{
    //printf("gdb_send_mem(%p, %x)\n", addr, len);
    if ((addr > (unsigned char*)TCB_AREA) && 
	len > 0 )
    {
	// XXX: check pagetable!!!
	mem2hex(addr, gdb_buffer, len);
	gdb_send_response();
    }
    else
	gdb_send_response("E00");
}

void kdebug_entry(exception_frame_t* frame)
{
    if (kdebug_arch_entry(frame) == 0)
    {
	printf("gdb-debug entry\n");
	gdb_send_response("S05");

	int exit_kdb = 0;
	while( !exit_kdb )
	{
	    unsigned char * buf = gdb_receive_request();
	    //printf("gdb-cmd: %s\n", buf);

	    switch(buf[0])
	    {
	    case '?':
		gdb_send_response("S00");	// reason for stopping
		break;

	    case 'c':
	    case 'D':
		if (buf[1] != 0)
		    printf("continue at some address not supported yet - ignored\n");
		kdebug_single_stepping(frame, 0); // disable single stepping
		exit_kdb = 1;
		break;

	    case 'm':
	    {
		/* format: mnnn,size */
		unsigned char * addr = (unsigned char*)string_to_int(&buf[1]);
		unsigned char * tmp;
		int len = 0;
		/* now try to find "," */
		for (tmp = &buf[2]; *tmp && *tmp != ','; tmp++);
		if (*tmp == ',')
		{
		    tmp++;
		    len = string_to_int(tmp);
		}
		// sending with len 0 is invalid!!!
		gdb_send_mem(addr, len);
		break;
	    }
	    case 'X':
	    {
		/* format: Maddr,len:val */
		unsigned char * addr = (unsigned char*)string_to_int(&buf[1]);
		if (addr < (unsigned char*)TCB_AREA)
		{
		    gdb_send_response("E00");
		    break;
		}

		unsigned char * tmp;
		int len = 0;

		/* now find "," */
		for (tmp = &buf[2]; *tmp && *tmp != ','; tmp++);
		if (*tmp == ',')
		{
		    tmp++;
		    len = string_to_int(tmp);
		    /* and finally the data */
		    while(*tmp && *tmp != ':') tmp++;
		    if (*tmp == ':')
		    {
			/* here follows the data */
			tmp++;
			printf("set memory @%x, len=%x, val=%s\n",
			       addr, len, tmp);

			while(len)
			{
			    printf("set mem %p to %x\n", addr, *tmp);
			    (*addr) = (*tmp);
			    addr++;tmp++;
			    len--;
			}
			gdb_send_response("OK");
			break;
		    }
		}
		gdb_send_response("E00");
	    } break;
	    case 'H':
		switch(buf[1]) 
		{
		case 'c': {
		    if (buf[2] == '-' && buf[3] == '1')
		    gdb_current_thread = L4_INVALID_ID;
		    else
		    gdb_current_thread.id.thread = string_to_int(&buf[2]);
		    gdb_send_response("OK");
		} break;
		case 'g':
		    gdb_send_response("E01");	// not implemented
		    break;
		} break; // 'H'

	    case 'g':
		gdb_send_registers(frame);
		break;

	    case 'P': // set register value
	    {
		if (buf[3] != '=') {
		    int reg = string_to_int(&buf[1]);
		    int val = string_to_int(&buf[4]);

		    switch(reg)
		    {
		    case 0: frame->eax = val; break;
		    case 1: frame->ecx = val; break;
		    case 2: frame->edx = val; break;
		    case 3: frame->ebx = val; break;
		    case 4: frame->esp = val; break;
		    case 5: frame->ebp = val; break;
		    case 6: frame->esi = val; break;
		    case 7: frame->edi = val; break;
		    case 8: frame->fault_address = val; break;
		    case 9: frame->eflags = val; break;
		    case 10: frame->cs = val; break;
		    case 11: frame->ss = val; break;
		    case 12: frame->ds = val; break;
		    case 13: frame->es = val; break;
		    case 14: 
		    case 15:
		    default: break;
		    }
		    gdb_send_response("OK");
		}
		else gdb_send_response("E00");
	    }
	    case 'q':
		if (buf[1] == 'C')	// get_current_thread
		{
		    tcb_t *current = ptr_to_tcb((ptr_t) frame);
		    gdb_buffer[0] = 'Q'; gdb_buffer[1] = 'C';
		    gdb_buffer[2] = hexchars[(current->myself.id.thread >> 12) & 0xf];
		    gdb_buffer[3] = hexchars[(current->myself.id.thread >> 8)  & 0xf];
		    gdb_buffer[4] = hexchars[(current->myself.id.thread >> 4)  & 0xf];
		    gdb_buffer[5] = hexchars[(current->myself.id.thread >> 0)  & 0xf];
		    gdb_buffer[6] = 0;
		    gdb_send_response();
		    break;
		};
		if (strncmp((char*)&buf[1], "fThreadInfo", 11) == 0)
		{
		    unsigned char * buf = gdb_buffer;
		    *buf++ = 'm';
		    tcb_t * tcb = get_idle_tcb();
		    while(tcb->present_next != get_idle_tcb())
		    {
			*buf++ = hexchars[(tcb->myself.id.thread >> 12) & 0xf];
			*buf++ = hexchars[(tcb->myself.id.thread >> 8) & 0xf];
			*buf++ = hexchars[(tcb->myself.id.thread >> 4) & 0xf];
			*buf++ = hexchars[(tcb->myself.id.thread >> 0) & 0xf];
			*buf++ = ',';
			tcb = tcb->present_next;
		    }
		    *(buf-1) = 0;
		    gdb_send_response();
		    break;
		}
		if (strncmp((char*)&buf[1], "sThreadInfo", 11) == 0)
		{
		    // no more threads...
		    gdb_send_response("l");
		    break;
		}
		if (strncmp((char*)&buf[1], "ThreadExtraInfo,",16) == 0)
		{
		    l4_threadid_t tid = L4_NIL_ID;
		    tid.id.thread = string_to_int(&buf[18]);
		    tcb_t * tcb = tid_to_tcb(tid);
		    printf("ThreadExtraInfo: %p\n", tcb);
		    mem2hex((unsigned char*)"that is kind of interesting...", gdb_buffer, 30);
		    gdb_send_response();
		    break;
		}
		gdb_send_response("");
		break;

	    case 'k':
		//kdebug_hwreset();
		break;

	    case 's': /* single step */
		kdebug_single_stepping(frame, 1);
		exit_kdb = 1;
		break;

	    case 'Z':
		switch(buf[1]) {
		case 0:		// software breakpoint
		    
		case 1:		// hardware breakpoint
		    gdb_send_response("`'");
		    break;
		case 2:		// write watchpoint
		case 3:		// read watchpoint
		case 4:		// access watchpoint
		default:
		    gdb_send_response("`'");
		    break;
		} break;
	    default:
		printf("unknown gdb command (%s)\n", buf);
		gdb_send_response("");
		break;
	    }
	}
    }
    kdebug_arch_exit(frame);
}


#endif /* CONFIG_DEBUGGER_GDB */
