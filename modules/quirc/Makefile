#
# QR-code module for DreamShell
# Copyright (C)2023 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = quirc
LIBRARY_NAME = $(TARGET_NAME)

OBJS = module.o \
	$(LIBRARY_NAME)/lib/decode.o \
	$(LIBRARY_NAME)/lib/identify.o \
	$(LIBRARY_NAME)/lib/quirc.o \
	$(LIBRARY_NAME)/lib/version_db.o

DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 2
VER_MICRO = 0

all: rm-elf $(LIBRARY_NAME)/Makefile

include ../../sdk/Makefile.loadable

$(LIBRARY_NAME)/Makefile:
	git clone https://github.com/dlbeer/$(LIBRARY_NAME).git
	cd ./$(LIBRARY_NAME) && git checkout 542848dd6b9b0eaa9587bbf25b9bc67bd8a71fca

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	-rm -f $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
#	cp $(LIBRARY_NAME)/lib/$(LIBRARY_NAME).h $(DS_SDK)/include
