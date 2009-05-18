######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     lib/ide/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.2 2001/11/30 14:24:22 ud3 Exp $
##                
######################################################################
include ../../Makeconf $(wildcard .depend)

LIB	= ../libide.a

SRCS	= libide.c
OBJS	= $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) cvrs $@ $^

