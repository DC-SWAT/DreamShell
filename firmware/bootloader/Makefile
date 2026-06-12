#
# DreamShell bootloader
# (c)2011-2026 SWAT
#

TARGET = bootloader
VER_MAJOR = 2
VER_MINOR = 9
VER_MICRO = 0
VERSION = $(VER_MAJOR).$(VER_MINOR)
TARGET_NAME = DreamShell_$(TARGET)_v$(VERSION)
TARGET_CD = cd/1DS_BOOT.BIN
KOS_ROMDISK_DIR = romdisk

all: rm-elf $(TARGET).elf

clean: rm-elf
	-rm -f $(OBJS)
	-rm -rf ./cd

rm-elf:
	-rm -f $(TARGET).elf $(TARGET_NAME).elf $(TARGET).bin $(TARGET_NAME).bin \
		$(TARGET_CD) $(TARGET_NAME).cdi romdisk.*
	-rm -rf $(KOS_ROMDISK_DIR)/.* 2> /dev/null || true

include ../../sdk/Makefile.cfg

BIOS_SRC_DIR = $(DS_RES)/firmware/bios/ds
BIOS_PATCH_DIR = $(BIOS_SRC_DIR)/patches
FW_DIR = $(DS_BUILD)/firmware/bios/ds
BOOTLOADER_ROM_OFFSET = 65536
NOGDROM_PATCH_1_OFFSET = 4328
NOGDROM_PATCH_2_OFFSET = 4334
NOGDROM_PATCH_3_OFFSET = 4342
MB32_PATCH_1_OFFSET = 2093440
MB32_PATCH_2_OFFSET = 2093444
WIDESCREEN_PATCH_OFFSET = 970112
BIOS_INSTALL_SPECS = \
	"devkit bootloader_devkit.bios 0 0 0" \
	"devkit_widescreen bootloader_devkit.bios 0 0 1" \
	"devkit_nogdrom bootloader_devkit.bios 1 0 0" \
	"devkit_nogdrom_widescreen bootloader_devkit.bios 1 0 1" \
	"devkit_32mb bootloader_devkit.bios 0 1 0" \
	"devkit_32mb_widescreen bootloader_devkit.bios 0 1 1" \
	"devkit_nogdrom_32mb bootloader_devkit.bios 1 1 0" \
	"devkit_nogdrom_32mb_widescreen bootloader_devkit.bios 1 1 1" \
	"retail bootloader_retail.bios 0 0 0" \
	"retail_widescreen bootloader_retail.bios 0 0 1" \
	"retail_nogdrom bootloader_retail.bios 1 0 0" \
	"retail_nogdrom_widescreen bootloader_retail.bios 1 0 1" \
	"retail_32mb bootloader_retail.bios 0 1 0" \
	"retail_32mb_widescreen bootloader_retail.bios 0 1 1" \
	"retail_nogdrom_32mb bootloader_retail.bios 1 1 0" \
	"retail_nogdrom_32mb_widescreen bootloader_retail.bios 1 1 1"

UTILS = $(DS_BASE)/src/utils

OBJS = src/main.o src/spiral.o src/menu.o src/descramble.o \
		$(DS_BASE)/src/fs/fs.o \
		$(UTILS)/memcpy.op $(UTILS)/memset.op romdisk.o

KOS_CFLAGS += -I$(DS_BASE)/include -I$(DS_BASE)/lib/fatfs/include -I./include -DVERSION="$(VERSION)"

$(TARGET).bin: $(TARGET).elf
$(TARGET).elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET).elf \
		$(OBJS) $(OBJEXTRA) -lz -lkmg -lfatfs $(KOS_LIBS)
	$(KOS_STRIP) $(TARGET).elf
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf $(TARGET).bin

%.op: %.S
	kos-cc $(CFLAGS) -c $< -o $@

$(TARGET_CD): $(TARGET).bin

