/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-booter/elf.c
 * Description:   tiny ELF file loader
 *                
 * @LICENSE@
 *                
 * $Id: elf.c,v 1.4 2001/12/09 04:06:30 ud3 Exp $
 *                
 ********************************************************************/

#include "elf.h"
#include <l4io.h>

#define BOOTABLE_ARM_ELF(h) \
 ((h.e_ident[EI_MAG0] == ELFMAG0) & (h.e_ident[EI_MAG1] == ELFMAG1) \
  & (h.e_ident[EI_MAG2] == ELFMAG2) & (h.e_ident[EI_MAG3] == ELFMAG3) \
  & (h.e_ident[EI_CLASS] == ELFCLASS32) & (h.e_ident[EI_DATA] == ELFDATA2LSB) \
  & (h.e_ident[EI_VERSION] == 1/*EV_CURRENT*/) & (h.e_type == ET_EXEC) \
  & (h.e_machine == EM_ARM) & (h.e_version == EV_CURRENT))

void loadelf(void* image, unsigned* entry, unsigned* paddr)
{
    Elf32_Ehdr* eh;
    dword_t filepos;
    
    eh = (Elf32_Ehdr*) image;

    if (BOOTABLE_ARM_ELF((*eh)))
    {
	dword_t i;
	dword_t memaddr, memsiz, filesiz;
	Elf32_Phdr *phdr;
	
	*entry = eh->e_entry;
	for (i = 0; i < eh->e_phnum; i++)
	{
	    dword_t j;
	    phdr = (Elf32_Phdr *) (eh->e_phoff + (int) eh +
				   eh->e_phentsize * i);
	    if (phdr->p_type == PT_LOAD)
	    {
		/* offset into file */
		filepos = phdr->p_offset;
		filesiz = phdr->p_filesz;
		memaddr = phdr->p_paddr;
		memsiz  = phdr->p_memsz;
		*paddr = memaddr;
		
		if (!filepos)
		{
		    filepos = (sizeof(Elf32_Ehdr)
			       + sizeof(Elf32_Phdr) * eh->e_phnum);
		    filesiz -= filepos;
		    memaddr += filepos;
		};
		printf("  0x%x bytes at %x\n",
		       filesiz, memaddr);
		for (j = 0; j < filesiz; j++)
		{
		    *((byte_t*) (j+memaddr)) = ((byte_t*) eh)[filepos+j];
		};
	    };
	}
    }
    else
	printf("  is no valid ELF file\n");
};
