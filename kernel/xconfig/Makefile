######################################################################
##                
## Copyright (C) 1999, 2000, 2001,  Karlsruhe University
##                
## File path:     Makefile
##                
## @LICENSE@
##                
## $Id: Makefile,v 1.8 2001/11/22 15:57:34 skoglund Exp $
##                
######################################################################
SHELL=bash

ifeq ($(MACHTYPE), i686-pc-cygwin)
	WISH=cygitkwish30
else
ifneq ($(shell type -p wish8.3),)
	WISH=wish8.3
else
	WISH=wish
endif
endif

-include ../Makeconf.local

doit:	config.tk
	@$(WISH) -f config.tk
	@echo Rewriting Makeconf.local
	@awk -F'[_=]' '/^CONFIG_ARCH_[^_]*=y/ { ARCH=$$3 } /^CONFIG_ARCH_[^_]*_[^_]*=y/ { if ($$3 == ARCH) { printf(",s/^PLATFORM=.*/PLATFORM=%s/\nwq\n", tolower($$4)) }}' .config | ed -s ../Makeconf.local

config.tk:	$(wildcard *.in) header.tk tail.tk tkparse 
	cat header.tk > config.tk && \
	./tkparse < config.in >> config.tk && \
	echo "set defaults \"defconfig\"" >> config.tk && \
	cat tail.tk >> config.tk

tkparse: tkparse.o tkcond.o tkgen.o
	${CC} -o tkparse tkparse.o tkcond.o tkgen.o

../Makeconf.local:
	@echo Creating default Makeconf.local
	@echo "PLATFORM=i686" >> ../Makeconf.local

clean:
	rm -f *.o tkparse config.tk
