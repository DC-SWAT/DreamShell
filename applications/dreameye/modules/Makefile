#
# Dreameye app module for DreamShell
# Copyright (C) 2023-2025 SWAT
# http://www.dc-swat.ru
#

APP_NAME = dreameye
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o qr_code.o photo.o gallery.o
DBG_LIBS = -lds -ldreameye -lquirc
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
