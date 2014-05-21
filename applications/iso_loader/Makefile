#
# ISO Loader App for DreamShell
# Copyright (C) 2011-2014 SWAT
# http://www.dc-swat.ru
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = iso_loader
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

all: install

$(DEPS): modules/module.c
	cd modules
	make

install: app.xml $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp -R images $(APP_DIR)
	cp -R covers $(APP_DIR)
	cp -R presets $(APP_DIR)

