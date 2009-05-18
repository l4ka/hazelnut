######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.13 2001/11/22 13:41:18 skoglund Exp $
##                
######################################################################

######################################################################
# the source files...

OBJ_SRC = 
LIB_SRC = print.c mini-kd.c gdb.c mdb.c init.c input.c tracepoints.c \
	  new-kdb.c interrupt.c tracing.c \
	  $(wildcard $(ARCH).c $(ARCH)-$(PLATFORM).c) \
	  $(wildcard asm-$(ARCH).S asm-$(ARCH)-$(PLATFORM).S) \
	  $(wildcard $(ARCH)_*.c)
LIB 	= libkdb.a

######################################################################
# includes
include ../Makeconf $(wildcard .depend)

######################################################################
# rules

all: $(OBJ) $(LIB)
