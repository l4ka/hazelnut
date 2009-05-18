######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.18 2001/11/22 13:40:29 skoglund Exp $
##                
######################################################################

######################################################################
# the source files...

OBJ_SRC = 
LIB_SRC = mapping.c mapping_alloc.c init.c kmemory.c thread.c schedule.c ipc.c \
	  memory.c global.c lib.c
LIB 	= libkernel.a

######################################################################
# includes
include ../Makeconf $(wildcard .depend)

######################################################################
# rules

all: $(OBJ) $(LIB)