cdi: $(TARGET_CD)
	@echo Updating IP.BIN with current bootloader version and date...
	@../../utils/update_ip_bin.sh $(DS_RES)/IP.BIN /tmp/IP_$(TARGET).BIN $(VER_MAJOR) $(VER_MINOR) $(VER_MICRO) 0x30 bootloader >/dev/null
	@mkdir -p ./cd
	@$(KOS_BASE)/utils/scramble/scramble $(TARGET).bin $(TARGET_CD)
	@$(DS_SDK)/bin/mkisofs -V DreamShell -C 0,11702 -G /tmp/IP_$(TARGET).BIN -joliet -rock -l -x .DS_Store -o $(TARGET).iso ./cd
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi >/dev/null
	@-rm -f $(TARGET).iso
	@-rm -f /tmp/IP_$(TARGET).BIN

# If you have problems with mkisofs try data/data image:
# $(DS_SDK)/bin/mkisofs -V DreamShell -G $(DS_RES)/IP.BIN -joliet -rock -l -x .DS_Store -o $(TARGET).iso ./cd
# @$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d >/dev/null

release: all cdi install
	@rm -f $(TARGET_NAME).bin $(TARGET_NAME).cdi
	@cp $(TARGET).bin $(TARGET_NAME).bin
	@mv $(TARGET).cdi $(TARGET_NAME).cdi

install: $(TARGET).bin
	@mkdir -p $(FW_DIR)
	@set -e; \
	for spec in $(BIOS_INSTALL_SPECS); do \
		set -- $$spec; \
		out=$(FW_DIR)/$(TARGET)_$$1.bios; \
		cp $(BIOS_SRC_DIR)/$$2 $$out; \
		if [ $$3 = 1 ]; then \
			dd if=$(BIOS_PATCH_DIR)/nogdrom_1.bin of=$$out bs=1 seek=$(NOGDROM_PATCH_1_OFFSET) conv=notrunc status=none; \
			dd if=$(BIOS_PATCH_DIR)/nogdrom_2.bin of=$$out bs=1 seek=$(NOGDROM_PATCH_2_OFFSET) conv=notrunc status=none; \
			dd if=$(BIOS_PATCH_DIR)/nogdrom_3.bin of=$$out bs=1 seek=$(NOGDROM_PATCH_3_OFFSET) conv=notrunc status=none; \
		fi; \
		if [ $$4 = 1 ]; then \
			dd if=$(BIOS_PATCH_DIR)/32mb.bin of=$$out bs=1 seek=$(MB32_PATCH_1_OFFSET) conv=notrunc status=none; \
			dd if=$(BIOS_PATCH_DIR)/32mb.bin of=$$out bs=1 seek=$(MB32_PATCH_2_OFFSET) conv=notrunc status=none; \
		fi; \
		if [ $$5 = 1 ]; then \
			dd if=$(BIOS_PATCH_DIR)/widescreen.bin of=$$out bs=1 seek=$(WIDESCREEN_PATCH_OFFSET) conv=notrunc status=none; \
		fi; \
		dd if=$(TARGET).bin of=$$out bs=$(BOOTLOADER_ROM_OFFSET) seek=1 conv=notrunc status=none; \
		echo "Installed $$1"; \
	done

nulldc: all cdi
	@echo Running...
	@-rm -f $(DS_BASE)/DS.cdi
	@cp $(TARGET).cdi $(DS_BASE)/DS.cdi
	@run $(DS_BASE)/emu/nullDC.exe -serial "debug.log"

lxdream: all cdi
	@echo Running...
	@lxdream -p $(TARGET).cdi

lxdelf: $(TARGET).elf
	lxdream -u -p -e $(TARGET).elf

flycast: cdi
	Flycast -config config:Debug.SerialConsoleEnabled=yes

run: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ip -t $(DC_LAN_IP) -x $(TARGET).elf

run-serial: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ser -t $(DC_SERIAL_PORT) -b $(DC_SERIAL_BAUD) -x $(TARGET).elf
