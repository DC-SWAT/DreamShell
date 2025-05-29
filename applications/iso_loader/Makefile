#
# ISO Loader App for DreamShell
# Copyright (C) 2011-2025 SWAT
# http://www.dc-swat.ru
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = iso_loader
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

ROMDISKS = presets_ide.romfs presets_sd.romfs presets_cd.romfs

all: install

$(DEPS): modules/module.c
	cd modules && make

presets/: presets.tar.gz
	tar -xzf presets.tar.gz

presets_ide.romfs: presets/
	$(KOS_GENROMFS) -f presets_ide.romfs -d presets/ide/ -V "presets_ide"

presets_sd.romfs: presets/
	$(KOS_GENROMFS) -f presets_sd.romfs -d presets/sd/ -V "presets_sd"

presets_cd.romfs: presets/
	$(KOS_GENROMFS) -f presets_cd.romfs -d presets/cd/ -V "presets_cd"

romdisks: $(ROMDISKS)

clean:
	cd modules && make clean && cd ../
	-rm -f $(ROMDISKS)
	-rm -rf presets/

install: app.xml $(DEPS) $(ROMDISKS)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	-mkdir -p $(APP_DIR)/presets
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp app.xml $(APP_DIR)/app.xml
	cp -R images $(APP_DIR)
	cp -R resources $(APP_DIR)
	mv $(ROMDISKS) $(APP_DIR)/resources
	-rm -rf presets/
