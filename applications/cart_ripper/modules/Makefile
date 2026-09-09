#
# Cart Ripper app module for DreamShell
# Copyright (C) 2026 SWAT
# http://www.dc-swat.ru
#

APP_NAME = cart_ripper
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o naomi_5881.o
DBG_LIBS = -lds -lisofs -lisoldr
EXPORTS_FILE = exports.txt

KOS_CFLAGS += -I./

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
