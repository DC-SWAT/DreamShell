#
# DreamShell modules Makefile
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

_SUBDIRS = vkb gumbo sqlite3 aicaos zip bzip2 minilzo \
		luaDS luaKOS luaSDL luaGUI luaMXML luaSTD luaSQL luaTask \
		SDL_net httpd telnetd mongoose \
		adx s3m mp3 ogg xvid ffmpeg

all: $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS))

$(patsubst %, _clean_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install
