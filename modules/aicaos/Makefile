#
# aicaos module for DreamShell
# Copyright (C) 2013-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = aicaos
TARGET_SPU = $(TARGET_NAME).drv
OBJS = module.o sh4/main.o sh4/aica_sh4.o aica_common_sh4.o sh4/aica_syscalls.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt
OPKG_DATA = default.desktop install.lua $(TARGET_NAME).klf $(TARGET_SPU)

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 1

KOS_CFLAGS += -I../../include/aicaos

all: rm-elf $(TARGET_SPU)

include ../../sdk/Makefile.loadable

aica_common_sh4.o: aica_common.c
	$(KOS_CC) $(KOS_CFLAGS) -c $< -o $@

$(TARGET_SPU):
	make -C arm SPUTARGET=$(TARGET_SPU)
	make -C arm SPUTARGET=$(TARGET_SPU) install

clean: arm-clean

arm-clean:
	make -C arm clean SPUTARGET=$(TARGET_SPU)

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_SPU)

install: $(TARGET) $(TARGET_SPU) $(TARGET_LIB) #opkg
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_BUILD)/firmware/aica/$(TARGET_SPU)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_SPU) $(DS_BUILD)/firmware/aica/$(TARGET_SPU)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
