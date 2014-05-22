#
# luaTask module for DreamShell
# Copyright (C) 2011-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = luaTask
OBJS = module.o src/ltask.o src/queue.o src/syncos.o

DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 6
VER_MICRO = 4

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I./src -I$(DS_SDK)/include/lua

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
	

