# DreamShell ##version##
#
# Copyright (C) 2014-2020 SWAT
# DreamShell command Makefile
# http://www.dc-swat.ru
#

TARGET = mke2fs
OBJS = mke2fs.o
DBG_LIBS = -lds

all: rm-elf

include ../../sdk/Makefile.loadable

KOS_CFLAGS += -I$(KOS_BASE)/addons/libkosext2fs -Werror -W -std=gnu99

rm-elf:
	-rm -f $(TARGET)
	
install: $(TARGET)
	-rm $(DS_BUILD)/cmds/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/cmds/$(TARGET)
