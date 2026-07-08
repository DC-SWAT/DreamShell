# DreamShell ##version##
#
# Copyright (C) 2026 Ruslan Rostovtsev
# DreamShell command Makefile
# http://www.dc-swat.ru
#

TARGET = mie
OBJS = main.o
DBG_LIBS = -lds

all: rm-elf

include ../../sdk/Makefile.loadable

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/cmds/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/cmds/$(TARGET)
