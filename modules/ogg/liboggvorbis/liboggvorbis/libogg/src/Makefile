# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Dan Potter

OBJS = framing.o bitwise.o
KOS_INCS += -I. -I../include -DLITTLE_ENDIAN=1
CFLAGS += -O2

all: libogg.a
libogg.a: $(OBJS)
	$(KOS_AR) rcs libogg.a $(OBJS)
	cp $(OBJS) ../build
	cp libogg.a ../build

clean:
	-rm -f $(OBJS) *.a
	-rm -f ../build/*.o *.a

include $(KOS_BASE)/Makefile.rules

