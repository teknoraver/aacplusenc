include config.mak
TOPDIR=$(CURDIR)
export CFLAGS=-Wall -pedantic -O3 -ftree-vectorize -I$(TOPDIR)/libaacenc -I$(TOPDIR)/libbitbuf -I$(TOPDIR)/libfr -I$(TOPDIR)/libsbrenc -I$(TOPDIR)/libresamp $(EXTRACFLAGS)
SUBDIRS=libaacenc libbitbuf libfr libsbrenc libresamp
LIBS=libaacenc/libaacenc.a libbitbuf/libbitbuf.a libfr/libfr.a libsbrenc/libsbrenc.a libresamp/libresamp.a
TARGET=aacplusenc

LDFLAGS=-L$(TOPDIR)/libaacenc -L$(TOPDIR)/libbitbuf -L$(TOPDIR)/libfr -L$(TOPDIR)/libsbrenc -L$(TOPDIR)/libresamp
LDFLAGS+=-laacenc -lbitbuf -lfr -lsbrenc -lresamp
LDFLAGS+=-lm

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

.PHONY: clean test
clean:
	rm -f $(ofiles) $(TARGET) config.* test-*
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i clean ;\
	done

install: all
	mkdir -p $(INSTDIR)/bin
	cp aacplusenc $(INSTDIR)/bin
	strip -s -R.comment $(INSTDIR)/bin/aacplusenc

MONO:=pan=1:0.5:0.5:channels=1
MPLAYER:=mplayer -msglevel all=0:statusline=5
AO:=-ao pcm:fast:file

test: all
	$(MPLAYER) -af lavcresample=32000:$(MONO) $(AO)=test32m.wav test.wav
	$(MPLAYER) -af lavcresample=44100:$(MONO) $(AO)=test44m.wav test.wav
	$(MPLAYER) -af lavcresample=48000:$(MONO) $(AO)=test48m.wav test.wav

	$(MPLAYER) -af lavcresample=32000 $(AO)=test32s.wav test.wav
	$(MPLAYER) -af lavcresample=44100 $(AO)=test44s.wav test.wav
	$(MPLAYER) -af lavcresample=48000 $(AO)=test48s.wav test.wav

	./aacplusenc test32m.wav test32m.aac 18
	./aacplusenc test44m.wav test44m.aac 40
	./aacplusenc test48m.wav test48m.aac 40

	./aacplusenc test32s.wav test32s.aac 18
	./aacplusenc test44s.wav test44s.aac 48
	./aacplusenc test48s.wav test48s.aac 48

	$(MPLAYER) test32m.aac
	$(MPLAYER) test44m.aac
	$(MPLAYER) test48m.aac

	$(MPLAYER) test32s.aac
	$(MPLAYER) test44s.aac
	$(MPLAYER) test48s.aac

config.mak:
	./configure
