
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

all : gdiopt
ciso : gdiopt.o
	$(LD) -o gdiopt gdiopt.o

ciso.o : gdiopt.c
	$(CC) -o gdiopt.o -c gdiopt.c

install : gdiopt
	$(INSTALL) -m 755 gdiopt $(DESTDIR)/gdiopt
	
run : gdiopt
	@./gdiopt test.gdi disk.gdi

clean:
	rm -rf *.o
	rm -rf gdiopt
