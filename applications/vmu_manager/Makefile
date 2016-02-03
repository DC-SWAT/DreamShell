#
# Speedtest App for DreamShell
# Copyright (C) 2015 megavolt85
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = vmu_manager
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf
LXDREAM_DIR = $(KOS_BASE)/DSonPC_Loader/DreamShell/apps/$(APP_NAME)

# Uncomment for enable DEBUG
#KOS_CFLAGS += -DVMDEBUG 

all: install

$(DEPS): modules/module.c
	cd modules && make
	
clean:
	cd modules && make clean && cd ../

install: app.xml $(DEPS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp -R images $(APP_DIR)
	cp -R fonts $(APP_DIR)
	
lxdream: app.xml $(DEPS)
	-mkdir -p $(LXDREAM_DIR)
	-mkdir -p $(LXDREAM_DIR)/modules
	-mkdir -p $(LXDREAM_DIR)/images
	-mkdir -p $(LXDREAM_DIR)/fonts
	cp modules/app_$(APP_NAME).klf $(LXDREAM_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(LXDREAM_DIR)/app.xml
	cp -R images $(LXDREAM_DIR)
	cp -R fonts $(LXDREAM_DIR)
	$(KOS_BASE)/DSonPC_Loader/DSloader
