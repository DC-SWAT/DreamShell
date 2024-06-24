#
# DreamShell libraries Makefile
# Copyright (C) 2008-2022 SWAT
# http://www.dc-swat.ru
#

_SUBDIRS = libcfg/src libparallax SDL_rtf \
	lua SDL SDL_gfx SDL_image SDL_ttf \
	libtsunami

_SUBDIRS_CLEAN = $(_SUBDIRS) freetype mxml

all: build_freetype build_mxml $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS_CLEAN))

$(patsubst %, _clean_dir_%, $(_SUBDIRS_CLEAN)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install

build_freetype:
	cd freetype/builds/unix && ./conf.sh
	$(MAKE) -C freetype
	cp ./freetype/objs/.libs/libfreetype.a libfreetype.a

build_mxml:
	cd ./mxml && ./conf.sh
	$(MAKE) -C mxml libmxml.a
	mv mxml/libmxml.a libmxml.a
