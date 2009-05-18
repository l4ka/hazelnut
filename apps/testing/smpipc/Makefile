include ../../Makeconf

SRCS    = crt0-$(ARCH).S $(wildcard *.c)
OBJS    = $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES        += ../../include
LDFLAGS         += -N -L../../lib
DEFINES         += USE_L4_TYPES
CFLAGS          += -x c++

TARGET	= $(notdir $(shell "pwd"))

LINKBASE=0x00220000

all:	$(TARGET).stripped

$(TARGET):	$(OBJS) Makefile ../../lib/libio.a
	$(LD) $(LDFLAGS) -e_start -Ttext=$(LINKBASE) -o $@ $(OBJS) -lio -lgcc

clean::
	rm -f $(TARGET) $(TARGET).stripped

