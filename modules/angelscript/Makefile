#
# AngelScript module for DreamShell
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = angelscript
OBJS = module.o
DBG_LIBS = -lds
LIBS = -langelscript -lk++
EXPORTS_FILE = exports.txt

VER_MAJOR = 2
VER_MINOR = 22
VER_MICRO = 0

LIB_VER = $(VER_MAJOR).$(VER_MINOR).$(VER_MICRO)
KOS_LIB_PATHS += -L./$(LIB_VER)/$(TARGET_NAME)/lib

all: rm-elf library

library:
	cd ./$(LIB_VER)/$(TARGET_NAME)/source && make

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)


