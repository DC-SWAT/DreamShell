# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Dan Potter

OBJS = sndoggvorbis.o main.o
SUBDIRS =
POSTDIRS = test

BUILD_TARGET = build 
KOS_CFLAGS += -I. -I../liboggvorbis/libvorbis/include -I../liboggvorbis/libogg/include

all: subdirs liboggvorbisplay.a 

libvorbisplay.a:$(OBJS)
	-cp $(OBJS) ./build/
	# $(KOS_AR) rcs ./lib/libvorbisplay.a build/main.o build/sndoggvorbis.o

liboggvorbisplay.a:$(OBJS)
	-cp $(OBJS) ./build/
	-cp ../liboggvorbis/build/*.o ./build/
	$(KOS_AR) rcs ./lib/liboggvorbisplay.a ./build/*.o

clean: clean_subdirs
	-rm -f $(OBJS) *.bck
	-rm -f ./build/*.o
	-rm -f ./lib/*.a

include ../../../../sdk/Makefile.library

