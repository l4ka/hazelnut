/* 
 * Copyright (c) 1996 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
/*
 * The "direct console" is an extremely simple console "driver" for PCs,
 * which writes directly to CGA/EGA/VGA display memory to output text,
 * and reads directly from the keyboard using polling to read characters.
 * The putchar and getchar routines are completely independent;
 * either can be used without the other.
 */
#ifndef _FLUX_X86_PC_DIRECT_CONS_H_
#define _FLUX_X86_PC_DIRECT_CONS_H_

#include <flux/c/common.h>

__FLUX_BEGIN_DECLS
void direct_cons_putchar(unsigned char c);
int direct_cons_getchar(void);
__FLUX_END_DECLS

#endif /* _FLUX_X86_PC_DIRECT_CONS_H_ */
