######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     arm/dnard/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.8 2001/11/22 13:18:49 skoglund Exp $
##                
######################################################################

######################################################################
# the source files...

LIB	= libplatform.a
OBJ_SRC = startup.S
LIB_SRC = init.c interrupt.c

######################################################################
# includes

include ../../../Makeconf $(wildcard .depend)


######################################################################
# rules

all: $(OBJ) $(LIB)
