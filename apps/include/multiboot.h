/*********************************************************************
 *                
 * Copyright (C) 2000,  University of Karlsruhe
 *                
 * Filename:      multiboot.h
 * Description:   Multiboot heeader definitions.
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
 * $Log: multiboot.h,v $
 * Revision 1.1  2000/09/21 11:02:27  skoglund
 * Moved multiboot definitions away from the l4 specific parts of the include
 * tree.
 *
 * Revision 1.1  2000/09/20 13:39:25  skoglund
 * Structures used for accessing the multiboot headers.
 *
 *                
 ********************************************************************/
#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__


typedef struct multiboot_header_t	multiboot_header_t;
typedef struct multiboot_info_t		multiboot_info_t;
typedef struct multiboot_module_t	multiboot_module_t;
typedef struct multiboot_mmap_t		multiboot_mmap_t;



struct multiboot_header_t
{
    unsigned long	magic;
    unsigned long	flags;
    unsigned long	checksum; /* magic + flags + checksum == 0 */

    /*
     * The following fields are only present if MULTIBOOT_EXTENDED_HDR
     * flag is set.
     */
    unsigned long	header_addr;
    unsigned long	load_addr;
    unsigned long	load_end_addr;
    unsigned long	bss_end_addr;
    unsigned long	entry;
};

/* Indicates start of the multiboot header. */
#define MULTIBOOT_MAGIC		0x1badb002


/* Flag value indicating that all boot modules are 4KB page aligned. */
#define MULTIBOOT_PAGE_ALIGN	0x00000001

/* Flag indicating that mem_* fields of multiboot_info_t must be present. */
#define MULTIBOOT_MEMORY_INFO	0x00000002

/* Flag value indicating an extended multiboot header. */
#define MULTIBOOT_EXTENDED_HDR	0x00010000


/* This value in EAX indicates multiboot compliant startup. */
#define MULTIBOOT_VALID         0x2badb002





struct multiboot_info_t
{
    unsigned long	flags;

    /* Amount of available memory (0 -> mem_lower * 1KB, and 
       1MB -> mem_upper * 1KB).  [Bit 0] */
    unsigned long	mem_lower;
    unsigned long	mem_upper;

    /* The BIOS disk device that program was loaded from.  [Bit 1] */
    struct {
	unsigned char	drive;
	unsigned char	part1;
	unsigned char	part2;
	unsigned char	part3;
    } boot_device;

    /* Null terminated commandline passed to the program.  [Bit 2] */
    char		*cmdline;

    /* List of boot modules.  [Bit 3] */
    unsigned long	mods_count;
    multiboot_module_t	*mods_addr;

    /* Symbol information.  [Bit 4 or 5] */
    union
    {
	/* Location and size of a.out symbol table.  [Bit 4] */
	struct
	{
	    unsigned long	tabsize;
	    unsigned long	strsize;
	    unsigned long	addr;
	    unsigned long	reserved;
	} aout;

	/* Location specification of elf section headers.  [Bit 5] */
	struct
	{
	    unsigned long	num;
	    unsigned long	size;
	    unsigned long	addr;
	    unsigned long	shndx;
	} elf;

    } syms;

    /* Location and size of memory map provided by BIOS.  [Bit 6] */
    unsigned long	mmap_length;
    multiboot_mmap_t	*mmap_addr;
};

#define MULTIBOOT_MEM		(1L << 0)
#define MULTIBOOT_BOOT_DEVICE	(1L << 1)
#define MULTIBOOT_CMDLINE	(1L << 2)
#define MULTIBOOT_MODS		(1L << 3)
#define MULTIBOOT_AOUT_SYMS	(1L << 4)
#define MULTIBOOT_ELF_SHDR	(1L << 5)
#define MULTIBOOT_MMAP		(1L << 6)



struct multiboot_module_t
{
    /* Physical start and end addresses of the module. */
    unsigned long	mod_start;
    unsigned long	mod_end;

    /* ASCII string associated with module (e.g., command line). */
    char		*string;

    /* Must be set to `0'. */
    unsigned long	reserved;
};




struct multiboot_mmap_t
{
    unsigned long	size;
    unsigned long	BaseAddrLow;
    unsigned long	BaseAddrHigh;
    unsigned long	LengthLow;
    unsigned long	LengthHigh;
    unsigned long	Type;

    /* Optional padding as indicated by `size'. */ 
};

/* Type value indicating available RAM. */
#define MULTIBOOT_MMAP_RAM	1


#endif /* !__MULTIBOOT_H__ */
