#
# GDPlay App module for DreamShell
# Copyright (C) 2016 megavolt85
# http://www.dc-swat.ru
#

APP_NAME = gdplay
TARGET_NAME = app_$(APP_NAME)
OBJS = exec.o module.o
DBG_LIBS = -lds
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

