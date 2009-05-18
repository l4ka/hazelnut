######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     Makefile
## Description:   Application directory Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.7 2001/12/09 03:25:12 ud3 Exp $
##                
######################################################################
# main rule
include Makeconf $(wildcard xconfig/.config)

SYMLINKS_ARCH		= include/l4
SYMLINKS_PLATFORM	= # include/arch 

SUBDIRS	+= lib/io

ifeq ($(CONFIG_BUILD_SIGMA0),y)
  SUBDIRS	+= sigma0
endif
ifeq ($(CONFIG_BUILD_ROOTTASK),y)
  SUBDIRS	+= root_task
endif
ifeq ($(CONFIG_BUILD_RMGR),y)
  SUBDIRS	+= rmgr/src
endif

all:	symlinks subdirs

subdirs:
	@for d in $(SUBDIRS); do \
	  $(MAKE) -C $${d} || exit 1; \
	done

symlinks::
	@for f in $(SYMLINKS_ARCH); do \
	  if test ! -e $${f}/arch -o -L $${f}/arch; then \
	    $(RM) -f $${f}/arch; \
	    $(LN_S) $(ARCH) $${f}/arch; \
	  else \
	    echo $${f}/arch is not a symbolic link!; \
	    exit 1; \
	  fi; \
	done
	rm -f include/l4/sys
	ln -sf arch/sys include/l4/sys
	ln -sf x86-x0-32 rmgr/include/l4/sys

.PHONY:	xconfig xconfig/
xconfig xconfig/:
	$(MAKE) -C xconfig

celan clean::
	@for d in $(SUBDIRS); do \
	  $(MAKE) -C $${d} clean; \
	done
	@rm -fv *-kernel{,.stripped}
	@echo purging symlinks
	@$(RM) -fv `find . -type l`

