#
# DreamShell SDK img4dc Makefile
# Copyright (C) 2020 SWAT
# http://www.dc-swat.ru
#

INSTALL = install
DESTDIR = ../..
TARGET_CDI = build/cdi4dc/cdi4dc
TARGET_MDS = build/mds4dc/mds4dc

all: $(TARGET_CDI)

$(TARGET_CDI): build
	@cd build && cmake ../ && make

build:
	@mkdir build

install: $(TARGET_CDI)
	$(INSTALL) -m 755 $(TARGET_CDI) $(DESTDIR)/cdi4dc
	$(INSTALL) -m 755 $(TARGET_MDS) $(DESTDIR)/mds4dc

clean:
	rm -rf build
