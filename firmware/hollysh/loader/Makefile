#
# DreamShell HollySH BIOS loader
# (c) 2025-2026 SWAT <http://www.dc-swat.ru>
#

TARGET = hollysh_loader

BUILD = build

VER_MAJOR ?= 1
VER_MINOR ?= 0
VER_MICRO ?= 0
VERSION ?= $(VER_MAJOR).$(VER_MINOR).$(VER_MICRO)

LOADER_BIN_ADDR ?= 0x8c004000
BOOTLOADER_ROM_OFFSET ?= 0
BOOTLOADER_ROM_SIZE ?= 0

TARGETPREFIX = sh-elf
TARGETCC = $(TARGETPREFIX)-gcc
TARGETOBJCOPY = $(TARGETPREFIX)-objcopy
TARGETAS = $(TARGETPREFIX)-as
TARGETSIZE = $(TARGETPREFIX)-size

TARGETLDFLAGS = -Wl,--gc-sections -Wl,--no-warn-rwx-segments -T$(DRV_SRC_DIR)/shlelf.xc \
				-Ttext=$(LOADER_BIN_ADDR) -nostartfiles -nostdlib -nodefaultlibs

TARGETCFLAGS = -ml -m4-single-only -ffunction-sections -fdata-sections -ffreestanding \
				-fno-builtin -fno-strict-aliasing -fomit-frame-pointer \
				-freorder-blocks-algorithm=simple -flto=auto \
				-Wall -std=c17 -Wextra -Werror \
				-DVERSION=\"$(VERSION)\" \
				-DLOADER_BIN_ADDR=$(LOADER_BIN_ADDR) \
				-DBOOTLOADER_ROM_OFFSET=$(BOOTLOADER_ROM_OFFSET) \
				-DBOOTLOADER_ROM_SIZE=$(BOOTLOADER_ROM_SIZE) \
				-DDEV_TYPE_IDE=1 \
				-DDEV_TYPE_SD=1 \
				-D_FS_READONLY=1 \
				-DMAX_OPEN_FILES=1 \
				-DLOG=1

DRV_SRC_DIR = ../../isoldr/loader

INCLUDES = -I./include -I$(DRV_SRC_DIR)/include -I$(DRV_SRC_DIR)/kos \
		   -I$(DRV_SRC_DIR)/fs/fat/include -I$(DRV_SRC_DIR)/dev \
		   -I../../../include

CFLAGS = $(TARGETCFLAGS) $(INCLUDES) -Os
LIBS = -lgcc

SRCS = \
	src/main.c src/log.c src/malloc.c \
	$(DRV_SRC_DIR)/bfont_syscall.s \
	$(DRV_SRC_DIR)/kos/src/memcpy.S \
	$(DRV_SRC_DIR)/kos/src/memset.S \
	$(DRV_SRC_DIR)/kos/src/memcmp.c \
	$(DRV_SRC_DIR)/kos/src/strlen.S \
	$(DRV_SRC_DIR)/kos/src/timer.c \
	$(DRV_SRC_DIR)/kos/src/cache.s \
	$(DRV_SRC_DIR)/kos/src/biosfont.c \
	$(DRV_SRC_DIR)/kos/src/printf.c \
	$(DRV_SRC_DIR)/kos/src/scif.c \
	$(DRV_SRC_DIR)/kos/src/dmac.c \
	$(DRV_SRC_DIR)/kos/src/math.s \
	$(DRV_SRC_DIR)/fs/fat/src/ff.c \
	$(DRV_SRC_DIR)/fs/fat/src/option/ccsbcs.c \
	$(DRV_SRC_DIR)/fs/fat/src/option/syscall.c \
	$(DRV_SRC_DIR)/fs/fat/src/diskio.c \
	$(DRV_SRC_DIR)/fs/fat/fs.c \
	$(DRV_SRC_DIR)/dev/ide/ide.c \
	$(DRV_SRC_DIR)/dev/sd/sd.c \
	$(DRV_SRC_DIR)/dev/sd/spi.c \
	$(DRV_SRC_DIR)/dev/sd/sci.c

C_SRCS = $(filter %.c,$(SRCS))
S_SRCS = $(filter %.s,$(SRCS))
S_UP_SRCS = $(filter %.S,$(SRCS))

OBJS = $(BUILD)/startup.os \
	$(addprefix $(BUILD)/,$(notdir $(C_SRCS:.c=.o))) \
	$(addprefix $(BUILD)/,$(notdir $(S_SRCS:.s=.o))) \
	$(addprefix $(BUILD)/,$(notdir $(S_UP_SRCS:.S=.o)))

VPATH = src $(sort $(dir $(filter-out src/%,$(SRCS))))

all: prepare build

prepare:
	mkdir -p $(BUILD)

build: $(BUILD)/$(TARGET).bin

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(TARGETCC) $(TARGETCFLAGS) $(TARGETLDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(TARGETSIZE) $@

clean:
	rm -rf $(BUILD)

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(TARGETOBJCOPY) -O binary -R .stack $< $@

$(BUILD)/startup.os: $(DRV_SRC_DIR)/startup.s
	@mkdir -p $(BUILD)
	$(TARGETAS) --isa=sh4 -little -o $@ -c $<

$(BUILD)/ff.o: $(DRV_SRC_DIR)/fs/fat/src/ff.c
	@mkdir -p $(BUILD)
	$(TARGETCC) $(CFLAGS) -fno-lto -c $< -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(BUILD)
	$(TARGETCC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(BUILD)
	$(TARGETCC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(BUILD)
	$(TARGETCC) $(CFLAGS) -c $< -o $@

.PHONY: all prepare build clean
