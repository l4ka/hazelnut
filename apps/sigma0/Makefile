######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     sigma0/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.22 2001/12/13 03:27:47 ud3 Exp $
##                
######################################################################


include ../Makeconf .depend

SRCS    = crt0-$(ARCH).S sigma0.c
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES        += ../include
LDFLAGS         += -N -L../lib
DEFINES         += USE_L4_TYPES
CFLAGS          += -x c++

TARGET	= $(notdir $(shell "pwd"))

ifeq ($(SIGMA0_LINKBASE), )
$(error no sigma0 link address configured - make xconfig)
endif

LINK_ADDR=$(SIGMA0_LINKBASE)

all:	$(TARGET).stripped

$(TARGET):	$(OBJS) Makefile $(TOPDIR)/apps/lib/libio.a
	$(LD) $(LDFLAGS) -e_start -Ttext=$(LINK_ADDR) -o $@ $(OBJS) -lio -lgcc
	@echo ""; echo "Done with $@."

$(OBJS) .depend: kip.h

clean::
	rm -f $(TARGET) $(TARGET).stripped

sigma0.c: sigma0-new.c

kip.h:	Makefile $(TOPDIR)/kernel/include/kernel.h
	@echo Building include file for kernel_info_page_t
	@( echo "#ifndef __KIP_H__"; echo "#define __KIP_H__"; echo ""; echo "#include <l4/l4.h>"; echo ""; cat $(TOPDIR)/kernel/include/kernel.h | awk 'BEGIN{printme=0} /typedef struct kernel_info_page_t {/ {printme = 1 } { if (printme == 1 ) { print $0 } } /} kernel_info_page_t;/ { printme = 0}' ; echo "#endif" ) > $@

