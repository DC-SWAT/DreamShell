# DreamShell ##version##
#
# Copyright (C) 2015 SWAT
# DreamShell command Makefile
# http://www.dc-swat.ru
#

TARGET = ciso
OBJS = main.o
DBG_LIBS = -lds -lminilzo
VER_MAJOR = 1
VER_MINOR = 1

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)
	
install: $(TARGET)
	-rm $(DS_BUILD)/cmds/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/cmds/$(TARGET)
