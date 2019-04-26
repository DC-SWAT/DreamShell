#
# PPP module for DreamShell
# Copyright (C) 2014-2019 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = ppp
MODEM_DIR = $(KOS_BASE)/kernel/arch/dreamcast/hardware/modem
OBJS = module.o $(MODEM_DIR)/mdata.o $(MODEM_DIR)/mintr.o \
		$(MODEM_DIR)/modem.o $(MODEM_DIR)/chainbuf.o
LIBS = -lppp
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 1

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += $(KOS_CSTD)

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
