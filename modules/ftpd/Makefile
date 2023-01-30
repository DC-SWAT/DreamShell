#
# FTP server module for DreamShell
# Copyright (C)2023 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = ftpd
LIBRARY_NAME = lftpd

OBJS = module.o \
	$(LIBRARY_NAME)/lftpd.o \
	$(LIBRARY_NAME)/lftpd_inet.o \
	$(LIBRARY_NAME)/lftpd_string.o \
	$(LIBRARY_NAME)/lftpd_log.o \
	$(LIBRARY_NAME)/lftpd_io.o

DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 1
VER_MICRO = 0

all: rm-elf #$(LIBRARY_NAME)/Makefile

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I$(DS_SDK)/include/network

$(LIBRARY_NAME)/Makefile:
	git clone https://github.com/vonnieda/$(LIBRARY_NAME).git
	cd ./$(LIBRARY_NAME) && git checkout 7c54d2676ba7a2bcf9f72fd21557d5b97729771f

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	-rm -f $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
