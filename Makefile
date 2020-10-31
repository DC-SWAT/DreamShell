#
# DreamShell Makefile
# Copyright (C) 2004-2020 SWAT
# http://www.dc-swat.ru
#
# This makefile can build CDI image (type "make cdi"),
# launch DS on DC through dc-tool-ip (type "make run")
# or launch DS on emulators nullDC and lxdream 
# (type "make nulldc" or "make lxdream").
# Scroll down to see other options.
#

TARGET = DS
TARGET_BIN = $(TARGET)_CORE.BIN
TARGET_BIN_CD = 1$(TARGET_BIN)
TRAGET_VERSION = -DVER_MAJOR=4 -DVER_MINOR=0 -DVER_MICRO=0 -DVER_BUILD=0x25 #RC 5

all: rm-elf $(TARGET)

include sdk/Makefile.cfg

INC_DIR = ./include
SRC_DIR = ./src
LIB_DIR = ./lib
RES_DIR = ./resources

KOS_LDFLAGS += -L$(LIB_DIR)
KOS_CFLAGS += -I$(INC_DIR) -I$(INC_DIR)/SDL -I$(INC_DIR)/fatfs -I$(INC_DIR)/ntfs \
			-DHAVE_SDLIMAGE $(TRAGET_VERSION) #-DEMU=1

SDL_VER = 1.2.13
SDL_GFX_VER = 2.0.25
SDL_IMAGE_VER = 1.2.12
SDL_TTF_VER = 2.0.11
SDL_RTF_VER = 0.1.1
LUA_VER = 5.1.4-2

EXTRA_LIBS = -lcfg -lmxml -lparallax

SDL_LIBS = -lSDL_$(SDL_VER) \
			-lSDL_image_$(SDL_IMAGE_VER) \
			-lSDL_ttf_$(SDL_TTF_VER) \
			-lSDL_rtf_$(SDL_RTF_VER) \
			-lSDL_gfx_$(SDL_GFX_VER) \
			-lfreetype
			 
IMAGE_LIBS = -lkmg -ljpeg -lpng -lz
LUA_LIBS = -llua_$(LUA_VER)
KLIBS = -lkosext2fs -lkosutils -lstdc++ -lm

CORE_LIBS = $(EXTRA_LIBS) $(SDL_LIBS) $(IMAGE_LIBS) $(LUA_LIBS) $(KLIBS)

SDL_GUI = $(LIB_DIR)/SDL_gui
SDL_CONSOLE = $(LIB_DIR)/SDL_Console/src

GUI_OBJS = $(SDL_GUI)/SDL_gui.o $(SDL_GUI)/Exception.o $(SDL_GUI)/Object.o \
			$(SDL_GUI)/Surface.o $(SDL_GUI)/Font.o $(SDL_GUI)/Callback.o \
			$(SDL_GUI)/Drawable.o $(SDL_GUI)/Screen.o $(SDL_GUI)/Widget.o \
			$(SDL_GUI)/Container.o $(SDL_GUI)/FastFont.o $(SDL_GUI)/TrueTypeFont.o \
			$(SDL_GUI)/Layout.o $(SDL_GUI)/Panel.o $(SDL_GUI)/CardStack.o \
			$(SDL_GUI)/FastLabel.o $(SDL_GUI)/Label.o $(SDL_GUI)/Picture.o \
			$(SDL_GUI)/TextEntry.o $(SDL_GUI)/AbstractButton.o $(SDL_GUI)/Button.o \
			$(SDL_GUI)/ToggleButton.o $(SDL_GUI)/ProgressBar.o $(SDL_GUI)/ScrollBar.o \
			$(SDL_GUI)/AbstractTable.o $(SDL_GUI)/ListBox.o $(SDL_GUI)/VBoxLayout.o \
			$(SDL_GUI)/RealScreen.o $(SDL_GUI)/ScrollPanel.o $(SDL_GUI)/RTF.o \
			$(SDL_GUI)/Window.o $(SDL_GUI)/FileManager.o

CONSOLE_OBJ = $(SDL_CONSOLE)/SDL_console.o $(SDL_CONSOLE)/DT_drawtext.o \
				$(SDL_CONSOLE)/internal.o

DRIVERS_OBJ = $(SRC_DIR)/drivers/spi.o $(SRC_DIR)/drivers/sd.o \
				$(SRC_DIR)/drivers/enc28j60.o $(SRC_DIR)/drivers/asic.o \
				$(SRC_DIR)/drivers/rtc.o

