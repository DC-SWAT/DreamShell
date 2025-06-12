#
# ISO Loader app module for DreamShell
# Copyright (C) 2011-2025 SWAT
# http://www.dc-swat.ru
#

APP_NAME = iso_loader
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o utils.o
DBG_LIBS = -lds -lisofs -lisoldr -lwave
EXPORTS_FILE = exports.txt

KOS_CFLAGS += -I./

VER_MAJOR = 0
VER_MINOR = 8
VER_MICRO = 3

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
