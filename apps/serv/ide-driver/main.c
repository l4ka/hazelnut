/*********************************************************************
 *                
 * Copyright (C) 2000,  University of Karlsruhe
 *                
 * Filename:      main.c
 * Description:   IDE driver which also implements a "relocatable
 *                disk"
 *                
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *                
 * $Log: main.c,v $
 * Revision 1.1  2000/09/21 15:59:00  skoglund
 * A first version of an ide driver.  The implementation of a recoverable
 * disk is not yet finished.
 *
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4io.h>
#include <multiboot.h>
#include <l4/rmgr/librmgr.h>
#include <l4/malloc.h>

#include "hd.h"


int main(multiboot_info_t *mbi, dword_t mbvalid)
{
    if ( mbvalid == MULTIBOOT_VALID )
    {
	if ( mbi->flags & MULTIBOOT_CMDLINE )
	    printf("Started %s [%p]\n", mbi->cmdline, l4_myself());
    }

    if ( ! rmgr_init() )
	panic("idedrv: rmgr_init() failed\n");

    init_driver();
    server();

    /* NOT REACED */
}


/*
 * Read partition table.
 */
void read_ptab(int drive_no)
{
    char ptable_buf[512];
    partition_t	*ptable = (partition_t *) (ptable_buf + 0x1be);
    int i;

    read_block(drive_no, 0, 1, (int *) ptable_buf);

    for ( i = 0; i < 4; i++ )
	hd[drive_no].partition_table[i] = ptable[i];
}


/*
 * Print partition info.
 */
void print_ptab(int drive_no, int num)
{
    partition_t *p = &hd[drive_no].partition_table[num];

    printf("Drive %d, Partition %d  (size %dKB)\n", drive_no, num,
	   (p->numsec * SECTOR_SIZE)/1024);
    printf("  boot         = 0x%02x\n"
	   "  begin_head   = %d\n"
	   "  begin_seccyl = %d\n"
	   "  system       = 0x%02x\n"
	   "  end_head     = %d\n"
	   "  end_seccyl   = %d\n"
	   "  start_sec    = %d\n"
	   "  num_sectors  = %d\n",
	   p->boot, p->bhead, p->bseccyl,
	   p->sys, p->ehead, p->eseccyl,
	   p->start, p->numsec);
}


/*
 * Initialization of relocatable disk driver.
 */
void init_driver(void)
{
    dword_t size;

    if ( open_controller() == -1 )
	panic("idedrv: open_controller() failed\n");

    for ( int i = 0; i < MAX_DRIVES; i++ )
    {
	 open_drive(i);
	 read_ptab(i);

	 // Find size of drive.
	 for ( int j = size = 0; j < 4; j++ )
	 {
	     partition_entry_t *p = hd[i].partition_table + j;
	     if ( p->numsec && (p->start + p->numsec) > size )
		 size = p->start + p->numsec;
	 }

	 // Initialize bitarrays.
	 hd[i].current_block.size = size;
	 hd[i].current_block.bytes = (byte_t *) malloc((size+7) >> 3);
	 hd[i].current_block.reset();

	 size /= 2;
	 hd[i].currently_written.size = size;
	 hd[i].currently_written.bytes = (byte_t *) malloc((size+7) >> 3);
	 hd[i].currently_written.reset();
    }
}
