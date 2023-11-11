#
# Bios ROM for DreamShell
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

SUBDIRS = bootstrap

include ../../sdk/Makefile.cfg

FW_NAME = bios
FW_BOOTSTRAP = ./bootstrap/boot.bin
FW_DIR = $(DS_BUILD)/firmware/$(FW_NAME)/ds
RD_DIR = $(FW_DIR)/romdisks

FW_FILES = ds_core.bios ds_core_rd.bios \
			boot_loader.bios boot_loader_rd_ext.bios

DEPS = $(FW_BOOTSTRAP) $(DS_BUILD)/DS_CORE.BIN \
		$(DS_BASE)/firmware/bootloader/bootloader.bin \
		initrd.img initrd_ext.img initrd_core.img

all: fw install

$(FW_BOOTSTRAP):
	cd bootstrap && make

fw: $(FW_FILES)
$(FW_FILES): $(DEPS)
	-rm -f $(FW_FILES)
	cat $(FW_BOOTSTRAP) $(DS_BUILD)/DS_CORE.BIN > ds_core.bios
	cat $(FW_BOOTSTRAP) $(DS_BUILD)/DS_CORE.BIN initrd.img > ds_core_rd.bios
	cat $(FW_BOOTSTRAP) ../bootloader/bootloader.bin > boot_loader.bios
	cat $(FW_BOOTSTRAP) ../bootloader/bootloader.bin initrd_ext.img > boot_loader_rd_ext.bios
	cat $(FW_BOOTSTRAP) ../bootloader/bootloader.bin initrd_core.img > boot_loader_rd_core.bios
	
initrd: initrd.img initrd_ext.img initrd_core.img
initrd.img: ./initrd/* ./initrd_ext/*
	$(KOS_GENROMFS) -f initrd.img -d initrd -V DreamShell -v -x .svn
	$(KOS_GENROMFS) -f initrd_ext.img -d initrd_ext -V DreamShell -v -x .svn
	cp $(DS_BUILD)/DS_CORE.BIN ./initrd/DS_CORE.BIN
	gzip -9 ./initrd/DS_CORE.BIN
	mv ./initrd/DS_CORE.BIN.gz ./initrd/ZDS_CORE.BIN
	$(KOS_GENROMFS) -f initrd_core.img -d initrd -V DreamShell -v -x .svn
	rm -f ./initrd/ZDS_CORE.BIN

install: fw rm-old
	cp ds_core.bios $(FW_DIR)/ds_core.bios
	cp ds_core_rd.bios $(FW_DIR)/ds_core_rd.bios
	cp boot_loader.bios $(FW_DIR)/boot_loader.bios
	cp boot_loader_rd_ext.bios $(FW_DIR)/boot_loader_rd_ext.bios
	cp boot_loader_rd_core.bios $(FW_DIR)/boot_loader_rd_core.bios
	cp initrd.img $(RD_DIR)/initrd.img
	cp initrd_ext.img $(RD_DIR)/initrd_ext.img
	cp initrd_core.img $(RD_DIR)/initrd_core.img
	
rm-old:
	@-mkdir -p $(FW_DIR)
	-rm -f $(FW_DIR)/ds_core.bios
	-rm -f $(FW_DIR)/ds_core_rd.bios
	-rm -f $(FW_DIR)/boot_loader.bios
	-rm -f $(FW_DIR)/boot_loader_rd_ext.bios
	-rm -f $(FW_DIR)/boot_loader_rd_core.bios
	-rm -f $(RD_DIR)/initrd.img
	-rm -f $(RD_DIR)/initrd_ext.img
	-rm -f $(RD_DIR)/initrd_core.img
	
clean:
	-rm -f $(FW_FILES)
	-rm -f initrd.img initrd_ext.img initrd_core.img
