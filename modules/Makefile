#
# DreamShell modules Makefile
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

_SUBDIRS = bzip2 ffmpeg isofs isoldr ppp \
		luaDS luaKOS luaSDL luaGUI luaMXML luaSQL luaSTD \
		mp3 ogg SDL_net sqlite3 zip luaSocket luaTask \
		http xvid adx s3m tolua tolua++ vkb opkg ini aicaos \
		angelscript minilzo http httpd telnetd mongoose \
		gumbo bflash OpenGL


all: $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS))

$(patsubst %, _clean_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean

install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install
