#
# luaGUI module for DreamShell
# Copyright (C) 2007-2013 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = luaGUI
LUA_MODULE_NAME = GUI
LUA_MODULE_OBJS = tolua_clean_$(LUA_MODULE_NAME).o
OBJS = module.o
DBG_LIBS = -lds -ltolua
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 2

OLD_FUNC =  tolua_function(tolua_S,"GUI_
NEW_FUNC = tolua_function(tolua_S,"

all: rm-elf

include ../../sdk/Makefile.loadable

tolua_clean_$(LUA_MODULE_NAME).c: tolua_$(LUA_MODULE_NAME).c
	sed 's/$(OLD_FUNC)/$(NEW_FUNC)/g' tolua_$(LUA_MODULE_NAME).c > tolua_clean_$(LUA_MODULE_NAME).c

KOS_CFLAGS += -I$(DS_SDK)/include/lua

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
