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
 * Definitions specific to block devices.
 */
#ifndef _FLUX_FDEV_BLK_H_
#define _FLUX_FDEV_BLK_H_

struct fdev_blk
{
	fdev_t		fdev;

	unsigned char	type;
	unsigned char	flags;
	unsigned	blksize;

	fdev_error_t (*read)(fdev_t *dev, void *buf, fdev_buf_vect_t *bufvec,
			     fdev_off_t offset, unsigned count,
			     unsigned *out_actual);
	fdev_error_t (*write)(fdev_t *dev, void *buf, fdev_buf_vect_t *bufvec,
			      fdev_off_t offset, unsigned count,
			      unsigned *out_actual);
};
typedef struct fdev_blk fdev_blk_t;

#define fdev_blk_read(dev, cmd, buf, bufvec)	\
	((dev)->read((dev), (cmd), (buf), (bufvec)))

#define fdev_blk_write(dev, cmd, buf, bufvec)	\
	((dev)->write((dev), (cmd), (buf), (bufvec)))

/* Specific block device types */
#define FDEV_BLK_DISK		0x01
#define FDEV_BLK_FLOPPY		0x02
#define FDEV_BLK_CDROM		0x03

/* Flags specific to block devices */
#define FDEV_BLKF_REMOVABLE	0x01	/* Removable media */
#define FDEV_BLKF_REMOVE_NOTIFY	0x02	/* Removal detection supported */

#endif /* _FLUX_FDEV_BLK_H_ */
