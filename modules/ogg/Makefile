#
# oggvorbis module for DreamShell
# Copyright (C) 2011-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = oggvorbis
OBJS = module.o
DBG_LIBS = -lds
LIBS = -loggvorbisplay
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 3
VER_MICRO = 0

KOS_LIB_PATHS += -L./liboggvorbis/liboggvorbisplay/lib

all: rm-elf

makelib:
	cd ./liboggvorbis && make

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)


