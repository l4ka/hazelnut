/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/ide/libide.c
 * Description:   Interface functions for the IDE driver.  Note that
 *                the current version only support writes of up to
 *                32KB, and disk sizes of up to 8GB.
 *                
 * @LICENSE@
 *                
 * $Id: libide.c,v 1.2 2001/11/30 14:24:22 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4/libide.h>

#define PAGE_BITS	(12)
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(~(PAGE_SIZE-1))

static l4_threadid_t idedrv_id;


/*
 * Function ide_init ()
 *
 *    Must be invoked prior to ide_read() or ide_write().
 *
 */
int ide_init(void)
{
    // TODO: this should not be hardcoded.
    idedrv_id.raw = 0x04050001;

    return 0;
}


/*
 * Function ide_read (drive, sec, buf, length)
 *
 *    Invokes an IPC to the IDE driver.  The buffer is mapped to the
 *    driver, and the driver unmaps the buffer prior to sending the
 *    reply IPC.
 *
 */
int ide_read(dword_t drive, dword_t sec, void *buf, dword_t length)
{
    l4_msgdope_t result;
    dword_t dw0, dw1, dw2;
    l4_fpage_t fp;
    l4_idearg_t arg;
    int r;

    length = (length + L4_IDE_SECTOR_SIZE-1) & ~(L4_IDE_SECTOR_SIZE-1);

    arg.args.pos = sec;
    arg.args.length = length / L4_IDE_SECTOR_SIZE;
    arg.args.drive = drive;
    arg.args.write = 0;

    /* Create fpage that contains the buffer. */
    dw0 = (dword_t) buf & PAGE_MASK;
    dw1 = ((dword_t) buf + length + PAGE_SIZE-1) & PAGE_MASK;
    for ( r = PAGE_BITS-1, dw2 = (dw1-dw0) >> PAGE_BITS;
	  dw2 > 0;
	  dw2 >>= 1, r++ ) {}
    fp = l4_fpage(dw0, r, 1, 0);

    r = l4_ipc_call(idedrv_id,
		    (void *) 2, /* Map fpage */
		    (dword_t) buf, (dword_t) fp.fpage, (dword_t) arg.raw,
		    (void *) 0,
		    &dw0, &dw1, &dw2,
		    L4_IPC_NEVER, &result);

    return r;
}


/*
 * Function ide_write (drive, sec, buf, length)
 *
 *    Invokes an IPC to the IDE driver.  The buffer is mapped read-
 *    only to the driver, and the driver unmaps the buffer prior to
 *    sending the reply IPC.
 *
 */
int ide_write(dword_t drive, dword_t sec, void *buf, dword_t length)
{
    l4_msgdope_t result;
    dword_t dw0, dw1, dw2;
    l4_fpage_t fp;
    l4_idearg_t arg;
    int r;

    length = (length + L4_IDE_SECTOR_SIZE-1) & ~(L4_IDE_SECTOR_SIZE-1);

    arg.args.pos = sec;
    arg.args.length = length / L4_IDE_SECTOR_SIZE;
    arg.args.drive = drive;
    arg.args.write = 1;

    /* Create fpage that contains the buffer. */
    dw0 = (dword_t) buf & PAGE_MASK;
    dw1 = ((dword_t) buf + length + PAGE_SIZE-1) & PAGE_MASK;
    for ( r = PAGE_BITS-1, dw2 = (dw1-dw0) >> PAGE_BITS;
	  dw2 > 0;
	  dw2 >>= 1, r++ ) {}
    fp = l4_fpage(dw0, r, 0, 0);

    r = l4_ipc_call(idedrv_id,
		    (void *) 2, /* Map fpage */
		    (dword_t) buf, (dword_t) fp.fpage, (dword_t) arg.raw,
		    (void *) 0,
		    &dw0, &dw1, &dw2,
		    L4_IPC_NEVER, &result);

    return r;
}
