######################################################################
##                
## Copyright (C) 2002,  Karlsruhe University
##                
## File path:     bench/int-latency/Makefile
## Description:   Application for measuring interrupt latencies
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.1 2002/05/07 19:08:28 skoglund Exp $
##                
######################################################################
include ../../Makeconf

SRCS    = crt0-$(ARCH).S $(wildcard *.c)
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES+=	../../include
LDFLAGS+=	-N -L../../lib
DEFINES+=	USE_L4_TYPES
CFLAGS+=	-x c++ --save-temps -ggdb
SFLAGS+=	--save-temps

TARGET=		$(notdir $(shell "pwd"))

LINKBASE=	$(ROOTTASK_LINKBASE)

all:	$(TARGET).stripped

$(TARGET):	$(OBJS) Makefile ../../lib/libio.a
	$(LD) $(LDFLAGS) -e_start -Ttext=$(LINKBASE) -o $@ $(OBJS) -lio -lgcc

clean::
	rm -f $(TARGET) $(TARGET).stripped

