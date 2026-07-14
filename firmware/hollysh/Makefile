#
# DreamShell HollySH BIOS firmware
# Copyright (C) 2025-2026 SWAT
# http://www.dc-swat.ru
#

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0
VER_BUILD = 0x30

VERSION = $(VER_MAJOR).$(VER_MINOR).$(VER_MICRO)

BIOS_VER = $(shell \
	major=$$(($(VER_MAJOR) & 255)); \
	minor=$$(($(VER_MINOR) & 255)); \
	micro=$$(($(VER_MICRO) & 255)); \
	build=$$(($(VER_BUILD) & 255)); \
	printf '0x%08x' $$((major * 16777216 + minor * 65536 + micro * 256 + build)))

BUILD = build
# Keep in sync with drivers/hollysh.h
BIOS_ROMDISK_ROM_OFFSET = 655360
LOADER_BIN_ADDR = 0x8c004000
LOADER_DIR = loader
LOADER_PATH = $(LOADER_DIR)/build/hollysh_loader.bin
LOADER_ROM_OFFSET = 65536
LOADER_ROM_SLOT_SIZE = 65536
BOOTLOADER_DIR = ../bootloader
BOOTLOADER_BIN = $(BOOTLOADER_DIR)/bootloader.bin
BOOTLOADER_ROM_OFFSET = $(shell expr $(LOADER_ROM_OFFSET) + $(LOADER_ROM_SLOT_SIZE))

include ../../sdk/Makefile.cfg

FONT_SRC_FILE = $(DS_RES)/firmware/bios/sega/1_01d_02.bios
FONT_SRC_OFFSET = 1048608
FONT_SRC_SIZE = 536496

TARGET_FONT_OFFSET = 1048608

TARGET_NAME = dreamshell_hollysh.bios
TARGET_BIN = $(BUILD)/$(TARGET_NAME)
FW_DIR = $(DS_BUILD)/firmware/bios/ds

BIOS_ELF = $(BUILD)/bios.elf
BIOS_BASE_BIN = $(BUILD)/bios_base.bin
BIOS_LOADER_BIN = $(BUILD)/bios_loader.bin
BIOS_BOOTLOADER_BIN = $(BUILD)/bios_bootloader.bin
BIOS_ROMDISK_BIN = $(BUILD)/bios_romdisk.bin
BIOS_CFG = $(BUILD)/bios_cfg.s
LOADER_CFG = $(BUILD)/loader_cfg.s
FONT_BIN = $(BUILD)/font.bin

