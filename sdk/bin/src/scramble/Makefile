
CC = gcc
LD = gcc
INSTALL = install
DESTDIR = ../..

all : scramble
ciso : scramble.o
	$(LD) -o scramble scramble.o

ciso.o : scramble.c
	$(CC) -o scramble.o -c scramble.c

install : scramble
	$(INSTALL) -m 755 scramble $(DESTDIR)/scramble

clean:
	rm -rf *.o
	rm -rf scramble
