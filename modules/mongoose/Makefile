#
# mongoose module for DreamShell
# Copyright (C) 2013 SWAT
#


TARGET_NAME = mongoose
OBJS = module.o mongoose.o
DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 3
VER_MINOR = 1
VER_MICRO = 0

KOS_CFLAGS += -DNDEBUG -DNO_CGI -DNO_SSL -DNO_POPEN

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB) opkg
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
