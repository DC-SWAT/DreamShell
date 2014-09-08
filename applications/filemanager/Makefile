#
# File manager App for DreamShell
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

include ../../sdk/Makefile.cfg

APP_NAME = filemanager
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = app.xml lua/main.lua

all: install

clean:
	@echo Nothing clean

install: $(DEPS)
	-mkdir -p $(APP_DIR)
	cp app.xml $(APP_DIR)/app.xml
	cp -R images $(APP_DIR)
	cp -R lua $(APP_DIR)
