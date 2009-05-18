include ../Makeconf

SRCS    = crt0-$(ARCH).S topp.c
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES        += ../include
LDFLAGS         += -N -L../lib
DEFINES         += USE_L4_TYPES
CFLAGS          += -x c++ --save-temps

TARGET	= $(notdir $(shell "pwd"))

DEBUG = 1
TOPP_LINKBASE=00600000

ifeq ($(TOPP_LINKBASE), )
$(error no link address configured - make xconfig)
endif

LINK_ADDR=$(TOPP_LINKBASE)

all:	$(TARGET).stripped

$(TARGET):	$(OBJS) Makefile $(TOPDIR)/apps/lib/libio.a
	$(LD) $(LDFLAGS) -e_start -Ttext=$(LINK_ADDR) -o $@ $(OBJS) -lio -lgcc -lide
	@echo ""; echo "Done with $@."

$(OBJS) .depend: kip.h


clean::
	rm -f $(TARGET) $(TARGET).stripped
	rm -f kip.h tcb.h


kip.h:	Makefile $(TOPDIR)/kernel/include/kernel.h
	@echo Building include file for kernel_info_page_t
	@( echo "#ifndef __KIP_H__"; echo "#define __KIP_H__"; echo ""; echo "#include <l4/l4.h>"; echo ""; cat $(TOPDIR)/kernel/include/kernel.h | awk 'BEGIN{printme=0} /typedef struct kernel_info_page_t {/ {printme = 1 } { if (printme == 1 ) { print $0 } } /} kernel_info_page_t;/ { printme = 0}' ; echo "#endif" ) > $@

#tcb.h:	Makefile $(TOPDIR)/kernel/include/thread.h
#	@echo Building include file for tcb_t
#	@( echo "#ifndef __TCB_H__"; echo "#define __TCB_H__"; echo ""; echo "#include <l4/l4.h>"; echo ""; cat $(TOPDIR)/kernel/include/thread.h | awk 'BEGIN{printme=0} /*cut here*/ {printme = 1 } { if (printme == 1 ) { print $0 } } /*cut there*/ { printme = 0}' ; echo "#endif" ) > $@

copy: 	all
	cp $(TARGET) /tftpboot/
