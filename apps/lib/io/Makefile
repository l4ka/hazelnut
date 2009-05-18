######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     lib/io/Makefile
## Description:   
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.10 2001/11/30 14:24:22 ud3 Exp $
##                
######################################################################
include ../../Makeconf $(wildcard .depend)

LIBS	= ../libio.a ../libionative.a

SRCS	= $(ARCH)-$(PLATFORM)-putc.c \
	  $(ARCH)-$(PLATFORM)-getc.c \
	  get_hex.c print.c
OBJS	= $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES += ../../include
CPPFLAGS += -DUSE_L4_TYPES 

ifneq ($(MODE),)
all:	$(MAKECMDGOALS)
  ifeq ($(MODE), ionative)
    CPPFLAGS += -DNATIVE
  endif

$(MAKECMDGOALS):	$(OBJS)
	$(AR) cvrs $@ $^

else

all:	$(LIBS)

../lib%.a:	$(SRCS)
	$(MAKE) "MODE=$*" $@
endif


.INTERMEDIATE: $(OBJS)


clean::
	@rm -f *~ *.i *.ii *.s $(OBJS) $(LIBS)
