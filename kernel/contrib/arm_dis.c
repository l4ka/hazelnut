/* Instruction printing code for the ARM
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
   Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe@pegasus.esprit.ec.org)
   Modification by James G. Smith (jsmith@cygnus.co.uk)

This file is part of libopcodes. 

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version. 

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details. 

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */




/*
 * - L4-ified by UD, Dec 2001
 * - brute-force C++-ified by UD, Dec 2001
 */


#include "arm-opc.h"

#define _(x) x
#define VOLATILE __volatile__
#define PTR char *
#define PARAMS(x) x
#define ATTRIBUTE_UNUSED
#define bfd_mach_i386_i386 0
#define bfd_mach_i386_i8086 1
#define bfd_mach_i386_i386_intel_syntax 2
#define abort() return 4
#define stderr NULL
#define boolean int
int fprintf(char* f, const char* format, ...) L4_SECT_KDEBUG;

typedef struct { dword_t eip; dword_t esp; dword_t ebp; int val; } jmp_buf;


typedef dword_t bfd_vma;
typedef byte_t bfd_byte;

typedef int (*fprintf_ftype) PARAMS((PTR, const char*, ...));

struct disassemble_info {
    char* stream;
    fprintf_ftype fprintf_func;
    void (*print_address_func)(bfd_vma addr, struct disassemble_info *info);
};




static char * arm_conditional[] =
{"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
 "hi", "ls", "ge", "lt", "gt", "le", "", "nv"};