FATFS_DIR = $(SRC_DIR)/fs/fat
FATFS = $(FATFS_DIR)/utils.o $(FATFS_DIR)/option/ccsbcs.o \
		 $(FATFS_DIR)/option/syscall.o $(FATFS_DIR)/ff.o \
		 $(FATFS_DIR)/dc.o

NTFS_DIR = $(SRC_DIR)/fs/ntfs
NTFS = $(NTFS_DIR)/attrib.o $(NTFS_DIR)/attrlist.o	\
		$(NTFS_DIR)/bitmap.o $(NTFS_DIR)/bootsect.o	\
		$(NTFS_DIR)/collate.o $(NTFS_DIR)/compat.o		\
		$(NTFS_DIR)/compress.o $(NTFS_DIR)/crypto.o	\
		$(NTFS_DIR)/debug.o $(NTFS_DIR)/device.o		\
		$(NTFS_DIR)/device_io.o $(NTFS_DIR)/dir.o		\
		$(NTFS_DIR)/index.o $(NTFS_DIR)/inode.o		\
		$(NTFS_DIR)/lcnalloc.o $(NTFS_DIR)/misc.o		\
		$(NTFS_DIR)/mft.o	$(NTFS_DIR)/mst.o				\
		$(NTFS_DIR)/runlist.o $(NTFS_DIR)/security.o	\
		$(NTFS_DIR)/unistr.o $(NTFS_DIR)/version.o		\
		$(NTFS_DIR)/volume.o #TODO $(NTFS_DIR)/dc_io.o
	
UTILS_DIR = $(SRC_DIR)/utils
UTILS_OBJ = $(SRC_DIR)/utils.o $(UTILS_DIR)/gmtime.o $(UTILS_DIR)/strftime.o \
			$(UTILS_DIR)/debug_console.o $(UTILS_DIR)/memcpy.op \
			$(UTILS_DIR)/memset.op $(UTILS_DIR)/memmove.op

OBJS = $(SRC_DIR)/main.o $(SRC_DIR)/video.o $(SRC_DIR)/console.o \
		$(SRC_DIR)/gui/gui.o $(SRC_DIR)/commands.o \
		$(SRC_DIR)/module.o $(SRC_DIR)/events.o $(SRC_DIR)/fs/fs.o  \
		$(SRC_DIR)/lua/lua.o $(SRC_DIR)/lua/lua_ds.o $(SRC_DIR)/lua/packlib.o \
		$(SRC_DIR)/app/app.o $(SRC_DIR)/app/load.o $(SRC_DIR)/list.o \
		$(SRC_DIR)/img/pvr.o $(SRC_DIR)/cmd_elf.o $(SRC_DIR)/vmu/vmu.o \
		$(SRC_DIR)/irq/exceptions.o $(SRC_DIR)/irq/setjmp.o \
		$(SRC_DIR)/settings.o $(DRIVERS_OBJ) $(GUI_OBJS) $(CONSOLE_OBJ) \
		$(UTILS_OBJ) $(FATFS) $(SRC_DIR)/exports.o $(SRC_DIR)/exports_gcc.o \
		romdisk.o #$(NTFS)

%.op: %.S
	kos-cc $(CFLAGS) -c $< -o $@

$(SRC_DIR)/exports.o: $(SRC_DIR)/exports.c
$(SRC_DIR)/exports.c: exports.txt
	$(KOS_BASE)/utils/genexports/genexports.sh exports.txt $(SRC_DIR)/exports.c ds_symtab
	$(KOS_BASE)/utils/genexports/genexportstubs.sh exports.txt $(SRC_DIR)/exports_stubs.c
	$(KOS_MAKE) -f Makefile.stubs

$(SRC_DIR)/exports_gcc.o: $(SRC_DIR)/exports_gcc.c
$(SRC_DIR)/exports_gcc.c: exports_gcc.txt
	$(KOS_BASE)/utils/genexports/genexports.sh exports_gcc.txt $(SRC_DIR)/exports_gcc.c gcc_symtab
	$(KOS_BASE)/utils/genexports/genexportstubs.sh exports_gcc.txt $(SRC_DIR)/exports_gcc_stubs.c
	$(KOS_MAKE) -f Makefile.gcc_stubs
	
romdisk.img: romdisk/logo.kmg.gz
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

