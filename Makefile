#
# DreamShell Makefile
# Copyright (C) 2004-2025 SWAT
# http://www.dc-swat.ru
#
# This Makefile can build CDI image (type "make cdi"),
# launch DS on DC through dc-tool-ip (type "make run")
# or launch DS on emulators nullDC and lxdream 
# (type "make nulldc" or "make lxdream").
# Scroll down to see other options.
#

TARGET = DS
TARGET_NAME = DreamShell_v4.0.2_RC1
TARGET_BIN = $(TARGET)_CORE.BIN
TARGET_BIN_CD = 1$(TARGET_BIN)
# Build types: 0x0N - Alpha, 0x1N - Beta, 0x2N - RC, 0x3N - Release
TRAGET_VERSION = -DVER_MAJOR=4 \
				-DVER_MINOR=0 \
				-DVER_MICRO=2 \
				-DVER_BUILD=0x21
# TARGET_DEBUG = 1 # or 2 for GDB
# TARGET_EMU = 1
# TARGET_PROF = 1

all: rm-elf $(TARGET)

KOS_ROMDISK_DIR = romdisk

include sdk/Makefile.cfg

INC_DIR = $(DS_BASE)/include
SRC_DIR = $(DS_BASE)/src
LIB_DIR = $(DS_BASE)/lib

KOS_LDFLAGS += -L$(LIB_DIR)
KOS_CFLAGS += -I$(INC_DIR) -I$(INC_DIR)/img -I$(INC_DIR)/SDL \
			-I$(LIB_DIR)/fatfs/include \
			-I$(INC_DIR)/tsunami \
			-DHAVE_SDLIMAGE $(TRAGET_VERSION)

KOS_CPPFLAGS += -Wno-template-id-cdtor

ifdef TARGET_DEBUG
KOS_CFLAGS += -g -DDS_DEBUG=$(TARGET_DEBUG)
endif
ifdef TARGET_EMU
KOS_CFLAGS += -DDS_EMU=1
endif

SDL_VER = 1.2.13
SDL_GFX_VER = 2.0.25
SDL_IMAGE_VER = 1.2.12
SDL_TTF_VER = 2.0.11
SDL_RTF_VER = 0.1.1
LUA_VER = 5.1.4-2
TSUNAMI_VER = 2.0.0
PARALLAX_VER = 2.0.0
FREETYPE_VER = 2.4.4

EXTRA_LIBS = -lcfg -lmxml -lfatfs

SDL_LIBS = -lSDL_$(SDL_VER) \
			-lSDL_image_$(SDL_IMAGE_VER) \
			-lSDL_ttf_$(SDL_TTF_VER) \
			-lSDL_rtf_$(SDL_RTF_VER) \
			-lSDL_gfx_$(SDL_GFX_VER) \
			-lfreetype_$(FREETYPE_VER)

IMAGE_LIBS = -lkmg -ljpeg -lpng -lz
LUA_LIBS = -llua_$(LUA_VER)
KLIBS = -lpthread -lkosutils -lstdc++ -lm
GRAPHICS_LIBS = -ltsunami_$(TSUNAMI_VER) \
				-lparallax_$(PARALLAX_VER)

CORE_LIBS = $(EXTRA_LIBS) $(SDL_LIBS) $(GRAPHICS_LIBS) $(IMAGE_LIBS) $(LUA_LIBS) $(KLIBS)

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

DRIVERS_OBJ = $(SRC_DIR)/drivers/spi.o $(SRC_DIR)/drivers/enc28j60.o

UTILS_DIR = $(SRC_DIR)/utils
UTILS_OBJ = $(SRC_DIR)/utils.o $(UTILS_DIR)/gmtime.o $(UTILS_DIR)/strftime.o \
			$(UTILS_DIR)/debug_console.o $(UTILS_DIR)/memcpy.op \
			$(UTILS_DIR)/memset.op $(UTILS_DIR)/memmove.op

