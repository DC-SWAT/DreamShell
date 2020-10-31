#
# Bzip2 module for DreamShell
# Copyright (C) 2009-2020 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = bzip2
OBJS = module.o           \
		bzip2/blocksort.o  \
		bzip2/huffman.o    \
		bzip2/crctable.o   \
		bzip2/randtable.o  \
		bzip2/compress.o   \
		bzip2/decompress.o \
		bzip2/bzlib.o

DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 6

KOS_LIB_PATHS += -L./$(TARGET_NAME)

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
	ln -nsf $(DS_SDK)/lib/$(TARGET_LIB) $(DS_SDK)/lib/libbz2.a
