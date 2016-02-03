#####################################
#                                   
#  CDIRip
#                                   
#####################################
 
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

# Output filename
OUTPUT="cdirip"

# Source files
SRCS=cdirip.c buffer.c cdi.c common.c audio.c
# Output object files (*.o)
OBJECTS = ${SRCS:.c=.o}
# Headers (*.h)
HEADERS= ${SRCS:.c=.h}

RM = rm -f
MV = mv -f
CP = cp -f

.c.o:
	${CC} -c ${CFLAGS} $<
	
cdirip: ${OBJECTS}
	${CC} -o $@ ${OBJECTS} -lm
	
all: cdirip

clean:
	-${RM} ${OUTPUT} ${OBJECTS}

install : cdirip
	$(INSTALL) -m 755 cdirip $(DESTDIR)/cdirip