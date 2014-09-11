# DreamShell ##version##
# (с) 2008-2014 SWAT
# http://www.dc-swat.ru
# 

_SUBDIRS = libcfg/src libparallax libpng lua mxml \
			freetype SDL_gfx SDL_image SDL_ttf SDL_rtf


all: $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS))

$(patsubst %, _clean_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install
