#
# luaSQL module for DreamShell
# Copyright (C) 2011-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = luaSQL
OBJS = module.o src/luasql.o src/ls_sqlite3.o
DBG_LIBS = -lds -lsqlite3
EXPORTS_FILE = exports.txt

VER_MAJOR = 2
VER_MINOR = 1
VER_MICRO = 1

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
