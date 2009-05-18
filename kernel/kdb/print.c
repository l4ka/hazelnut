/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     print.c
 * Description:   Various output functions for the kernel debugger.
 *                
 * @LICENSE@
 *                
 * $Id: print.c,v 1.34 2001/12/04 16:56:50 uhlig Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_ARCH(cpu.h)

static int print_hex(dword_t val) L4_SECT_KDEBUG;
static int print_hex(word_t val) L4_SECT_KDEBUG;
static int print_hex(byte_t val) L4_SECT_KDEBUG;
static int print_string(char* s) L4_SECT_KDEBUG;
static int print_dec(dword_t val, dword_t width = 0) L4_SECT_KDEBUG;
int do_printf(const char** format_p) L4_SECT_KDEBUG;

static const byte_t hexchars[] L4_SECT_KDEBUG = "0123456789abcdef";

#if defined(CONFIG_DEBUGGER_NEW_KDB)
extern struct {
    dword_t tid;
    char name[9];
} name_tab[32];           
#endif

static int print_hex(dword_t val)
{
    int i, n = 1;
    for ( i = 28; i >= 0; i -= 4, n++ )
	putc(hexchars[(val >> i) & 0xF]);
    return n;
}

static int print_hex(word_t val)
{
    int i, n = 1;
    for ( i = 12; i >= 0; i -= 4, n++ )
	putc(hexchars[(val >> i) & 0xF]);
    return n;
}

static int print_hex(byte_t val)
{
    int i, n = 1;
    for ( i = 4; i >= 0; i -= 4, n++ )
	putc(hexchars[(val >> i) & 0xF]);
    return n;
}

static int print_string(char* s)
{
    int n = 0;
    while ( *s ) { putc(*s++); n++; }
    return n;
}


static int print_dec(dword_t val, dword_t width)
{
    char buff[16];
    int i = 14;

    buff[15] = '\0';
    do
    {
	buff[i] = (val % 10) + '0';
	val = val / 10;
	i--;
	if ( width > 0 ) width--;
    } while( val );

    while ( i >= 0 && width-- > 0 )
	buff[i--] = ' ';
  
    return print_string(&buff[i+1]);
}


DEFINE_SPINLOCK(printf_spin_lock);

int do_printf(const char** format_p)
{
    const char* format = *format_p;
    int n = 0;
    int i = 0;
    int width = 8;
    
#define arg(x) (((dword_t*) format_p)[(x)+1])

    spin_lock(&printf_spin_lock);

    /* sanity check */
    if (format == NULL)
      return 0;

    while (*format)
      {
	switch (*(format))
	  {
	  case '%':
	      width = 0;
	  reentry:
	    switch (*(++format))
	      {
	      case '0'...'9':
		  width = width*10 + (*format)-'0';
		  goto reentry;
		  break;
	      case 'c':
		putc(arg(i));
		n++;
		break;
	      case 'd':
		n += print_dec(arg(i), width);
		break;
	      case 'p':
	      case 'x':
		  switch(width) {
		  case 2: n += print_hex((byte_t) arg(i)); break;
		  case 4: n += print_hex((word_t) arg(i)); break;
		  default: n += print_hex((dword_t) arg(i)); break;
		  }
		break;
	      case 's':
		  if ((char*) arg(i))
		      n += print_string((char*) arg(i));
		  else
		      n += print_string("(null)");
		  break;
	      case 'T':
		  if (l4_is_invalid_id((l4_threadid_t) {raw : arg(i)}))
		  {
		      n += print_string("INVALID "); break;
		  }
		  if (l4_is_nil_id((l4_threadid_t) {raw : arg(i)}))
		  {
		      n += print_string("NIL_ID  "); break;
		  }
#if defined(CONFIG_DEBUGGER_NEW_KDB)
		  //print nickname if possible
		  for (i=0; i<32; i++) 
		      if (name_tab[i].tid == (dword_t) arg(i)) {
			  print_string(name_tab[i].name);
			  break;
		      }
		  //else print hex value
#endif
		  if (arg(i) < 17)
		  {
		      n += print_string("IRQ_");
		      if (arg(i) < 11) { putc(' '); n++; }
		      n += print_dec(arg(i)-1); if (arg(i) < 11) putc(' ');
		      n += print_string("  ");
		  }
		  print_hex(arg(i));
		  break;
	      case 't':
		  if (l4_is_invalid_id((l4_threadid_t) {raw : arg(i)}))
		  {
		      n += print_string("INVALID"); break;
		  }
		  if (l4_is_nil_id((l4_threadid_t) {raw : arg(i)}))
		  {
		      n += print_string("NIL_ID"); break;
		  }
#if defined(CONFIG_DEBUGGER_NEW_KDB)
		  //print nickname if possible
		  for (i=0; i<32; i++) 
		      if (name_tab[i].tid == (dword_t) arg(i)) {
			  print_string(name_tab[i].name);
			  break;
		      }
		  //else print hex value
#endif
		  if (arg(i) < 17)
		  {
		      n += print_string("IRQ_");
		      n += print_dec(arg(i)-1);
		  }
		  else
		  {
		      print_hex((byte_t) (((l4_threadid_t) {
			  raw : arg(i)}).x0id.task));
		      putc('.');
		      print_hex((byte_t) (((l4_threadid_t) {
			  raw : arg(i)}).x0id.thread));
		      n += 5;
		  }
		  break;
	      case '%':
		  putc('%');
		  n++;
		  format++;
		  continue;
	      default:
		n += print_string("?");
		break;
	      };
	    i++;
	    break;
	  default:
	    putc(*format);
	    n++;
	    if(*format == '\n')
	      putc('\r');
	    break;
	  }
	format++;
      }
    spin_unlock(&printf_spin_lock);
    return n;
}

int printf(const char* format,...)
{
    return do_printf(&format);
};
