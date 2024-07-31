#
# tolua module for DreamShell
# Copyright (C) 2009-2022 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = tolua
TOLUA_LIB = ./tolua/src/lib
OBJS = module.o \
	$(TOLUA_LIB)/tolua_event.o \
	$(TOLUA_LIB)/tolua_is.o \
	$(TOLUA_LIB)/tolua_map.o \
	$(TOLUA_LIB)/tolua_push.o \
	$(TOLUA_LIB)/tolua_to.o
	
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 5
VER_MINOR = 1
VER_MICRO = 4

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I./tolua/include -I$(DS_SDK)/include/lua

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
