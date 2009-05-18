/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm_dis.c
 * Description:   Disassembler wrapper for ARM.
 *                
 * @LICENSE@
 *                
 * $Id: arm_dis.c,v 1.1 2001/12/13 03:04:02 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include "kdebug.h"

#if defined(CONFIG_DEBUG_DISAS)
#include "../contrib/arm_dis.c"
#endif
