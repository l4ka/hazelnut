######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     lib/l4/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.4 2001/11/30 14:24:22 ud3 Exp $
##                
######################################################################
include ../../Makeconf

LIB	= ../libl4.a

SRCS	= libl4.c
OBJS	= $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES += ../../include

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) cvrs $@ $^

clean:
	@rm -f *~ *.i *.ii *.s $(OBJS) $(LIB)

.depend:	$(SRCS)
	$(CC) -M $(CPPFLAGS) $(SRCS) > .depend

include .depend
