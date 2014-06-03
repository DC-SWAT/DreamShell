#
# opkg module for DreamShell
# Copyright (C) 2013-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = opkg
OBJS = module.o libopk.o unsqfs.o libini/libini.o
DBG_LIBS = -lds -lminilzo
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 2

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I./libini -DUSE_GZIP -DUSE_LZO $(KOS_CSTD) \
				-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE #-DSQUASHFS_TRACE 

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
