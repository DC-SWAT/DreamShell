#
# Main App for DreamShell
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = main
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
	cp -R images $(APP_DIR)
	cp -R scripts $(APP_DIR)
	cp app.xml $(APP_DIR)/app.xml
