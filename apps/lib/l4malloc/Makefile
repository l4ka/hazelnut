######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     lib/l4malloc/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.2 2001/11/30 14:24:23 ud3 Exp $
##                
######################################################################
include ../../Makeconf $(wildcard .depend)

LIB	= ../libl4malloc.a

SRCS	= libl4malloc.c
OBJS	= $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) cvrs $@ $^

