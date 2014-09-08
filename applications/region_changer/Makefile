#
# Region Changer App for DreamShell
# Copyright (C) 2010-2014 SWAT
# http://www.dc-swat.ru
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = region_changer
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = app.xml lua/main.lua modules/app_$(APP_NAME).klf

all: install

$(DEPS): modules/module.c
	cd modules && make
	
clean:
	cd modules && make clean && cd ../

install: $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp -R images $(APP_DIR)
	cp -R lua $(APP_DIR)
	cp -R doc $(APP_DIR)