ROMDISK_DIR = romdisk
ROMDISK_IMG = $(BUILD)/romdisk.img
ROMDISK_CONTENT = $(filter-out $(ROMDISK_DIR)/.keepme,$(wildcard $(ROMDISK_DIR)/*))
ROMDISK_EXTRA = $(if $(strip $(ROMDISK_CONTENT)),$(ROMDISK_IMG))

BIOS_PARTS = $(FONT_SRC_FILE) $(ROMDISK_EXTRA)

BIOS_SRCS = start.s bios.s sh4.s holly.s vectors.s

all: bios_clean $(TARGET_BIN)

romdisk: $(ROMDISK_IMG)

$(BUILD):
	mkdir -p $(BUILD)

$(BOOTLOADER_BIN):
	$(MAKE) -C $(BOOTLOADER_DIR) bootloader.bin

$(LOADER_PATH): $(BOOTLOADER_BIN)
	@boot_size=$$(stat -f%z $(BOOTLOADER_BIN) 2>/dev/null || stat -c%s $(BOOTLOADER_BIN)); \
	$(MAKE) -C $(LOADER_DIR) VERSION=$(VERSION) \
		LOADER_BIN_ADDR=$(LOADER_BIN_ADDR) \
		BOOTLOADER_ROM_OFFSET=$(BOOTLOADER_ROM_OFFSET) \
		BOOTLOADER_ROM_SIZE=$$boot_size

$(BIOS_CFG): Makefile | $(BUILD)
	@echo '    .string    "HOLLYSH"' > $@
	@echo '    .align     4' >> $@
	@echo "    .long      $(BIOS_VER)" >> $@

$(LOADER_CFG): $(LOADER_PATH) | $(BUILD)
	@echo "LOADER_ROM_OFFSET:" > $@
	@echo "    .long        $(LOADER_ROM_OFFSET)" >> $@
	@echo "LOADER_BIN_ADDR:" >> $@
	@echo "    .long        $(LOADER_BIN_ADDR)" >> $@
	@echo "LOADER_SIZE:" >> $@
	@bytes=$$(stat -f%z $(LOADER_PATH) 2>/dev/null || stat -c%s $(LOADER_PATH)); \
	words=$$(expr \( $$bytes + 3 \) / 4); \
	echo "    .long        $$words" >> $@

$(BIOS_ELF): $(BIOS_CFG) $(LOADER_CFG) $(LOADER_PATH) $(BIOS_SRCS) | $(BUILD)
	$(KOS_CC) -fpic -I$(BUILD) $(BIOS_SRCS) -o $@ -Wl,-Ttext=0x00000000 \
		-nostartfiles -nostdlib -nodefaultlibs -e _sg_bios_start

ifneq ($(strip $(ROMDISK_CONTENT)),)
$(ROMDISK_IMG): $(ROMDISK_CONTENT) | $(BUILD)
	$(KOS_GENROMFS) -f $@ -d $(ROMDISK_DIR) -V DreamShell -v -x .keepme
endif

$(BIOS_BASE_BIN): $(BIOS_ELF) | $(BUILD)
	$(KOS_OBJCOPY) -O binary -R .stack $< $@

$(BIOS_LOADER_BIN): $(BIOS_BASE_BIN) $(LOADER_PATH) | $(BUILD)
	cp $< $@
	truncate -s $(LOADER_ROM_OFFSET) $@
	cat $(LOADER_PATH) >> $@

$(BIOS_BOOTLOADER_BIN): $(BIOS_LOADER_BIN) $(BOOTLOADER_BIN) | $(BUILD)
	cp $< $@
	truncate -s $(BOOTLOADER_ROM_OFFSET) $@
	@boot_bytes=$$(stat -f%z $(BOOTLOADER_BIN) 2>/dev/null || stat -c%s $(BOOTLOADER_BIN)); \
	boot_slot=$$(expr $(BIOS_ROMDISK_ROM_OFFSET) - $(BOOTLOADER_ROM_OFFSET)); \
	if [ $$boot_bytes -gt $$boot_slot ]; then \
		echo "Bootloader overflow: $$boot_bytes bytes, max $$boot_slot"; \
		exit 1; \
	fi; \
	cat $(BOOTLOADER_BIN) >> $@; \
	truncate -s $(BIOS_ROMDISK_ROM_OFFSET) $@

ifneq ($(strip $(ROMDISK_CONTENT)),)
BIOS_BEFORE_FONT = $(BIOS_ROMDISK_BIN)

$(BIOS_ROMDISK_BIN): $(BIOS_BOOTLOADER_BIN) $(ROMDISK_IMG) | $(BUILD)
	@offset=$$(expr $(BIOS_ROMDISK_ROM_OFFSET) + $$(stat -f%z $(ROMDISK_IMG) 2>/dev/null || stat -c%s $(ROMDISK_IMG))); \
	if [ $$offset -gt $(TARGET_FONT_OFFSET) ]; then \
		echo "Romdisk overflow: $$offset bytes, max $(TARGET_FONT_OFFSET)"; \
		exit 1; \
	fi; \
	cp $< $@; \
	truncate -s $(BIOS_ROMDISK_ROM_OFFSET) $@; \
	cat $(ROMDISK_IMG) >> $@
else
BIOS_BEFORE_FONT = $(BIOS_BOOTLOADER_BIN)
endif

$(FONT_BIN): $(FONT_SRC_FILE) | $(BUILD)
	tail -c +$(FONT_SRC_OFFSET) $< >$(BUILD)/tmpfont.bin
	tail -c +2 $(BUILD)/tmpfont.bin >$@
	rm -f $(BUILD)/tmpfont.bin
	truncate -s $(FONT_SRC_SIZE) $@

$(TARGET_BIN): $(BIOS_BEFORE_FONT) $(FONT_BIN) | $(BUILD)
	cp $(BIOS_BEFORE_FONT) $@
	truncate -s $(TARGET_FONT_OFFSET) $@
	cat $(FONT_BIN) >> $@
	truncate -s 2M $@

install: $(TARGET_BIN)
	-mkdir -p $(FW_DIR)
	cp $(TARGET_BIN) $(FW_DIR)/$(TARGET_NAME)

bios_clean:
	rm -rf $(BUILD)

clean: bios_clean
	$(MAKE) -C $(LOADER_DIR) clean

.PHONY: all romdisk install clean bios_clean
