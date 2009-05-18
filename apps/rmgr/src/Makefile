# $Id: Makefile,v 1.5 2001/12/04 18:49:28 ud3 Exp $
#
# Makefile for rmgr

include ../../Makeconf

OSKIT_LIB = ../lib
SIZE	  = size

#SRCS    = crt0-$(ARCH).S sigma0.c
SRCS	= crt0.S startup.c rmgr.c oskit_support.c exec.c globals.c init.c memmap.c \
		  trampoline.c irq.c cfg-parse.c cfg-mem.c pe.c
LIBS = -L$(OSKIT_LIB) -lkern -lexec -lmc -llmm
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES        = ../include ../include/flux/c
LDFLAGS         += -L../lib
DEFINES         += USE_L4_TYPES
CFLAGS		+= -D__VERSION_X__

TARGET	= rmgr

LINK_ADDR=$(RMGR_LINK_ADDR)

all:	$(TARGET)

$(TARGET):	$(OBJS) Makefile 
	$(LD) $(LDFLAGS) -e_start -T rmgr.ld -o $@ $(OBJS) $(LIBS)
	@cp -f $@ $@.stripped
	$(STRIP) $@.stripped
	@chmod a+r $@ $@.stripped
	@echo ""; echo "Done with $@."; echo ""

trampoline.o: trampoline.c
	$(COMP_MESSAGE)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
# assert we don't need data
	@data_size="`$(SIZE) --format=sysv $@ | grep ^.data | awk '{print $$2}'`"; \
	  bss_size="`$(SIZE) --format=sysv $@ | grep ^.bss | awk '{print $$2}'`"; \
	  if [ $$bss_size -gt 0 -o $$data_size -gt 0 ]; then \
	    echo "ERROR: $@: data or bss size > 0"; \
	    rm -f $@; \
	    exit 1; \
	  else \
	    true; \
	  fi

.cfg-parse.d cfg-parse.o: cfg-scan.c

cfg-parse.y: cfg-scan.c

..PRECIOUS: cfg-parse.c

# didn't think we need this rule, but somehow make want's
# to delete the c-file first, even if it's not there
%.c:	%.l
	flex -t $< > $@

clean::
	rm -f $(TARGET) $(TARGET).stripped cfg-scan.c cfg-parse.c

