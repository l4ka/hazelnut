/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/io/get_hex.c
 * Description:   generic get_hex() based on putc() and getc()
 *                
 * @LICENSE@
 *                
 * $Id: get_hex.c,v 1.7 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/

#include <l4/l4.h>
#include <l4io.h>

word_t
get_hex(void)
{
  word_t val = 0;
  char c, t;
  int i = 0;

  while ((t = c = getc()) != '\r')
  {
      switch(c)
      {
      case '0' ... '9':
	  c -= '0';
	  break;
	  
      case 'a' ... 'f':
	  c -= 'a' - 'A';
      case 'A' ... 'F':
	  c = c - 'A' + 10;
	  break;
      default:
	  continue;
      };
      val <<= 4;
      val += c;
      
      /* let the user see what happened */
      putc(t);

      if (++i == 8)
	  break;
  };
  return val;
}
