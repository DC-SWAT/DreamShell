#
# DreamShell modules Makefile
# Copyright (C) 2009-2022 SWAT
# http://www.dc-swat.ru
#

_SUBDIRS = minilzo isofs isoldr ppp bflash dreameye \
		bzip2 zip ini opkg sqlite3 vkb \
		luaTask luaSocket tolua tolua++ \
		luaDS luaKOS luaSDL luaGUI luaMXML luaSQL luaSTD \
		SDL_net http httpd telnetd mongoose gumbo \
		adx s3m mp3 ogg xvid ffmpeg bitcoin openssl # aicaos

all: $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS))

$(patsubst %, _clean_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install
