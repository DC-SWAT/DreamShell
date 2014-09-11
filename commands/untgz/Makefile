# DreamShell ##version##
#
# Copyright (C) 2011-2013 SWAT
# DreamShell command Makefile
# http://www.dc-swat.ru
#

TARGET = untgz
OBJS = untgz.o
DBG_LIBS = -lds

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	
install: $(TARGET)
	-rm $(DS_BUILD)/cmds/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/cmds/$(TARGET)
