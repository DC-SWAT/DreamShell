#
# GAMES app module for DreamShell
# Copyright (C) 2024 Maniac Vera
# http://www.dc-swat.ru
#

APP_NAME = games_menu
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o utils.o menu.o system_menu.o preset.o
DBG_LIBS = -lds -lisofs -lisoldr -lwave
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 1
VER_MICRO = 0

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)