const char * arm_regnames[16] = 
{ "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "sl",  "fp",  "ip",  "sp",  "lr",  "pc" };

static char * arm_fp_const[] =
{"0.0", "1.0", "2.0", "3.0", "4.0", "5.0", "0.5", "10.0"};

static char * arm_shift[] = 
{"lsl", "lsr", "asr", "ror"};


static void
arm_decode_shift (long given, fprintf_ftype func, char * stream)
{
  func (stream, "%s", arm_regnames[given & 0xf]);
  
  if ((given & 0xff0) != 0)
    {
      if ((given & 0x10) == 0)
	{
	  int amount = (given & 0xf80) >> 7;
	  int shift = (given & 0x60) >> 5;
	  
	  if (amount == 0)
	    {
	      if (shift == 3)
		{
		  func (stream, ", rrx");
		  return;
		}
	      
	      amount = 32;
	    }
	  
	  func (stream, ", %s #%d", arm_shift[shift], amount);
	}
      else
	func (stream, ", %s %s", arm_shift[(given & 0x60) >> 5],
	      arm_regnames[(given & 0xf00) >> 8]);
    }
}

/* Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction (always 4 on ARM). */
static int
print_insn_arm (bfd_vma pc, struct disassemble_info * info, long given)
{
  struct arm_opcode *  insn;
  char *               stream = info->stream;
  fprintf_ftype        func   = info->fprintf_func;

  for (insn = arm_opcodes; insn->assembler; insn++)
    {
      if ((given & insn->mask) == insn->value)
	{
	  char * c;
	  
	  for (c = insn->assembler; *c; c++)
	    {
	      if (*c == '%')
		{
		  switch (*++c)
		    {
		    case '%':
		      func (stream, "%%");
		      break;

		    case 'a':
		      if (((given & 0x000f0000) == 0x000f0000)
			  && ((given & 0x02000000) == 0))
			{
			  int offset = given & 0xfff;
			  
			  func (stream, "[pc");
 
			  if (given & 0x01000000)
			    {
			      if ((given & 0x00800000) == 0)
				offset = - offset;
			  
			      /* pre-indexed */
			      func (stream, ", #%d]", offset);

			      offset += pc + 8;

			      /* Cope with the possibility of write-back
				 being used.  Probably a very dangerous thing
				 for the programmer to do, but who are we to
				 argue ?  */
			      if (given & 0x00200000)
				func (stream, "!");
			    }
			  else
			    {
			      /* Post indexed.  */
			      func (stream, "], #%d", offset);

			      offset = pc + 8;  /* ie ignore the offset.  */
			    }
			  
			  func (stream, "\t; ");
			  info->print_address_func (offset, info);
			}
		      else
			{
			  func (stream, "[%s", 
				arm_regnames[(given >> 16) & 0xf]);
			  if ((given & 0x01000000) != 0)
			    {
			      if ((given & 0x02000000) == 0)
				{
				  int offset = given & 0xfff;
				  if (offset)
				    func (stream, ", %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				}
			      else
				{
				  func (stream, ", %s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""));
				  arm_decode_shift (given, func, stream);
				}

			      func (stream, "]%s", 
				    ((given & 0x00200000) != 0) ? "!" : "");
			    }
			  else
			    {
			      if ((given & 0x02000000) == 0)
				{
				  int offset = given & 0xfff;
				  if (offset)
				    func (stream, "], %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				  else 
				    func (stream, "]");
				}
			      else
				{
				  func (stream, "], %s",
					(((given & 0x00800000) == 0) 
					 ? "-" : ""));
				  arm_decode_shift (given, func, stream);
				}
			    }
			}
		      break;

		    case 's':
                      if ((given & 0x004f0000) == 0x004f0000)
			{
                          /* PC relative with immediate offset.  */
			  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
			  
			  if ((given & 0x00800000) == 0)
			    offset = -offset;
			  
			  func (stream, "[pc, #%d]\t; ", offset);
			  
			  (*info->print_address_func)
			    (offset + pc + 8, info);
			}
		      else
			{
			  func (stream, "[%s", 
				arm_regnames[(given >> 16) & 0xf]);
			  if ((given & 0x01000000) != 0)
			    {
                              /* Pre-indexed.  */
			      if ((given & 0x00400000) == 0x00400000)
				{
                                  /* Immediate.  */
                                  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
				  if (offset)
				    func (stream, ", %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				}
			      else
				{
                                  /* Register.  */
				  func (stream, ", %s%s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""),
                                        arm_regnames[given & 0xf]);
				}

			      func (stream, "]%s", 
				    ((given & 0x00200000) != 0) ? "!" : "");
			    }
			  else
			    {
                              /* Post-indexed.  */
			      if ((given & 0x00400000) == 0x00400000)
				{
                                  /* Immediate.  */
                                  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
				  if (offset)
				    func (stream, "], %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				  else 
				    func (stream, "]");
				}
			      else
				{
                                  /* Register.  */
				  func (stream, "], %s%s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""),
                                        arm_regnames[given & 0xf]);
				}
			    }
			}
		      break;
			  
		    case 'b':
		      (*info->print_address_func)
			(BDISP (given) * 4 + pc + 8, info);
		      break;

		    case 'c':
		      func (stream, "%s",
			    arm_conditional [(given >> 28) & 0xf]);
		      break;

		    case 'm':
		      {
			int started = 0;
			int reg;

			func (stream, "{");
			for (reg = 0; reg < 16; reg++)
			  if ((given & (1 << reg)) != 0)
			    {
			      if (started)
				func (stream, ", ");
			      started = 1;
			      func (stream, "%s", arm_regnames[reg]);
			    }
			func (stream, "}");
		      }
		      break;

		    case 'o':
		      if ((given & 0x02000000) != 0)
			{
			  int rotate = (given & 0xf00) >> 7;
			  int immed = (given & 0xff);
			  immed = (((immed << (32 - rotate))
				    | (immed >> rotate)) & 0xffffffff);
			  func (stream, "#%d\t; 0x%x", immed, immed);
			}
		      else
			arm_decode_shift (given, func, stream);
		      break;

		    case 'p':
		      if ((given & 0x0000f000) == 0x0000f000)
			func (stream, "p");
		      break;

		    case 't':
		      if ((given & 0x01200000) == 0x00200000)
			func (stream, "t");
		      break;

		    case 'h':
		      if ((given & 0x00000020) == 0x00000020)
			func (stream, "h");
                      else
                        func (stream, "b");
		      break;

		    case 'A':
		      func (stream, "[%s", arm_regnames [(given >> 16) & 0xf]);
		      if ((given & 0x01000000) != 0)
			{
			  int offset = given & 0xff;
			  if (offset)
			    func (stream, ", %s#%d]%s",
				  ((given & 0x00800000) == 0 ? "-" : ""),
				  offset * 4,
				  ((given & 0x00200000) != 0 ? "!" : ""));
			  else
			    func (stream, "]");
			}
		      else
			{
			  int offset = given & 0xff;
			  if (offset)
			    func (stream, "], %s#%d",
				  ((given & 0x00800000) == 0 ? "-" : ""),
				  offset * 4);
			  else
			    func (stream, "]");
			}
		      break;

		    case 'B':
		      /* Print ARM V5 BLX(1) address: pc+25 bits.  */
		      {
			bfd_vma address;
			bfd_vma offset = 0;
			
			if (given & 0x00800000)
			  /* Is signed, hi bits should be ones.  */
			  offset = (-1) ^ 0x00ffffff;

			/* Offset is (SignExtend(offset field)<<2).  */
			offset += given & 0x00ffffff;
			offset <<= 2;
			address = offset + pc + 8;
			
			if (given & 0x01000000)
			  /* H bit allows addressing to 2-byte boundaries.  */
			  address += 2;

		        info->print_address_func (address, info);
		      }
		      break;

		    case 'I':
		      /* Print a Cirrus/DSP shift immediate.  */
		      /* Immediates are 7bit signed ints with bits 0..3 in
			 bits 0..3 of opcode and bits 4..6 in bits 5..7
			 of opcode.  */
		      {
			int imm;

			imm = (given & 0xf) | ((given & 0xe0) >> 1);

			/* Is ``imm'' a negative number?  */
			if (imm & 0x40)
			  imm |= (-1 << 7);

			func (stream, "%d", imm);
		      }

		      break;

		    case 'C':
		      func (stream, "_");
		      if (given & 0x80000)
			func (stream, "f");
		      if (given & 0x40000)
			func (stream, "s");
		      if (given & 0x20000)
			func (stream, "x");
		      if (given & 0x10000)
			func (stream, "c");
		      break;

		    case 'F':
		      switch (given & 0x00408000)
			{
			case 0:
			  func (stream, "4");
			  break;
			case 0x8000:
			  func (stream, "1");
			  break;
			case 0x00400000:
			  func (stream, "2");
			  break;
			default:
			  func (stream, "3");
			}
		      break;
			
		    case 'P':
		      switch (given & 0x00080080)
			{
			case 0:
			  func (stream, "s");
			  break;
			case 0x80:
			  func (stream, "d");
			  break;
			case 0x00080000:
			  func (stream, "e");
			  break;
			default:
			  func (stream, _("<illegal precision>"));
			  break;
			}
		      break;
		    case 'Q':
		      switch (given & 0x00408000)
			{
			case 0:
			  func (stream, "s");
			  break;
			case 0x8000:
			  func (stream, "d");
			  break;
			case 0x00400000:
			  func (stream, "e");
			  break;
			default:
			  func (stream, "p");
			  break;
			}
		      break;
		    case 'R':
		      switch (given & 0x60)
			{
			case 0:
			  break;
			case 0x20:
			  func (stream, "p");
			  break;
			case 0x40:
			  func (stream, "m");
			  break;
			default:
			  func (stream, "z");
			  break;
			}
		      break;

		    case '0': case '1': case '2': case '3': case '4': 
		    case '5': case '6': case '7': case '8': case '9':
		      {
			int bitstart = *c++ - '0';
			int bitend = 0;
			while (*c >= '0' && *c <= '9')
			  bitstart = (bitstart * 10) + *c++ - '0';

			switch (*c)
			  {
			  case '-':
			    c++;
			    
			    while (*c >= '0' && *c <= '9')
			      bitend = (bitend * 10) + *c++ - '0';
			    
			    if (!bitend)
			      abort ();
			    
			    switch (*c)
			      {
			      case 'r':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%s", arm_regnames[reg]);
				}
				break;
			      case 'd':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%d", reg);
				}
				break;
			      case 'x':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "0x%08x", reg);
				  
				  /* Some SWI instructions have special
				     meanings.  */
				  if ((given & 0x0fffffff) == 0x0FF00000)
				    func (stream, "\t; IMB");
				  else if ((given & 0x0fffffff) == 0x0FF00001)
				    func (stream, "\t; IMBRange");
				}
				break;
			      case 'X':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%01x", reg & 0xf);
				}
				break;
			      case 'f':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  if (reg > 7)
				    func (stream, "#%s",
					  arm_fp_const[reg & 7]);
				  else
				    func (stream, "f%d", reg);
				}
				break;
			      default:
				abort ();
			      }
			    break;
			  
			  case '`':
			    c++;
			    if ((given & (1 << bitstart)) == 0)
			      func (stream, "%c", *c);
			    break;
			  case '\'':
			    c++;
			    if ((given & (1 << bitstart)) != 0)
			      func (stream, "%c", *c);
			    break;
			  case '?':
			    ++c;
			    if ((given & (1 << bitstart)) != 0)
			      func (stream, "%c", *c++);
			    else
			      func (stream, "%c", *++c);
			    break;
			  default:
			    abort ();
			  }
			break;
		      
		      }
		    }
		}
	      else
		func (stream, "%c", *c);
	    }
	  return 4;
	}
    }
  abort ();
}










void paf(bfd_vma addr, struct disassemble_info *info) L4_SECT_KDEBUG;
void paf(bfd_vma addr, struct disassemble_info *info)
{
    printf("%x", addr);
};


int do_printf(const char** format_p);
int fprintf(char* f, const char* format, ...)
{
    return do_printf(&format);
};

int arm_disas(dword_t pc) L4_SECT_KDEBUG;
int arm_disas(dword_t pc)
{
    disassemble_info info =
    {
	NULL,
	fprintf,
	paf
    };
    dword_t insn;
    space_t *space = (space_t *) get_current_pgtable();
    if (!kdebug_get_mem(space, pc, &insn))
    {
	printf("##");
	return 4;
    }
    return print_insn_arm(pc, &info, insn);
}