logo: romdisk/logo.kmg.gz
romdisk/logo.kmg.gz: $(RES_DIR)/logo_sq.png
	$(KOS_BASE)/utils/kmgenc/kmgenc -v $(RES_DIR)/logo_sq.png
	mv $(RES_DIR)/logo_sq.kmg logo.kmg
	gzip -9 logo.kmg
	mv logo.kmg.gz romdisk/logo.kmg.gz

$(TARGET): $(TARGET_BIN)

$(TARGET).elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET).elf \
		$(OBJS) $(OBJEXTRA) $(CORE_LIBS) $(KOS_LIBS)

$(TARGET_BIN): $(TARGET).elf
	@echo Copy elf for debug...
	@cp $(TARGET).elf $(TARGET)-DBG.elf
	@echo Strip target...
	@$(KOS_STRIP) $(TARGET).elf
	@echo Creating binary file...
	@$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf $(TARGET_BIN)

$(TARGET_BIN_CD): $(TARGET_BIN)
	@echo Scramble binary file...
	@./sdk/bin/scramble $(TARGET_BIN) $(TARGET_BIN_CD)

cdi: $(TARGET).cdi

nulldc: $(TARGET).cdi
	@echo Running DreamShell...
	@./emu/nullDC.exe -serial "debug.log"

nulldcl: $(TARGET).cdi
	@echo Running DreamShell with log...
	@run ./emu/nullDC.exe -serial "debug.log" > emu.log
	
run: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ip -c $(DS_BUILD) -t $(DC_LAN_IP) -x $(TARGET).elf

run-serial: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ser -c $(DS_BUILD) -t $(DC_SERIAL_PORT) -b $(DC_SERIAL_BAUD) -x $(TARGET).elf
	
debug: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ip -g -c $(DS_BUILD) -t $(DC_LAN_IP) -x $(TARGET).elf

debug-serial: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ser -g -c $(DS_BUILD) -t $(DC_SERIAL_PORT) -b $(DC_SERIAL_BAUD) -x $(TARGET).elf

gdb: $(TARGET)-DBG.elf
	$(KOS_CC_BASE)/bin/$(KOS_CC_PREFIX)-gdb $(TARGET)-DBG.elf --eval-command "target remote localhost:2159"

lxdream: $(TARGET).cdi
	lxdream -p $(TARGET).cdi
	
lxdelf: $(TARGET).elf
	lxdream -u -p -e $(TARGET).elf

lxdgdb:
	lxdream -g 2000 -n $(TARGET).cdi &
	sleep 2
	$(KOS_CC_BASE)/bin/$(KOS_CC_PREFIX)-gdb $(TARGET)-DBG.elf --eval-command "target remote localhost:2000"

dropbox: $(TARGET).elf
	@$(KOS_STRIP) $(TARGET).elf
	rm -f ~/Dropbox/Dev/DS/DS.elf
	cp $(TARGET).elf ~/Dropbox/Dev/DS/DS.elf

$(TARGET).cdi: $(TARGET_BIN_CD) $(DS_BUILD)/lua/startup.lua
	@echo Creating ISO...
	@-rm -f $(DS_BUILD)/$(TARGET_BIN)
	@-rm -f $(DS_BUILD)/$(TARGET_BIN_CD)
	@cp $(TARGET_BIN_CD) $(DS_BUILD)/$(TARGET_BIN_CD)
	@$(DS_SDK)/bin/mkisofs -V DreamShell -G $(RES_DIR)/IP.BIN -joliet -rock -l -x .svn -o $(TARGET).iso $(DS_BUILD)
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d > conv_log.txt
	@-rm -f conv_log.txt
	@-rm -f $(TARGET).iso

#@$(DS_SDK)/bin/mkisofs -V DreamShell -C 0,11702 -G $(RES_DIR)/IP.BIN -joliet -rock -l -x .svn -o $(TARGET).iso $(DS_BUILD)
#@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi > conv_log.txt

TARGET_CLEAN_BIN = 1$(TARGET)_CORE.BIN $(TARGET)_CORE.BIN $(TARGET).elf $(TARGET).cdi

clean:
	-rm -f $(TARGET_CLEAN_BIN) $(OBJS) $(SRC_DIR)/exports.c $(SRC_DIR)/exports_gcc.c

rm-elf:
	-rm -f $(TARGET_CLEAN_BIN) 
