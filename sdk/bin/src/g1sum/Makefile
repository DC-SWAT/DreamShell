#
# DreamShell SDK g1sum
#
# Copyright (C) 2026 SWAT
# http://www.dc-swat.ru
#

TARGET = g1sum
CC ?= gcc
CFLAGS = -O2 -Wall -Wextra
INSTALL = install
DESTDIR = ../..

all: $(TARGET)

$(TARGET): g1sum.c
	$(CC) $(CFLAGS) -o $@ $<
	strip $@

install: $(TARGET)
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)/$(TARGET)

clean:
	rm -f $(TARGET)
