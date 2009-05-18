/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     lib.h
 * Description:   Various in-kernel helper functions.
 *                
 * @LICENSE@
 *                
 * $Id: lib.h,v 1.4 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __LIB_H__
#define __LIB_H__

void zero_memory(dword_t* dest, int len);
void memcpy(dword_t* dest, dword_t * src, int len);


#endif /* __LIB_H__ */
