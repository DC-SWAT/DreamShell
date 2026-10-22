#
# CRT Test app module for DreamShell
#

APP_NAME = crttest
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o draw.o patterns.o scroll_art.o gpu.o view.o
DBG_LIBS = -lds -lwave
EXPORTS_FILE = exports.txt

KOS_CFLAGS += -I./

VER_MAJOR = 1
VER_MINOR = 5
VER_MICRO = 0

all: rm-elf

include ../../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
