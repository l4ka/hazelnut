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
	@awk -F'[_=]' -f rewrite.awk .config | ed -s ../Makeconf.local

config.tk:	$(wildcard *.in) header.tk tail.tk tkparse 
	cat header.tk > config.tk && \
	./tkparse < config.in >> config.tk && \
	echo "set defaults \"defconfig\"" >> config.tk && \
	cat tail.tk >> config.tk

tkparse: tkparse.o tkcond.o tkgen.o
	${CC} -o tkparse tkparse.o tkcond.o tkgen.o

../Makeconf.local:
	@echo Creating default Makeconf.local
	@echo "PLATFORM=i586" > $@
	@echo "SIGMA0_LINKBASE=" >> $@
	@echo "ROOTTASK_LINKBASE=" >> $@

clean:
	rm -f *.o tkparse config.tk
