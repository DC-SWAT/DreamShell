#
# libini module for DreamShell
# Copyright (C) 2013-2022 SWAT
#

TARGET_NAME = ini

OBJS = module.o
LIBS = -lini
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0

KOS_LIB_PATHS += -L./libini/src/.libs

all: rm-elf build_lib

libini/Makefile:
	cd ./libini && ./configure --host=sh-elf \
	--prefix="`pwd`/build" CFLAGS="${KOS_CFLAGS}" LDFLAGS="${KOS_LDFLAGS}" \
	LIBS="${KOS_LIBS}" CC="${KOS_CC}" --disable-shared

build_lib: libini/Makefile
	cd ./libini && make

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	-rm -f $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)



