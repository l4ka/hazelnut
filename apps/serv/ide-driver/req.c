/*********************************************************************
 *                
 * Copyright (C) 2000,  University of Karlsruhe
 *                
 * Filename:      req.c
 * Description:   The server that handles requests to the ide drive.
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
 * $Log: req.c,v $
 * Revision 1.1  2000/09/21 15:59:00  skoglund
 * A first version of an ide driver.  The implementation of a recoverable
 * disk is not yet finished.
 *
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4/libide.h>
#include <l4io.h>

#include "hd.h"


void server(void)
{
    l4_threadid_t client;
    l4_msgdope_t result;
    dword_t dw0, dw1, dw2 = 0, buf;
    l4_idearg_t arg;
    l4_fpage_t fp;
    int r;

    /* Window for receiving buffers. */
    fp = l4_fpage(RCV_LOCATION, RCV_SIZE, 1, 1);

    printf("idedrv: awaiting requests\n");

    for (;;)
    {
	r = l4_ipc_wait(&client, (void *) fp.fpage,
			&dw0, &dw1, &arg.raw,
			L4_IPC_NEVER, &result);

	for (;;)
	{
	    buf = RCV_LOCATION + (dw0 & ((1 << RCV_SIZE)-1));


	    if ( arg.args.write )
	    {
		printf("idedrv: write_block(%d, %d, %d, %p)\n",
		       arg.args.drive, arg.args.pos,
		       arg.args.length, buf);

//		r = write_block(arg.args.drive,
//				arg.args.pos,
//				arg.args.length,
//				(int *) buf);
	    }
	    else
	    {
		printf("idedrv: read_block(%d, %d, %d, %p)\n",
		       arg.args.drive, arg.args.pos,
		       arg.args.length, buf);
	       
		r = read_block(arg.args.drive,
			       arg.args.pos,
			       arg.args.length,
			       (int *) buf);
	    }

	    l4_fpage_unmap(fp, L4_FP_ALL_SPACES|L4_FP_FLUSH_PAGE);

	    r = l4_ipc_reply_and_wait(client, (void *) 0,
				      dw0, dw1, dw2,
				      &client, (void *) fp.fpage, 
				      &dw0, &dw1, &arg.raw,
				      L4_IPC_NEVER, &result);

	    if ( L4_IPC_ERROR(result) )
	    {
		printf("idedrv: error reply_and_wait (%x)\n", result.raw);
		break;
	    }
	}

    }
}
