#
# CRT Test app for DreamShell
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = crttest
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

all: install

ART = modules/scroll_near.rgb modules/scroll_sky.rgb modules/scroll_mid.rgb \
	modules/scroll_cloud.rgb modules/scroll_floor.rgb modules/scroll_vert.rgb

$(DEPS): modules/module.c modules/draw.c modules/patterns.c modules/draw.h modules/patterns.h \
	modules/view.c modules/view.h modules/gpu.c modules/gpu.h \
	modules/scroll_art.c modules/scroll_art.h modules/scroll_clouds.h
	cd modules && $(MAKE)

clean:
	cd modules && $(MAKE) clean && cd ../

install: app.xml $(DEPS) theme.wav $(ART)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	-mkdir -p $(APP_DIR)/images
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp theme.wav $(APP_DIR)/theme.wav
	cp $(ART) $(APP_DIR)/
	cp app.xml $(APP_DIR)/app.xml
	cp images/*.png $(APP_DIR)/images
