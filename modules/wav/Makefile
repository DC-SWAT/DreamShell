#
# WAV module for DreamShell
# Copyright (C) 2023-2024 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = wave
OBJS = module.o libwav/sndwav.o libwav/libwav.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 1
VER_MICRO = 1

KOS_CFLAGS += -I./libwav

all: rm-elf library

include ../../sdk/Makefile.loadable

libwav/libwav.h:
	git clone https://github.com/Dreamcast-Projects/libwav.git

library: libwav/libwav.h

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
