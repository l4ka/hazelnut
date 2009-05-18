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

#ifndef _FLUX_DISKPART_DEC_H_
#define _FLUX_DISKPART_DEC_H_

/*
 * Definition of the DEC disk label,
 * which is located (you guessed it)
 * at the end of the 4.3 superblock.
 */

struct dec_partition_info {
        unsigned int    n_sectors;      /* how big the partition is */
        unsigned int    offset;         /* sector no. of start of part. */
};

typedef struct {
        int     magic;
#       define  DEC_LABEL_MAGIC         0x032957
        int     in_use;
        struct  dec_partition_info partitions[8];
} dec_label_t;

/*
 * Physical location on disk.
 * This is independent of the filesystem we use,
 * although of course we'll be in trouble if we
 * screwup the 4.3 SBLOCK..
 */

#define DEC_LABEL_BYTE_OFFSET   ((2*8192)-sizeof(dec_label_t))


/*
 * Definitions for the primary boot information
 * This is common, cuz the prom knows it.
 */

typedef struct {
        int             pad[2];
        unsigned int    magic;
#       define          DEC_BOOT0_MAGIC 0x2757a
        int             mode;
        unsigned int    phys_base;
        unsigned int    virt_base;
        unsigned int    n_sectors;
        unsigned int    start_sector;
} dec_boot0_t;

typedef struct {
        dec_boot0_t     vax_boot;
                                        /* BSD label still fits in pad */
        char                    pad[0x1e0-sizeof(dec_boot0_t)];
        unsigned long           block_count;
        unsigned long           starting_lbn;
        unsigned long           flags;
        unsigned long           checksum; /* add cmpl-2 all but here */
} alpha_boot0_t;

#endif /* _FLUX_DISKPART_DEC_H_ */
