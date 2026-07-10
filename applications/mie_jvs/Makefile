#
# MIE JVS App for DreamShell
# Copyright (C) 2026 Ruslan Rostovtsev
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = mie_jvs
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
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp images/*.png $(APP_DIR)/images
