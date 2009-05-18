######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.32 2001/12/07 19:04:23 skoglund Exp $
##                
######################################################################

# main rule
LIB=

include Makeconf

SUBDIRS	= src src/$(ARCH) src/$(ARCH)/$(PLATFORM) kdb
LIBDIRS = -Lsrc -Lsrc/$(ARCH) -Lsrc/$(ARCH)/$(PLATFORM) -Lkdb \
	  -L$(LIBGCC)

LIBS	= --start-group -lkernel -larch -lplatform -lkdb --end-group


KERNEL	= $(ARCH)-kernel
OBJS	= src/version.o

SYMLINKS_ARCH		= include
SYMLINKS_PLATFORM	= include/arch 

all:	$(KERNEL)

$(KERNEL):	include/config.h include/tcb_layout.h $(SUBDIRS) $(OBJS)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(LIBDIRS) src/$(ARCH)/$(PLATFORM)/startup.o $(OBJS) $(LIBS)
	@$(STRIP) -N gcc2_compiled. $@
	cp $@ $@.stripped
	$(STRIP) $@.stripped
	@chmod a-x+r $@ $@.stripped
	@echo Done. ; echo ""

include/config.h:	$(wildcard xconfig/*.in)
	$(MAKE) -C xconfig

.PHONY: src/version.o
src/version.o:
	@touch src/version.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) '-D__USER__="$(USER)@'`hostname`'"' '-D__GCCVERSION__="$(shell $(CC) -v 2>&1 | tail -1)"' -o $@ -c src/version.c

.PHONY: xconfig xconfig/
xconfig xconfig/:
	$(MAKE) -C xconfig

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

# include some ... ehmmm ... rules for
# - include/tcb_layout.h
include Makefile.voodoo


dep:	include/tcb_layout.h
	@for d in $(SUBDIRS); do \
	  $(MAKE) -C $${d} .depend || exit 1; \
	done

celan clean::
	@for d in $(SUBDIRS) xconfig; do \
	  (cd $${d} && $(MAKE) clean) \
	done
	find include -name *~ | xargs rm -f
	@$(RM) -fv *-kernel{,.stripped,.lst,.map}
	@[ `find . -name "*.[oa]" | wc -l` != 0 ] &&\
	  (echo ; echo Did not delete the following files - review;\
	   find . -name "*.[oa]"; echo) || true
	@echo done.

mrproper: clean
	@echo Removing dependency files
	@$(RM) -fv `find . -name .depend`
	@echo Removing all object files and libraries "*.[oa]"
	@$(RM) -fv `find . -name "*.[oa]"`
	@echo Removing some machine-generated files
	@$(RM) -fv include/tcb_layout.h

lst $(KERNEL).lst:	$(KERNEL)
	@echo -n "Generating Listing $<.lst ... "
	@chmod u+w -f $<.lst || true
	@$(OBJDUMP).l4 -d $< > $<.lst || $(OBJDUMP) -d $< > $<.lst
	@chmod a-w $<.lst
	@echo done.

map $(KERNEL).map:	$(KERNEL)
	@echo -n "Generating Map File $<.map ... "
	@chmod u+w -f $<.map || true
	@$(NM) -n $< > $<.map
	@chmod a-w $<.map
	@echo done.
