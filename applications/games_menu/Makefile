#
# GAMES Loader App for DreamShell
# Copyright (C) 2024 MANIAC VERA
# http://www.dc-swat.ru
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = games_menu
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

KOS_CFLAGS += -I./

all: install

$(DEPS): $(wildcard modules/*.c)
	cd modules && make
	
clean:
	cd modules && make clean && cd ../

install: app.xml $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	-mkdir -p $(APP_DIR)/images
	-mkdir -p $(APP_DIR)/covers	
	-mkdir -p $(APP_DIR)/fonts
	-mkdir -p $(APP_DIR)/music
	-mkdir -p $(APP_DIR)/presets
	-mkdir -p $(APP_DIR)/resources
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp -R images $(APP_DIR)
	cp -R fonts $(APP_DIR)
	cp -R resources $(APP_DIR)
	