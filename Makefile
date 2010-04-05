include config.mak
TOPDIR=$(CURDIR)
export CFLAGS=-Wall -pedantic -O3 -ftree-vectorize -I$(TOPDIR)/libaacenc -I$(TOPDIR)/libbitbuf -I$(TOPDIR)/libfr -I$(TOPDIR)/libsbrenc -I$(TOPDIR)/libresamp $(EXTRACFLAGS)
SUBDIRS=libaacenc libbitbuf libfr libsbrenc libresamp
LIBS=libaacenc/libaacenc.a libbitbuf/libbitbuf.a libfr/libfr.a libsbrenc/libsbrenc.a libresamp/libresamp.a
TARGET=aacplusenc

LDFLAGS=-lm
LDFLAGS+=-L$(TOPDIR)/libaacenc -L$(TOPDIR)/libbitbuf -L$(TOPDIR)/libfr -L$(TOPDIR)/libsbrenc -L$(TOPDIR)/libresamp
LDFLAGS+=-laacenc -lbitbuf -lfr -lsbrenc -lresamp

ifdef FFTW3
	LDFLAGS+=-lfftw3f
endif


INSTDIR=/usr/local

%.a:
	$(MAKE) -C $$(dirname $*)

all: config.h $(TARGET)

$(TARGET): $(LIBS) au_channel.h adts.h aacplusenc.c
	for i in $(LIBS) ; do \
		$(MAKE) $$i ;\
	done
	$(CC) $(CFLAGS) -o $(TARGET) aacplusenc.c $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(ofiles) $(TARGET) config.* test-*
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i clean ;\
	done

install: all
	mkdir -p $(INSTDIR)/bin
	cp aacplusenc $(INSTDIR)/bin
	strip -s -R.comment $(INSTDIR)/bin/aacplusenc

config.mak:
	./configure
