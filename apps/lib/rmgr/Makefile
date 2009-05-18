include ../../Makeconf $(wildcard .depend)

LIB	= ../librmgr.a

SRCS	= librmgr.c
OBJS	= $(patsubst %.S, %.o, $(patsubst %.c, %.o, $(SRCS)))

INCLUDES = ../../rmgr/include 

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) cvrs $@ $^

