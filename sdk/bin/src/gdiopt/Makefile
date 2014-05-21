
CC = gcc
LD = gcc
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
