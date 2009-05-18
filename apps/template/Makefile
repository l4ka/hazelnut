######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     template/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.4 2001/11/30 14:20:06 ud3 Exp $
##                
######################################################################
include ../Makeconf $(wildcard .depend)

SRCS    = crt0-$(ARCH).S $(wildcard *.c)
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES        += ../include
LDFLAGS         += -N -L../lib
DEFINES         += USE_L4_TYPES
CFLAGS          += -x c++

TARGET	= $(notdir $(shell "pwd"))

$(error edit the Makefile - adjust the next line and then delete this line)
LINKBASE=0x0c200000

all:	$(TARGET)

$(TARGET):	$(OBJS) Makefile ../lib/libio.a
	$(LD) $(LDFLAGS) -e_start -Ttext=$(LINKBASE) -o $@ $(OBJS) -lio -lgcc
	@cp -f $@ $@.stripped
	$(STRIP) $@.stripped
	@echo ""; echo "Done with $@."; echo ""

clean::
	rm -f $(TARGET) $(TARGET).stripped

