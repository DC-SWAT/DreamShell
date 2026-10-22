#
# CRT Test app for DreamShell
#

SUBDIRS = modules

include ../../sdk/Makefile.cfg

APP_NAME = crttest
APP_DIR = $(DS_BUILD)/apps/$(APP_NAME)
DEPS = modules/app_$(APP_NAME).klf

all: install

ART = images/scroll_near.rgb images/scroll_sky.rgb images/scroll_mid.rgb \
	images/scroll_cloud.rgb images/scroll_floor.rgb images/scroll_vert.rgb

$(DEPS): modules/module.c modules/draw.c modules/patterns.c modules/draw.h modules/patterns.h \
	modules/view.c modules/view.h modules/gpu.c modules/gpu.h \
	modules/scroll_art.c modules/scroll_art.h modules/scroll_clouds.h
	cd modules && $(MAKE)

clean:
	cd modules && $(MAKE) clean && cd ../

install: app.xml $(DEPS) sounds/theme.wav $(ART)
	-mkdir -p $(APP_DIR)
	-mkdir -p $(APP_DIR)/modules
	-mkdir -p $(APP_DIR)/images
	-mkdir -p $(APP_DIR)/sounds
	cp modules/app_$(APP_NAME).klf $(APP_DIR)/modules/app_$(APP_NAME).klf
	cp sounds/theme.wav $(APP_DIR)/sounds/theme.wav
	-rm -f $(APP_DIR)/theme.wav
	cp $(ART) $(APP_DIR)/images/
	-rm -f $(APP_DIR)/scroll_near.rgb $(APP_DIR)/scroll_sky.rgb \
		$(APP_DIR)/scroll_mid.rgb $(APP_DIR)/scroll_cloud.rgb \
		$(APP_DIR)/scroll_floor.rgb $(APP_DIR)/scroll_vert.rgb
	cp app.xml $(APP_DIR)/app.xml
	cp images/*.png $(APP_DIR)/images
