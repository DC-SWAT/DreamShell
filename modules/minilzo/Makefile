#
# minilzo module for DreamShell
# Copyright (C) 2009-2022 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = minilzo
OBJS = module.o $(TARGET_NAME).o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 2
VER_MINOR = 0
VER_MICRO = 9

KOS_CFLAGS += -I../../include/minilzo

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
