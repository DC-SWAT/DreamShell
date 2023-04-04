#
# DreamShell commands Makefile
# Copyright (C) 2009-2016 SWAT
# http://www.dc-swat.ru
#

_SUBDIRS = bin2iso flashrom netstat ping scramble \
			untgz vmu vmode mke2fs gdiopt ciso sip

all: $(patsubst %, _dir_%, $(_SUBDIRS))

$(patsubst %, _dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _dir_%, %, $@)

clean: $(patsubst %, _clean_dir_%, $(_SUBDIRS))

$(patsubst %, _clean_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _clean_dir_%, %, $@) clean


install: $(patsubst %, _install_dir_%, $(_SUBDIRS))

$(patsubst %, _install_dir_%, $(_SUBDIRS)):
	$(MAKE) -C $(patsubst _install_dir_%, %, $@) install

