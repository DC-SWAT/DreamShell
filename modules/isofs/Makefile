#
# isofs module for DreamShell
# Copyright (C) 2011-2014 SWAT
#

TARGET_NAME = isofs
OBJS = module.o fs_iso9660.o ciso.o cdi.o gdi.o
DBG_LIBS = -lds -lminilzo
EXPORTS_FILE = exports.txt
#KOS_CFLAGS += -DDEBUG=1

VER_MAJOR = 1
VER_MINOR = 5
VER_MICRO = 1

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
