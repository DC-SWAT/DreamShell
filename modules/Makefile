#
# DreamShell modules Makefile
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

_SUBDIRS = vkb luaDS luaKOS luaSDL luaGUI luaMXML luaSTD \
		zip bzip2 minilzo httpd telnetd mongoose aicaos

all: $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS))

$(patsubst %, _clean_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install
