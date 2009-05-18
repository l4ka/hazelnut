/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/kdebug.h
 * Description:   MIPS specific kdebug stuff.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug.h,v 1.5 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_KDEBUG_H__
#define __MIPS_KDEBUG_H__

INLINE void spin(int pos = 80) {
    while(1);
}

INLINE void spin1(int pos) {
    putc('.');
}

INLINE void enter_kdebug(const char* message = NULL)
{
    if (message)
	printf(message);
    kdebug_entry(NULL);
}

INLINE ptr_t get_current_pgtable()
{
    return 0;
}

#endif /* __MIPS_KDEBUG_H__ */
