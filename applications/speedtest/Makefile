#
# Speedtest App for DreamShell
# Copyright (C) 2014 megavolt85
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = speedtest
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
	cp -R fonts $(APP_DIR)
