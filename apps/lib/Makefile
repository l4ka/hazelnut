######################################################################
##                
## Copyright (C) 2001,  Karlsruhe University
##                
## File path:     lib/Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.4 2001/11/30 14:24:22 ud3 Exp $
##                
######################################################################
include ../Makeconf

SUBDIRS	=	l4 io

all:	$(SUBDIRS)

$(SUBDIRS)::
	@cd $@ && $(MAKE)

clean:
	@for d in $(SUBDIRS); do \
	  (cd $${d} && $(MAKE) clean) \
	done
