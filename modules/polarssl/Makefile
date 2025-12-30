#
# PolarSSL module for DreamShell
# Copyright (C) 2025 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = polarssl
TARGET_LIB = lib$(TARGET_NAME)-shared.a

OBJS = module.o

DBG_LIBS = -lds
LIBS = -lpolarssl

EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	-rm -f $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
