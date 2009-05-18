/*********************************************************************
 *                
 * Copyright (C) 2000,  University of Karlsruhe
 *                
 * Filename:      hd.h
 * Description:   
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
 * $Log: hd.h,v $
 * Revision 1.1  2000/09/21 15:59:00  skoglund
 * A first version of an ide driver.  The implementation of a recoverable
 * disk is not yet finished.
 *
 *                
 ********************************************************************/
#ifndef __HD_H__
#define __HD_H__


#define	MAX_DRIVES		1	/* must be power of 2 */
#define	MAX_SECTORS_PER_REQUEST	8
#define	SECTOR_SIZE		512

/* Location where fpages are received */
#define RCV_LOCATION		0x00800000
#define RCV_SIZE		16		/* 64KB */



/*
 * Structure used to hold drive parameters.
 */
typedef struct drive_parameter_table_t drvparm_t;
struct drive_parameter_table_t {
    word_t cylinders;
    byte_t heads;
    word_t start_cylinder;
    word_t pre_comp_cylinder;
    byte_t ecc_burst_length;
    byte_t control_byte;
    byte_t normal_timeout;
    byte_t format_timeout;
    byte_t checkdisk_timeout;
    word_t landingzone_cylinder; 
    byte_t sectors;
    byte_t unused;
} __attribute__ ((packed));


/*
 * Structure used to hold partition table entries.
 */
typedef struct partition_entry_t partition_t;
struct partition_entry_t {
    byte_t boot;	/* Boot flag: 0 = not active, 0x80 active */
    byte_t bhead;	/* Begin: Head number */
    word_t bseccyl;	/* Begin: Sector and cylinder numb of boot sector */
    byte_t sys;		/* System code: 0x83 linux, 0x82 Linux swap etc. */
    byte_t ehead;	/* End: Head number */
    word_t eseccyl;	/* End: Sector and cylinder number of last sector */
    dword_t start;	/* Relative sector number of start sector */
    dword_t numsec;	/* Number of sectors in the partition */
} __attribute__ ((packed));



/*
 * Bitarrays are used for keeping track of valid diskblocks.
 */
class bitarray_t
{
 public:

    int		size;
    byte_t	*bytes;

    inline int chk(int num)
	{
	    return bytes[num >> 8] & (1 << (num & 7));
	}

    inline void set(int num)
	{
	    bytes[num >> 8] |= (1 << (num & 7));
	}

    inline void clr(int num)
	{
	    bytes[num >> 8] &= ~(1 << (num & 7));
	}

    inline void reset(void)
	{
	    long *ptr = (long *) bytes;
	    int i = (((size+7) >> 3)+3) >> 2;

	    while ( i-- )
		*ptr++ = 0;
	}
};


struct hd_vector {
    int		sectors_per_drive;
    partition_t	partition_table[4];
    bitarray_t	current_block;
    bitarray_t	currently_written;
};

extern struct hd_vector hd[MAX_DRIVES];





/*
 * Prototypes.
 */

/* From main.c */
void read_ptab(int drive_no);
void print_ptab(int drive_no, int num);
void init_driver(void);

/* From ide.c */
int open_controller(void);
int open_drive(int drive_no);
int read_block(int drive_no, int sector_no, int sectors, int *buffer_address);
int write_block(int drive_no, int sector_no, int sectors, int *buffer_address);

/* From req.c */
void server(void);




/*
 * Helper functions.
 */

static inline byte_t inb(dword_t port)
{
    byte_t tmp;

    if (port < 0x100) /* GCC can optimize this if constant */
	__asm__ __volatile__ ("inb %w1, %0	\n"
			      :"=al"(tmp)
			      :"dN"(port));
    else
	__asm__ __volatile__ ("inb %%dx, %0	\n"
			      :"=al"(tmp)
			      :"d"(port));

    return tmp;
}


static inline byte_t inw(dword_t port)
{
    byte_t tmp;

    if (port < 0x100) /* GCC can optimize this if constant */
	__asm__ __volatile__ ("in %w1, %0	\n"
			      :"=ax"(tmp)
			      :"dN"(port));
    else
	__asm__ __volatile__ ("in %%dx, %0	\n"
			      :"=ax"(tmp)
			      :"d"(port));

    return tmp;
}


static inline void outb(dword_t port, byte_t val)
{
    if (port < 0x100) /* GCC can optimize this if constant */
	__asm__ __volatile__ ("outb %1, %w0	\n"
			      :
			      :"dN"(port), "al"(val));
    else
	__asm__ __volatile__ ("outb %1, %%dx	\n"
			      :
			      :"d"(port), "al"(val));
}


static int insw(int port, void *data, int num) 
{
    int ret;

    __asm__ __volatile__ (
	"pushw %%es		# store es		\n"
	"pushw %%ds		# copy ds to es		\n"
	"popw  %%es		#			\n"
	"cld			# Clear direction flag	\n"
	"rep			# Repeat prefix		\n"
	"insw			#			\n"
	"popw  %%es		# restore es		\n"
	:"=a"(ret)
	:"d"(port), "D"(data), "c"(num)); 

    return ret;
}


static int outsw(int port, void *data, int num) 
{
    int ret;

    __asm__ __volatile__ (
	"cld			# Clear direction flag	\n"
	"rep			# Repeat prefix		\n"
	"outsw			#			\n"
	:"=a"(ret)
	:"d"(port), "S"(data), "c"(num));

    return ret;
}


static inline void cli()
{
    __asm__ __volatile__ ("cli");
}


static inline void sti()
{
    __asm__ __volatile__ ("sti");
}



#define panic(format, args...)					\
do {								\
    printf("PANIC: %s(): %s, line %d\n",			\
	   __FUNCTION__, __FUNCTION__, __LINE__);		\
    printf("PANIC: " format , ## args);				\
    for (;;)							\
	enter_kdebug("panic");					\
} while(0)


#endif /* __HD_H__ */
