#
# Cart Ripper App for DreamShell
# Copyright (C) 2026 SWAT
# http://www.dc-swat.ru
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = cart_ripper
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

all: install

$(DEPS): modules/module.c modules/naomi_5881.c
	cd modules && make

clean:
	cd modules && make clean && cd ../

install: app.xml $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	-mkdir -p $(APP_DIR)/images
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp images/*.png $(APP_DIR)/images
