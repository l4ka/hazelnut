######################################################################
##                
## Copyright (C) 1999, 200, 2001,  Karlsruhe University
##                
## File path:     x86/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.20 2002/05/13 13:04:30 stoess Exp $
##                
######################################################################

######################################################################
# the source files...

LIB	= libarch.a
OBJ_SRC = 
LIB_SRC = init.c exception.S memory.c resources.c interrupt.c \
	  io_mapping.c smp.c smpipc.c smp-boot.S 

######################################################################
# includes
include ../../Makeconf $(wildcard .depend)

######################################################################
# rules

all: $(OBJ) $(LIB)
