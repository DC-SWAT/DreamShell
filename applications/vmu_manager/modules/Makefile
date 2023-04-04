#
# VMU Manager App module for DreamShell
# Copyright (C) 2015 megavolt85
# http://www.dc-swat.ru
#

APP_NAME = vmu_manager
TARGET_NAME = app_$(APP_NAME)
OBJS = vmdfs.o fs_vmd.o module.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0

KOS_CFLAGS += -I./

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)

