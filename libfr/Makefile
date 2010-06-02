####################################################################
#
# Makefile for fractional arithmetic library
# $Id: Makefile,v 1.7.2.3 2004/12/22 09:04:53 ehr Exp $
#
####################################################################
################## user section: insert objlist here #######
LIB=libfr.a

SRCS=$(wildcard *.c)

OBJS = $(SRCS:.c=.o)

$(LIB): $(OBJS)
	ar r $(LIB) $(OBJS)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
