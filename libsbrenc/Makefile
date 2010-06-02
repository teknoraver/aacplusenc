####################################################################
#
# Makefile for SBR encoder - library
#
####################################################################
################## user section: insert objlist here #######
LIB=libsbrenc.a

SRCS=$(wildcard *.c)

OBJS = $(SRCS:.c=.o)

$(LIB): $(OBJS)
	ar r $@ $(OBJS)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
