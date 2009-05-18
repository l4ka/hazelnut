/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/mini-print.c
 * Description:   tiny version of printf
 *                
 * @LICENSE@
 *                
 * $Id: mini-print.c,v 1.3 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4io.h>
#include <l4/l4.h>

void print_hex(dword_t val);
void print_string(char* s);
void print_dec(dword_t val);

static const byte_t hexchars[] = "0123456789ABCDEF";

void print_hex(dword_t val)
{
  signed int i;	/* becomes negative */
  for (i=28; i >= 0; i -= 4)
    putc(hexchars[(val >> i) & 0xF]);
};

void print_string(char* s)
{
  while (*s)
    putc(*s++);
};


void print_dec(dword_t val)
{
  char buff[16];
  dword_t i = 14;

  buff[15] = '\0';
  do
  {
      buff[i] = (val % 10) + '0';
      val = val / 10;
      i--;
  } while(val);

  
 print_string(&buff[i+1]);

};




int printf(const char* format, ...)
{
    dword_t ret = 1;
    dword_t i = 0;
    
#define arg(x) (((ptr_t) &format)[(x)+1])

    /* sanity check */
    if (format == NULL)
      return 0;

    while (*format)
      {
	switch (*(format))
	  {
	  case '%':
	    switch (*(++format))
	      {
	      case 'c':
		putc(arg(i));
		break;
	      case 'd':
		print_dec(arg(i));
		break;
	      case 'p':
	      case 'x':
		print_hex((dword_t)arg(i));
		break;
	      case 's':
		print_string((char*) arg(i));
		break;
	      default:
		print_string("?");
		break;
	      };
	    i++;
	    break;
	  default:
	    putc(*format);
	    if(*format == '\n')
	      putc('\r');
	    break;
	  };
	format++;
      };
    return ret;
};

