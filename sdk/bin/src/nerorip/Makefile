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

all: nerorip

nerorip: main.o nrg.o util.o
	CC -Wall -Wextra -o nerorip main.o nrg.o util.o

main.o: main.c
	CC -Wall -Wextra -c -o main.o main.c

nrg.o: nrg.c
	CC -Wall -Wextra -c -o nrg.o nrg.c

util.o: util.c
	CC -Wall -Wextra -c -o util.o util.c

clean:
	rm -f *.o nerorip

install : nerorip
	$(INSTALL) -m 755 nerorip $(DESTDIR)/nerorip