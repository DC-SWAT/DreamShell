#
# Region Changer module for DreamShell
# Copyright (C) 2010-2016 SWAT
# http://www.dc-swat.ru
#

APP_NAME = region_changer
LUA_MODULE_NAME = RC
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o flashrom.o
DBG_LIBS = -lds -ltolua
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 8
VER_MICRO = 5

all: rm-elf

include ../../../sdk/Makefile.loadable

KOS_CFLAGS += -I$(DS_SDK)/include/lua

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)

