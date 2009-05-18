/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/mapping.c
 * Description:   Mapping specifications for the ARM.
 *                
 * @LICENSE@
 *                
 * $Id: mapping.c,v 1.3 2001/11/22 13:18:48 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>

dword_t mdb_pgshifts[5] = {
    10,		// 1KB
    12,		// 4KB
    16,		// 64KB
    20,		// 1MB
    32		// 4GB (whole address space)
};

mdb_buflist_t mdb_buflists[6] = {
    { 12 },	// Mapping nodes
    { 8 },	// ptrnode_t
    { 64 },	// 64KB array, 4KB array
    { 16 },	// 1KB array
    { 16384 },	// 1MB array
    { 0 }
};

