#
# MIE JVS app module for DreamShell
# Copyright (C) 2026 SWAT
#

APP_NAME = mie_jvs
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o
EXPORTS_FILE = exports.txt

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
