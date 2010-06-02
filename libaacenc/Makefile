####################################################################
#
# Makefile for aac encoder - library
#
####################################################################
################## user section: insert objlist here #######
LIB = libaacenc.a

SRCS=$(wildcard *.c)

OBJS = $(SRCS:.c=.o)

$(LIB): $(OBJS)
	ar r $@ $(OBJS)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
