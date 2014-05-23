# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Dan Potter

OBJS = mdct.o smallft.o block.o envelope.o window.o lsp.o lpc.o analysis.o synthesis.o psy.o info.o floor1.o floor0.o res0.o mapping0.o registry.o codebook.o sharedbook.o lookup.o bitrate.o vorbisfile.o
KOS_CFLAGS += -I. -I../include -DLITTLE_ENDIAN=1

all: libvorbis.a
libvorbis.a: $(OBJS) 
	$(KOS_AR) rcs libvorbis.a $(OBJS)
	cp $(OBJS) ../build
	cp libvorbis.a ../build

clean:
	-rm -f $(OBJS) libvorbis.a
	-rm -f ../build/*.o ../build/*.a

include $(KOS_BASE)/Makefile.rules

