# DreamShell ##version##
#
# Copyright (C) 2015 SWAT
# DreamShell command Makefile
# http://www.dc-swat.ru
#

TARGET = gdiopt
OBJS = main.o
DBG_LIBS = -lds
VER_MAJOR = 0
VER_MINOR = 1

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	
install: $(TARGET)
	-rm $(DS_BUILD)/cmds/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/cmds/$(TARGET)
