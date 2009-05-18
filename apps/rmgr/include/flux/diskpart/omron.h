/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * Definition of the Omron disk label,
 * which is located at sector 0. It
 * _is_ sector 0, actually.
 */

#ifndef _FLUX_DISKLABEL_OMRON_H_
#define _FLUX_DISKLABEL_OMRON_H_

struct omron_partition_info {
        unsigned long   offset;
        unsigned long   n_sectors;
};

typedef struct {
        char            packname[128];  /* in ascii */

        char            pad[512-(128+8*8+11*2+4)];

        unsigned short  badchk; /* checksum of bad track */
        unsigned long   maxblk; /* # of total logical blocks */
        unsigned short  dtype;  /* disk drive type */
        unsigned short  ndisk;  /* # of disk drives */
        unsigned short  ncyl;   /* # of data cylinders */
        unsigned short  acyl;   /* # of alternate cylinders */
        unsigned short  nhead;  /* # of heads in this partition */
        unsigned short  nsect;  /* # of 512 byte sectors per track */
        unsigned short  bhead;  /* identifies proper label locations */
        unsigned short  ppart;  /* physical partition # */
        struct omron_partition_info
                        partitions[8];

        unsigned short  magic;  /* identifies this label format */
#       define  OMRON_LABEL_MAGIC       0xdabe

        unsigned short  cksum;  /* xor checksum of sector */

} omron_label_t;

/*
 * Physical location on disk.
 */

#define OMRON_LABEL_BYTE_OFFSET 0

#endif /* _FLUX_DISKLABEL_OMRON_H_ */
