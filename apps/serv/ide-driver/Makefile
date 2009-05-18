include ../../Makeconf $(wildcard .depend)

SRCS    = crt0-$(ARCH).S $(wildcard *.c)
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES	+= ../../include
LDFLAGS		+= -N -L../../lib
DEFINES		+= USE_L4_TYPES
CFLAGS		+= -x c++
LIBDEPS		 = ../../lib/libio.a \
		   ../../lib/librmgr.a \
		   ../../lib/libl4malloc.a
LIBS		+= -lio -lrmgr -ll4malloc -lgcc

TARGET	= $(notdir $(shell "pwd"))

LINKBASE=0x00400000

all:	$(TARGET)

$(TARGET):	$(OBJS) Makefile $(LIBDEPS)
	$(LD) $(LDFLAGS) -e_start -Ttext=$(LINKBASE) -o $@ $(OBJS) $(LIBS)
	@cp -f $@ $@.stripped
	$(STRIP) $@.stripped
	@echo ""; echo "Done with $@."; echo ""

clean::
	rm -f $(TARGET) $(TARGET).stripped

