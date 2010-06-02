####################################################################
#
# Makefile for bit buffer management library
# $Id: Makefile,v 1.2.4.1 2004/02/03 17:40:04 ehr Exp $
#
####################################################################
################## user section: insert objlist here #######
LIB=libresamp.a

SRCS=$(wildcard *.c)

OBJS = $(SRCS:.c=.o)

$(LIB): $(OBJS)
	ar r $@ $(OBJS)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
