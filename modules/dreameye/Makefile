#
# DreamEye module for DreamShell
# Copyright (C) 2015, 2023 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = dreameye
OBJS = module.o $(TARGET_NAME).o preview.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 2
VER_MICRO = 1

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
