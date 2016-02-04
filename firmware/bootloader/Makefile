#
# DreamShell boot loader
# (c)2011-2016 SWAT
#

TARGET = loader
VERSION = 2.4
TARGET_NAME = DreamShell_boot_$(TARGET)_v$(VERSION)
TARGET_CD = cd/1DS_BOOT.BIN

all: rm-elf $(TARGET).elf

clean: rm-elf
	-rm -f $(OBJS)

rm-elf:
	-rm -f $(TARGET).elf $(TARGET).bin $(TARGET_CD) romdisk.*

include ../../sdk/Makefile.cfg

FATFS = $(DS_BASE)/src/fs/fat
DRIVERS = $(DS_BASE)/src/drivers
UTILS = $(DS_BASE)/src/utils

OBJS = src/main.o src/spiral.o src/menu.o src/descramble.o \
		$(DRIVERS)/spi.o $(DRIVERS)/sd.o $(DRIVERS)/rtc.o \
		$(FATFS)/utils.o $(FATFS)/option/ccsbcs.o $(FATFS)/option/syscall.o \
		$(FATFS)/ff.o $(FATFS)/dc.o $(FATFS)/../fs.o \
		$(UTILS)/memcpy.op $(UTILS)/memset.op

KOS_CFLAGS += -I$(DS_BASE)/include -I$(DS_BASE)/include/fatfs -I./include -DVERSION="$(VERSION)"

$(TARGET).bin: $(TARGET).elf
$(TARGET).elf: $(OBJS) romdisk.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET).elf \
		$(OBJS) romdisk.o $(OBJEXTRA) -lkosext2fs -lz -lkmg $(KOS_LIBS)
	$(KOS_STRIP) $(TARGET).elf
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf $(TARGET).bin
	
%.op: %.S
	kos-cc $(CFLAGS) -c $< -o $@
		
romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

$(TARGET_CD): $(TARGET).bin

cdi: $(TARGET_CD)
	@mkdir -p ./cd
	@$(DS_SDK)/bin/scramble $(TARGET).bin $(TARGET_CD)
	@$(DS_SDK)/bin/mkisofs -V DreamShell -G res/IP.BIN -joliet -rock -l -o $(TARGET).iso ./cd
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d > cdi4dc.out
	@-rm -f $(TARGET).iso

#@$(DS_SDK)/bin/mkisofs -V DreamShell -C 0,11702 -G res/IP.BIN -joliet -rock -l -o $(TARGET).iso ./cd
#@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi > cdi4dc.out

release: all cdi
	rm -f $(TARGET_NAME).elf $(TARGET_NAME).bin $(TARGET_NAME).cdi
	mv $(TARGET).elf $(TARGET_NAME).elf
	mv $(TARGET).bin $(TARGET_NAME).bin
	mv $(TARGET).cdi $(TARGET_NAME).cdi

nulldc: all cdi
	@echo Running...
	@-rm -f $(DS_BASE)/DS.cdi
	@cp $(TARGET).cdi $(DS_BASE)/DS.cdi
	@run $(DS_BASE)/emu/nullDC.exe -serial "debug.log"

lxdream: all cdi
	@echo Running...
	@lxdream -p $(TARGET).cdi
	
lxelf: $(TARGET).elf
	lxdream -u -p -e $(TARGET).elf
	
run: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool -t $(DC_IP) -x $(TARGET).elf
