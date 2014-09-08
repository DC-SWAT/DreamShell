
# Makefile for the raw2wav program.

ifeq ($(LBITS),64)
INCS = -I/usr/include/x86_64-linux-gnu -I/usr/include/$(gcc -print-multiarch)
LIBS = -L/usr/lib32
CC = gcc -m32 $(INCS)
LD = gcc -m32 $(LIBS)
else
CC = gcc
LD = gcc
endif

CFLAGS = -O2
INSTALL = install
DESTDIR = ../..

all: raw2wav

install : raw2wav
	$(INSTALL) -m 755 raw2wav $(DESTDIR)/raw2wav
	
run : raw2wav
	@./raw2wav test.raw test.wav

clean:
	rm -rf *.o
	rm -rf raw2wav
