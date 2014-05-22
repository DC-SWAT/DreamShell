#
# SDL_net module for DreamShell
# Copyright (C) 2012-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = SDL_net
OBJS = lib/SDLnet.o lib/SDLnetselect.o lib/SDLnetTCP.o lib/SDLnetUDP.o module.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 2
VER_MICRO = 8

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I$(DS_SDK)/include/SDL

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
