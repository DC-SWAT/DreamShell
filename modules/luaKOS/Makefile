#
# luaKOS module for DreamShell
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = luaKOS
LUA_MODULE_NAME = KOS
OBJS = module.o
DBG_LIBS = -lds -ltolua
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 3

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I$(DS_SDK)/include/lua

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)

