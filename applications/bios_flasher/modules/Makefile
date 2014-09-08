#
# Bios Flasher App module for DreamShell
# Copyright (C) 2009-2014 SWAT
# Copyright (C) 2013 Yev
# http://www.dc-swat.ru
#

APP_NAME = bios_flasher
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o
DBG_LIBS = -lds -lbflash
EXPORTS_FILE = exports.txt
#KOS_CFLAGS += $(KOS_CSTD)

VER_MAJOR = 2
VER_MINOR = 0
VER_MICRO = 1

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)

