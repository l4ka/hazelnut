######################################################################
##                
## Copyright (C) 2001-2002,  Karlsruhe University
##                
## File path:     arm-booter/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.15 2002/12/12 08:19:26 ud3 Exp $
##                
######################################################################
include ../Makeconf

KERNEL=		../../kernel/arm-kernel.stripped
MODULES=	../sigma0/sigma0.stripped \
		../root_task/root_task.stripped

BP_MOD_PARTS=$(patsubst %.stripped, %, $(notdir $(MODULES)))
BP_MOD_FILES=$(addprefix tmp/, $(BP_MOD_PARTS))
BP_KNL_PARTS=$(patsubst %.stripped, %, $(notdir $(KERNEL)))
BP_KNL_FILES=$(addprefix tmp/, $(BP_KNL_PARTS))
BP_OBJS=$(addsuffix .o, $(BP_MOD_FILES) $(BP_KNL_FILES))

ifeq ($(PLATFORM), dnard)
	LINKBASE=0x0e500000
endif

ifeq ($(PLATFORM), brutus)
	LINKBASE=0xd8000000
endif

ifeq ($(PLATFORM), pleb)
	LINKBASE=0xc0008000
endif

ifeq ($(PLATFORM), ep7211)
	LINKBASE=0xc0700000
endif

ifeq ($(PLATFORM), ipaq)
	LINKBASE=0xc1000000
endif

# crt0-$(ARCH).S or crt0-$(ARCH)-$(PLATFORM).S must come first
SRCS    = $(wildcard crt0-$(ARCH).S crt0-$(ARCH)-$(PLATFORM).S) main.c elf.c modules.c
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES        += ../include
LDFLAGS         += -N -L../lib -static -Ttext=$(LINKBASE)
DEFINES         += USE_L4_TYPES
CFLAGS          += -x c++ -fno-builtin


TARGET	= arm-booter

all: $(TARGET).bin

$(TARGET).bin:	$(TARGET).stripped
	$(OBJCOPY) -Obinary -S $< $@
	@chmod a+r $@	
	@chmod a-x $@	
	@echo ""; echo "Done with $@"; echo ""

$(TARGET):	$(OBJS) Makefile linker.lds ../lib/libionative.a $(BP_OBJS)
	@echo ""; echo "Linking $@"
	$(LD) -Tlinker.lds $(LDFLAGS) -o $@ $(OBJS) $(BP_OBJS) -lionative -lgcc

main.o: modules.h

$(BP_MOD_FILES) $(BP_KNL_FILES):	$(MODULES) $(KERNEL)
	@for i in $(MODULES) $(KERNEL); do \
	   cp $$i tmp/`basename $$i|sed -e 's/\.stripped//'`; \
	 done

modules.h: $(MODULES) Makefile
	@echo "Building $@"
	@for i in `echo $(BP_MOD_PARTS) $(BP_KNL_PARTS) | tr '-' '_'`; do \
	   echo "extern byte_t _binary_$${i}_start[];" ; \
	   echo "extern byte_t _binary_$${i}_end[];" ; \
	   echo "extern byte_t _binary_$${i}_size[];" ; \
	   echo "extern dword_t $${i}_vaddr;" ; \
	   echo "extern dword_t $${i}_entry;" ; \
	   echo ""; \
	 done > modules.h
	@echo "" >> modules.h
	@echo "void loadmodules();"				>> modules.h
	@echo "void loadelf(void* image, unsigned* entry, unsigned *paddr);" \
		>> modules.h


modules.c: modules.h $(MODULES) Makefile
	@echo "Building $@"
	@echo "/* automatically generated file - do not edit */">  modules.c
	@echo							>> modules.c
	@echo "#include <l4/l4.h>"				>> modules.c
	@echo '#include "modules.h"'				>> modules.c
	@echo							>> modules.c
	@for i in `echo $(BP_MOD_PARTS) $(BP_KNL_PARTS) | tr '-' '_'`; do \
	   echo "unsigned $${i}_vaddr;" ; \
	   echo "unsigned $${i}_entry;" ; \
	 done >> modules.c
	@echo							>> modules.c
	@echo "#include <l4io.h>"				>> modules.c
	@echo							>> modules.c
	@echo 'void loadmodules()'				>> modules.c
	@echo {							>> modules.c
	@for i in `echo $(BP_MOD_PARTS) | tr '-' '_'`; do \
	   echo -n "  printf(\" Loading $${i}\n\");" ; \
	   echo " loadelf(_binary_$${i}_start, &$${i}_entry, &$${i}_vaddr);"; \
	 done >> modules.c
	@echo }							>> modules.c

tmp/%.o:	tmp/% Makefile
	@echo "Preparing $<"
	@echo "SECTIONS { .$(<F) : { *(.data); . = ALIGN(4); }  }" > $<.lnk
	@(cd tmp;$(LD) -T $(<F).lnk -r --oformat default -o $(@F) -bbinary $(<F))
	@rm $<.lnk

clean::
	rm -v -f $(TARGET)* modules.* tmp/[^C]*

