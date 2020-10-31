#
# mp3 module for DreamShell
# Copyright (C) 2011-2020 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = mpg123
OBJS = module.o
DBG_LIBS = -lds
LIBS = -lmp3_$(TARGET_NAME)
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 2
VER_MICRO = 0

KOS_LIB_PATHS += -L./libmp3

all: rm-elf library

library:
	cd ./libmp3 && make

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)