OBJS = $(SRC_DIR)/main.o $(SRC_DIR)/video.o $(SRC_DIR)/console.o \
		$(SRC_DIR)/gui/gui.o $(SRC_DIR)/commands.o \
		$(SRC_DIR)/module.o $(SRC_DIR)/events.o $(SRC_DIR)/fs/fs.o  \
		$(SRC_DIR)/lua/lua.o $(SRC_DIR)/lua/lua_ds.o $(SRC_DIR)/lua/packlib.o \
		$(SRC_DIR)/app/app.o $(SRC_DIR)/app/load.o $(SRC_DIR)/list.o \
		$(SRC_DIR)/img/SegaPVRImage.o $(SRC_DIR)/img/stb_image.o \
		$(SRC_DIR)/img/load.o  $(SRC_DIR)/img/utils.o \
		$(SRC_DIR)/img/decode.o $(SRC_DIR)/img/convert.o $(SRC_DIR)/img/copy.o  \
		$(SRC_DIR)/cmd_elf.o $(SRC_DIR)/vmu/vmu.o \
		$(SRC_DIR)/irq/exceptions.o $(SRC_DIR)/irq/setjmp.o \
		$(SRC_DIR)/settings.o $(SRC_DIR)/sfx.o \
		$(DRIVERS_OBJ) $(GUI_OBJS) $(CONSOLE_OBJ) \
		$(UTILS_OBJ) $(SRC_DIR)/exports.o $(SRC_DIR)/exports_gcc.o \
		romdisk.o

ifdef TARGET_PROF
	OBJS += $(SRC_DIR)/profiler.o
	KOS_CFLAGS += -DDS_PROF=$(TARGET_PROF)
endif

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

logo: $(KOS_ROMDISK_DIR)/logo.kmg.gz
$(KOS_ROMDISK_DIR)/logo.kmg.gz: $(DS_RES)/logo_sq.png
	$(KOS_BASE)/utils/kmgenc/kmgenc -v $(DS_RES)/logo_sq.png
	mv $(DS_RES)/logo_sq.kmg logo.kmg
	gzip -9 logo.kmg
	mv logo.kmg.gz $(KOS_ROMDISK_DIR)/logo.kmg.gz

sfx: $(KOS_ROMDISK_DIR)/startup.raw.gz
$(KOS_ROMDISK_DIR)/startup.raw.gz: $(DS_RES)/sfx/startup.wav
	ffmpeg -i $(DS_RES)/sfx/startup.wav -af "apad=pad_dur=2" $(DS_RES)/sfx/startup_pad.wav
	ffmpeg -i $(DS_RES)/sfx/startup_pad.wav -acodec adpcm_yamaha -fs 327680 -f s16le $(KOS_ROMDISK_DIR)/startup.raw
	rm -f $(DS_RES)/sfx/startup_pad.wav
	gzip -9 $(KOS_ROMDISK_DIR)/startup.raw

make-build: $(DS_BUILD)/lua/startup.lua

$(DS_BUILD)/lua/startup.lua: $(DS_RES)/lua/startup.lua
	@echo Creating build directory...
	@mkdir -p $(DS_BUILD)
	@mkdir -p $(DS_BUILD)/apps
	@mkdir -p $(DS_BUILD)/cmds
	@mkdir -p $(DS_BUILD)/modules
	@mkdir -p $(DS_BUILD)/screenshot
	@mkdir -p $(DS_BUILD)/vmu
	@mkdir -p $(DS_BUILD)/sfx
	ffmpeg -i $(DS_RES)/sfx/click.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/click.wav
	ffmpeg -i $(DS_RES)/sfx/click2.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/click2.wav
	ffmpeg -i $(DS_RES)/sfx/screenshot.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/screenshot.wav
	ffmpeg -i $(DS_RES)/sfx/move.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/move.wav
	ffmpeg -i $(DS_RES)/sfx/chpage.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/chpage.wav
	ffmpeg -i $(DS_RES)/sfx/slide.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/slide.wav
	ffmpeg -i $(DS_RES)/sfx/error.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/error.wav
	ffmpeg -i $(DS_RES)/sfx/success.wav -acodec adpcm_yamaha $(DS_BUILD)/sfx/success.wav
	@cp -R $(DS_RES)/doc $(DS_BUILD)
	@cp -R $(DS_RES)/firmware $(DS_BUILD)
	@cp -R $(DS_RES)/fonts $(DS_BUILD)
	@cp -R $(DS_RES)/gui $(DS_BUILD)
	@cp -R $(DS_RES)/lua $(DS_BUILD)
	@mkdir -p $(DS_BUILD)/firmware/aica
	@cp ../kernel/arch/dreamcast/sound/arm/stream.drv $(DS_BUILD)/firmware/aica/kos_stream.drv

