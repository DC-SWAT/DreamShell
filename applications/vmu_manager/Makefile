#
# Speedtest App for DreamShell
# Copyright (C) 2015 megavolt85
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = vmu_manager
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

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

