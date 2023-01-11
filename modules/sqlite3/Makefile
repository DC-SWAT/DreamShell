#
# sqlite3 module for DreamShell
# Copyright (C) 2011-2022 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = sqlite3
OBJS = sqlite3.o module.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 3
VER_MINOR = 8
VER_MICRO = 1
VER_BUILD = 0

KOS_LIB_PATHS = -L../../lib
KOS_CFLAGS += -DSTDC_HEADERS=1 \
				-DHAVE_SYS_TYPES_H=1 \
				-DHAVE_SYS_STAT_H=1 \
				-DHAVE_STDLIB_H=1 \
				-DHAVE_STRING_H=1 \
				-DHAVE_INTTYPES_H=1 \
				-DHAVE_STDINT_H=1 \
				-DHAVE_UNISTD_H=1 \
				-DHAVE_USLEEP=1 \
				-DHAVE_LOCALTIME_R=1 \
				-DHAVE_GMTIME_R=1 \
				-DSQLITE_OMIT_WAL=1 \
				-DSQLITE_NO_SYNC=1 \
				-DSQLITE_DISABLE_DIRSYNC=1 \
				-DSQLITE_DISABLE_LFS=1 \
				-DSQLITE_OMIT_LOAD_EXTENSION=1 \
				-DSQLITE_THREADSAFE=0

all: rm-elf install

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