libs: $(LIB_DIR)/libSDL_$(SDL_VER).a

$(LIB_DIR)/libSDL_$(SDL_VER).a:
	cd $(LIB_DIR) && make

build: $(TARGET)
	@echo Building modules, commands, applications and firmwares...
	cd $(DS_BASE)/modules && make
	cd $(DS_BASE)/commands && make
	cd $(DS_BASE)/applications && make
	cd $(DS_BASE)/firmware/isoldr && make && make install
	cd $(DS_BASE)/firmware/bootloader && make && make install
#   cd $(DS_BASE)/firmware/aica && make && make install

clean-all: clean
	@-rm -rf $(DS_BUILD)/.* 2> /dev/null
	@-rm -rf $(DS_BASE)/release
	@-rm -rf $(DS_BASE)/build
	@-rm -rf $(DS_SDK)/lib/*.a
	cd $(DS_BASE)/lib && make clean
	cd $(DS_BASE)/modules && make clean
	cd $(DS_BASE)/commands && make clean
	cd $(DS_BASE)/applications && make clean
	cd $(DS_BASE)/firmware/isoldr && make clean
	cd $(DS_BASE)/firmware/bootloader && make clean

release: build cdi
	@echo Creating a full release...
	@-rm -rf $(DS_BUILD)/.* 2> /dev/null || true
	@-rm -rf $(DS_BASE)/release || true
	@mkdir -p $(DS_BASE)/release/$(TARGET)
	@cp -R $(DS_BUILD)/* $(DS_BASE)/release/$(TARGET)
	@cp $(TARGET_BIN) $(DS_BASE)/release/$(TARGET)
	@cd $(DS_BASE)/firmware/bootloader && make && make release
	@mv $(DS_BASE)/firmware/bootloader/*.cdi $(DS_BASE)/release
	@mv $(TARGET).cdi $(DS_BASE)/release/$(TARGET_NAME).cdi
	@echo Compressing...
	@cd $(DS_BASE)/release && zip -q -r $(TARGET_NAME).zip * 2> /dev/null
	@echo 
	@echo "\033[42m Complete $(DS_BASE)/release \033[0m"

update:
	@echo Fetching DreamShell from GitHub...
	@git fetch && git checkout origin/master
	@git submodule update --init --recursive
	@echo Fetching KallistiOS from GitHub...
	@cd $(KOS_BASE) && git fetch && git checkout `cat $(DS_BASE)/sdk/doc/KallistiOS.txt`
	@echo Fetching kos-ports from GitHub...
	@cd $(KOS_BASE)/../kos-ports && git fetch && git checkout origin/master

update-build:
	@echo Fetching DreamShell from GitHub...
	@git fetch && git checkout origin/master
	@git submodule update --init --recursive
	@echo Fetching and build KallistiOS from GitHub...
	@cd $(KOS_BASE) && git fetch && git checkout `cat $(DS_BASE)/sdk/doc/KallistiOS.txt` && make clean && make
	@echo Fetching and build kos-ports from GitHub...
	@mv include include_
	@cd $(KOS_BASE)/../kos-ports && git fetch && git checkout origin/master && ./utils/build-all.sh
	@mv include_ include
	@echo Building DreamShell...
	@make clean-all && make build

toolchain:
	@cp $(DS_SDK)/toolchain/environ.sh $(KOS_BASE)/environ.sh
	@cp $(DS_SDK)/toolchain/patches/*.diff $(KOS_BASE)/utils/dc-chain/patches
	@source $(KOS_BASE)/environ.sh
	@cd $(KOS_BASE)/utils/dc-chain && cp Makefile.default.cfg Makefile.cfg && make

$(TARGET): libs $(TARGET_BIN) make-build

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

$(TARGET).cdi: $(TARGET_BIN_CD) make-build
	@echo Creating ISO...
	@-rm -f $(DS_BUILD)/$(TARGET_BIN)
	@-rm -f $(DS_BUILD)/$(TARGET_BIN_CD)
	@cp $(TARGET_BIN_CD) $(DS_BUILD)/$(TARGET_BIN_CD)
	@-rm -rf $(DS_BUILD)/.* 2> /dev/null
	@$(DS_SDK)/bin/mkisofs -V DreamShell -C 0,11702 -G $(DS_RES)/IP.BIN -joliet -rock -l -x .DS_Store -o $(TARGET).iso $(DS_BUILD)
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi >/dev/null
	@-rm -f $(TARGET).iso
	@-rm -f $(DS_BUILD)/$(TARGET_BIN_CD)

# If you have problems with mkisofs try data/data image:
# $(DS_SDK)/bin/mkisofs -V DreamShell -G $(DS_RES)/IP.BIN -joliet -rock -l -x .DS_Store -o $(TARGET).iso $(DS_BUILD)
# @$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d >/dev/null

nulldc: $(TARGET).cdi
	@echo Running DreamShell...
	@./emu/nullDC.exe -serial "debug.log"

nulldcl: $(TARGET).cdi
	@echo Running DreamShell with log...
	@run ./emu/nullDC.exe -serial "debug.log" > emu.log
	
run: $(TARGET).elf
	sudo $(DS_SDK)/bin/dc-tool-ip -c $(DS_BUILD) -t $(DC_LAN_IP) -x $(TARGET).elf

run-ns: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool-ip -t $(DC_LAN_IP) -x $(TARGET).elf

run-serial: $(TARGET).elf
	sudo $(DS_SDK)/bin/dc-tool-ser -c $(DS_BUILD) -t $(DC_SERIAL_PORT) -b $(DC_SERIAL_BAUD) -x $(TARGET).elf
	
debug: $(TARGET).elf
	sudo $(DS_SDK)/bin/dc-tool-ip -g -c $(DS_BUILD) -t $(DC_LAN_IP) -x $(TARGET).elf

debug-serial: $(TARGET).elf
	sudo $(DS_SDK)/bin/dc-tool-ser -g -c $(DS_BUILD) -t $(DC_SERIAL_PORT) -b $(DC_SERIAL_BAUD) -x $(TARGET).elf

gdb: $(TARGET)-DBG.elf
	$(KOS_CC_BASE)/bin/$(KOS_CC_PREFIX)-gdb $(TARGET)-DBG.elf --eval-command "target remote localhost:2159"

lxdream: $(TARGET).cdi
	lxdream -d -p $(TARGET).cdi
	
lxdelf: $(TARGET).elf
	lxdream -u -p -e $(TARGET).elf

lxdgdb: $(TARGET).cdi
	lxdream -g 2000 -n $(TARGET).cdi &
	sleep 2
	$(KOS_CC_BASE)/bin/$(KOS_CC_PREFIX)-gdb $(TARGET)-DBG.elf --eval-command "target remote localhost:2000"

flycast: $(TARGET).cdi
	Flycast -config config:Debug.SerialConsoleEnabled=yes ./$(TARGET).cdi

gprof:
	@sh-elf-gprof $(TARGET)-DBG.elf $(DS_BUILD)/kernel_gmon.out > gprof.out
	@cat gprof.out | gprof2dot | dot -Tpng -o $(TARGET)-kernel.png
	@sh-elf-gprof $(TARGET)-DBG.elf $(DS_BUILD)/video_gmon.out > gprof.out
	@cat gprof.out | gprof2dot | dot -Tpng -o $(TARGET)-video.png
	@-rm -rf gprof.out
	@echo "\033[42m Profiling data saved to $(TARGET)-*.png \033[0m"

core: $(TARGET_BIN)
	cp $(DS_BASE)/$(TARGET_BIN) $(DS_BUILD)

TARGET_CLEAN_BIN = 1$(TARGET)_CORE.BIN $(TARGET)_CORE.BIN $(TARGET).elf $(TARGET).cdi

clean:
	-rm -f $(TARGET_CLEAN_BIN) $(OBJS) $(SRC_DIR)/exports.c $(SRC_DIR)/exports_gcc.c

rm-elf:
	-rm -f $(TARGET_CLEAN_BIN)
	-rm -f $(SRC_DIR)/main.o
	-rm -f romdisk.*
	-rm -rf $(KOS_ROMDISK_DIR)/.* 2> /dev/null || true
