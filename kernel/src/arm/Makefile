######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     arm/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.10 2001/12/07 19:09:18 skoglund Exp $
##                
######################################################################

######################################################################
# the source files...

LIB	= libarch.a
OBJ_SRC = 
LIB_SRC = farcalls.c init.c memory.c exception.S kmemory.c mapping.c \
	  resources.c


######################################################################
# includes
include ../../Makeconf $(wildcard .depend)



######################################################################
# rules

all: $(OBJ) $(LIB)
