#
# Launch App module for DreamShell
# Copyright (C) 2026 SWAT
#

APP_NAME = launch_app
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o items.o scene.o config.o
DBG_LIBS = -lds -lwave
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
