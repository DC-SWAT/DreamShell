#
# GD Ripper App module for DreamShell
# Copyright (C) 2014 megavolt85
# http://www.dc-swat.ru
#

APP_NAME = gd_ripper
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o
DBG_LIBS = -lds -lisofs
EXPORTS_FILE = exports.txt

KOS_CFLAGS += -I./

VER_MAJOR = 1
VER_MINOR = 6
VER_MICRO = 0

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
