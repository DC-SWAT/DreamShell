#
# bflash module for DreamShell
# Copyright (C) 2013-2016 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = bflash
OBJS = module.o $(TARGET_NAME).o devices.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 6
VER_MICRO = 0

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
