#
# File manager App for DreamShell
# Copyright (C) 2009-2016, 2026 SWAT
# http://www.dc-swat.ru
#

include ../../sdk/Makefile.cfg

APP_NAME = filemanager
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = app.xml lua/main.lua

all: install

clean:
	@echo Nothing clean

install: app.xml $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/images
	cp app.xml $(APP_DIR)/app.xml
	cp images/*.png $(APP_DIR)/images
	cp -R images/toolbar $(APP_DIR)/images
	cp -R lua $(APP_DIR)
