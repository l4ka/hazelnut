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
 * This file contains the definitions for the VTOC label.
 */

#ifndef _FLUX_DISKLABEL_VTOC_H_
#define _FLUX_DISKLABEL_VTOC_H_

#define PDLOCATION      29      /* VTOC sector */

#define VTOC_SANE       0x600DDEEE      /* Indicates a sane VTOC */

#define HDPDLOC         29              /* location of pdinfo/vtoc */


/* Here are the VTOC definitions */
#define V_NUMPAR        16              /* maximum number of partitions */

struct localpartition   {
        u_int   p_flag;                 /*permision flags*/
        long    p_start;                /*physical start sector no of partition*/
        long    p_size;                 /*# of physical sectors in partition*/
};
typedef struct localpartition localpartition_t;


struct evtoc {
        u_int   fill0[6];
        u_int   cyls;                   /*number of cylinders per drive*/
        u_int   tracks;                 /*number tracks per cylinder*/
        u_int   sectors;                /*number sectors per track*/
        u_int   fill1[13];
        u_int   version;                /*layout version*/
        u_int   alt_ptr;                /*byte offset of alternates table*/
        u_short alt_len;                /*byte length of alternates table*/
        u_int   sanity;                 /*to verify vtoc sanity*/
        u_int   xcyls;                  /*number of cylinders per drive*/
        u_int   xtracks;                /*number tracks per cylinder*/
        u_int   xsectors;               /*number sectors per track*/
        u_short nparts;                 /*number of partitions*/
        u_short fill2;                  /*pad for 286 compiler*/
        char    label[40];
        struct localpartition part[V_NUMPAR];/*partition headers*/
        char    fill[512-352];
};



#endif /* _FLUX_DISKLABEL_VTOC_H_ */
