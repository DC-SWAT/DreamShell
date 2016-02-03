
# Makefile for the wav2adpcm program.

ifeq ($(LBITS),64)
INCS = -I/usr/include/x86_64-linux-gnu -I/usr/include/$(gcc -print-multiarch)
LIBS = -L/usr/lib32
CC = gcc -m32 $(INCS)
LD = gcc -m32 $(LIBS)
else
CC = gcc
LD = gcc
endif

CFLAGS = -O2 -Wall
INSTALL = install
DESTDIR = ../..

all: wav2adpcm

install : wav2adpcm
	$(INSTALL) -m 755 wav2adpcm $(DESTDIR)/wav2adpcm
	
run : wav2adpcm
	@./wav2adpcm pcm.wav adpcm.wav

clean:
	rm -rf *.o
	rm -rf wav2adpcm
