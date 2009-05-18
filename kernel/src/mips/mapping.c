/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/mapping.c
 * Description:   Mapping specification for MIPS.
 *                
 * @LICENSE@
 *                
 * $Id: mapping.c,v 1.2 2001/11/22 13:27:53 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>

dword_t mdb_pgshifts[2] = {
    12,		// 4KB
    32		// 4GB (whole address space)
};

mdb_buflist_t mdb_buflists[4] = {
    { 12 },	// Mapping nodes
    { 8 },	// ptrnode_t
    { 4096 },	// 4KB array
    { 0 }
};

