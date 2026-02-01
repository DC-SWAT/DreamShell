#
# ISO Loader module for DreamShell
# Copyright (C) 2009-2026 SWAT
#

TARGET_NAME = isoldr
OBJS = module.o exec.o execasm.o vm2.o preset.o
DBG_LIBS = -lds -lisofs
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 8
VER_MICRO = 4

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET) module.o
	
install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
