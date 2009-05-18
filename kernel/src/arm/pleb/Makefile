######################################################################
##                
## Copyright (C) 2002,  Karlsruhe University
##                
## File path:     arm/pleb/Makefile
## Description:   
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.1 2002/01/24 20:20:06 ud3 Exp $
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
