#
# luaSocket module for DreamShell
# Copyright (C) 2011-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = luaSocket
OBJS = module.o \
	src/luasocket.o \
	src/timeout.o \
	src/buffer.o \
	src/io.o \
	src/auxiliar.o \
	src/options.o \
	src/inet.o \
	src/tcp.o \
	src/udp.o \
	src/except.o \
	src/select.o \
	src/usocket.o \
	src/mime.o

DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 2
VER_MINOR = 0
VER_MICRO = 1

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I./src -I$(DS_SDK)/include/lua

install_lua:
	cp src/ltn12.lua $(DS_BUILD)/lua/lib/ltn12.lua
	cp src/socket.lua $(DS_BUILD)/lua/lib/socket.lua
	cp src/mime.lua $(DS_BUILD)/lua/lib/mime.lua
	mkdir -p $(DS_BUILD)/lua/lib/socket
	cp src/http.lua $(DS_BUILD)/lua/lib/socket/http.lua
	cp src/url.lua $(DS_BUILD)/lua/lib/socket/url.lua
	cp src/tp.lua $(DS_BUILD)/lua/lib/socket/tp.lua
	cp src/ftp.lua $(DS_BUILD)/lua/lib/socket/ftp.lua
	cp src/smtp.lua $(DS_BUILD)/lua/lib/socket/smtp.lua

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB) install_lua
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
	

