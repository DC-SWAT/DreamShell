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

all: subdirs liboggvorbisplay-local.a 

libvorbisplay-local.a:$(OBJS)
	-cp $(OBJS) ./build/

liboggvorbisplay-local.a:$(OBJS)
	-cp $(OBJS) ./build/
	-cp ../liboggvorbis/build/*.o ./build/
	$(KOS_AR) rcs ./lib/liboggvorbisplay-local.a ./build/*.o

clean: clean_subdirs
	-rm -f $(OBJS) *.bck
	-rm -f ./build/*.o
	-rm -f ./lib/*.a

include ../../../../sdk/Makefile.library

