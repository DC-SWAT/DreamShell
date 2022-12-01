
CFLAGS = -O2
INSTALL = install
DESTDIR = ../..

all : mkbios
ciso : mkbios.o
	$(LD) -o mkbios mkbios.o

ciso.o : mkbios.c
	$(CC) -o mkbios.o -c mkbios.c

install : mkbios
	$(INSTALL) -m 755 mkbios $(DESTDIR)/mkbios

clean:
	rm -rf *.o
	rm -rf mkbios
