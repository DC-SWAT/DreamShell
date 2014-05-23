# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Dan Potter

OBJS =
SUBDIRS = libogg libvorbis 

all: subdirs liboggvorbis.a 

# Target: liboggvorbis.a -> base library for OggVobis Files
liboggvorbis.a:
	-rm -f ./build/*.o
	-cp ./libvorbis/build/*.o ./build/
	-cp ./libogg/build/*.o ./build/
	# $(KOS_AR) rcs ./lib/liboggvorbis.a ./build/*.o

# Target: liboggvorbisplay.a -> KOS sndstream decoder library
liboggvorbisplay.a:
	-rm -f ./build/*.o
	-cp ./libvorbis/build/*.o build/
	-cp ./libogg/build/*.o build/
	-cp ../libsndoggvorbis/build/*.o build/
	$(KOS_AR) rcs ./lib/liboggvorbisplay.a build/*.o

include ../../../../sdk/Makefile.library

clean: clean_subdirs
	-rm -f build/*.o lib/*.a

run:

