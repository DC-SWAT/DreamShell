#
# Main App module for DreamShell
# Copyright (C) 2009-2016 SWAT
# http://www.dc-swat.ru
#

APP_NAME = main
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 6
VER_MICRO = 0

KOS_CFLAGS += -I./

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)

