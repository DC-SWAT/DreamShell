#
# xvid module for DreamShell
# Copyright (C) 2011-2020 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = xvid
OBJS = module.o
DBG_LIBS = -lds
LIBS = -lxvidcore
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0

KOS_LIB_PATHS += -L./xvidcore/build/generic/=build

all: rm-elf library

library:
	cd ./xvidcore/build/generic && make
	
library_clean:
	cd ./xvidcore/build/generic && make clean

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
	ln -nsf $(DS_SDK)/lib/$(TARGET_LIB) $(DS_SDK)/lib/libxvidcore.a
