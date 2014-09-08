#
# Main App module for DreamShell
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

APP_NAME = main
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 4
VER_MICRO = 1

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)

