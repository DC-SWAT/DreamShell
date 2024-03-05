#
# DreamShell boot loader
# (c)2011-2024 SWAT
#

TARGET = bootloader
VERSION = 2.6
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

FATFS = $(DS_BASE)/src/fs/fat
DRIVERS = $(DS_BASE)/src/drivers
UTILS = $(DS_BASE)/src/utils

OBJS = src/main.o src/spiral.o src/menu.o src/descramble.o \
		$(DRIVERS)/spi.o $(DRIVERS)/sd.o $(DRIVERS)/rtc.o \
		$(FATFS)/utils.o $(FATFS)/option/ccsbcs.o $(FATFS)/option/syscall.o \
		$(FATFS)/ff.o $(FATFS)/dc.o $(FATFS)/../fs.o \
		$(UTILS)/memcpy.op $(UTILS)/memset.op romdisk.o

KOS_CFLAGS += -I$(DS_BASE)/include -I$(DS_BASE)/include/fatfs -I./include -DVERSION="$(VERSION)"

$(TARGET).bin: $(TARGET).elf
$(TARGET).elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET).elf \
		$(OBJS) $(OBJEXTRA) -lkosext2fs -lz -lkmg $(KOS_LIBS)
	$(KOS_STRIP) $(TARGET).elf
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf $(TARGET).bin

%.op: %.S
	kos-cc $(CFLAGS) -c $< -o $@

$(TARGET_CD): $(TARGET).bin

cdi: $(TARGET_CD)
	@mkdir -p ./cd
	@$(DS_SDK)/bin/scramble $(TARGET).bin $(TARGET_CD)
	@$(DS_SDK)/bin/mkisofs -V DreamShell -C 0,11702 -G $(DS_RES)/IP.BIN -joliet -rock -l -x .DS_Store -o $(TARGET).iso ./cd
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi >/dev/null
	@-rm -f $(TARGET).iso

# If you have problems with mkisofs try data/data image:
# $(DS_SDK)/bin/mkisofs -V DreamShell -G $(DS_RES)/IP.BIN -joliet -rock -l -x .DS_Store -o $(TARGET).iso ./cd
# @$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d >/dev/null

release: all cdi install
	@rm -f $(TARGET_NAME).bin $(TARGET_NAME).cdi
	@cp $(TARGET).bin $(TARGET_NAME).bin
	@mv $(TARGET).cdi $(TARGET_NAME).cdi

install: $(TARGET).bin
	@$(DS_SDK)/bin/mkbios $(DS_BUILD)/firmware/bios/ds/boot_loader_devkit_nogdrom.bios $(TARGET).bin
	@$(DS_SDK)/bin/mkbios $(DS_BUILD)/firmware/bios/ds/boot_loader_devkit.bios $(TARGET).bin
	@$(DS_SDK)/bin/mkbios $(DS_BUILD)/firmware/bios/ds/boot_loader_retail_nogdrom.bios $(TARGET).bin
	@$(DS_SDK)/bin/mkbios $(DS_BUILD)/firmware/bios/ds/boot_loader_retail.bios $(TARGET).bin

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
