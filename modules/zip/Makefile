#
# zip module for DreamShell
# Copyright (C) 2009-2020 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = zip
OBJS = module.o minizip.o miniunz.o \
		./minizip/zip.o ./minizip/unzip.o \
		./minizip/ioapi.o

DBG_LIBS = -lds -lbzip2
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 30

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I./minizip -DHAVE_BZIP2=1 -DIOAPI_NO_64=1

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
