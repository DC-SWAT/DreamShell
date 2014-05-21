
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
INCS = -I/usr/include/x86_64-linux-gnu -I/usr/include/$(gcc -print-multiarch)
LIBS = -L/usr/lib32
CC = gcc -m32 $(INCS)
LD = gcc -m32 $(LIBS)
else
CC = gcc
LD = gcc
endif

INSTALL = install
DESTDIR = ../..

all : ciso
ciso : ciso.o
	$(LD) -o ciso ciso.o -lz -llzo2

ciso.o : ciso.c
	$(CC) -o ciso.o -c ciso.c

install : ciso
	$(INSTALL) -m 755 ciso $(DESTDIR)/ciso

clean:
	rm -rf *.o
	rm -rf ciso
