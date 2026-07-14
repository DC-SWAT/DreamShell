#
# Launch App for DreamShell
# Copyright (C) 2026 SWAT
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = launch_app
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

all: install

$(DEPS): $(wildcard modules/*.c)
	cd modules && make

clean:
	cd modules && make clean && cd ../

install: app.xml config.lua $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	-mkdir -p $(APP_DIR)/scripts
	-mkdir -p $(APP_DIR)/images
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp config.lua $(APP_DIR)/config.lua
	cp images/*.png $(APP_DIR)/images
