#
# DreamShell ISO Loader (low-level version)
# Copyright (C) 2009-2023 SWAT
# Copyright (C) 2019 megavolt85
#

TARGET = 0x8c000000

SYSCALLOBJS = syscalls.o flash_font_sys.o syscallsc.o
SYSCALLBIN = syscalls

TARGETPREFIX	= sh-elf
TARGETCC		= $(TARGETPREFIX)-gcc
TARGETOBJCOPY	= $(TARGETPREFIX)-objcopy
TARGETLD		= $(TARGETPREFIX)-ld
TARGETAS		= $(TARGETPREFIX)-as
TARGETSIZE		= $(TARGETPREFIX)-size
TARGETLDFLAGS	= -Wl,--gc-sections -Tshlelf.xc -nostartfiles -nostdlib -nodefaultlibs
TARGETCFLAGS	= -ml -m4-single -ffunction-sections -fdata-sections -ffreestanding \
					-fno-builtin -fno-strict-aliasing -fomit-frame-pointer \
					-Wall -std=c17 -Wextra -Werror

# TARGETCFLAGS += -DLOG
# SYSCALLOBJS += log.o memcpy.o

TARGETCFLAGS += -I./include
LIBS = -lgcc

INSTALL_PATH = ../../../build/firmware/isoldr

%.bin: %.elf
	$(TARGETOBJCOPY) -O binary $< $@
%.o: %.c
	$(TARGETCC) $(TARGETCFLAGS) -Os $(INCLUDE) -c $< -o $@
%.o: %.s
	$(TARGETCC) $(TARGETCFLAGS) $(INCLUDE) -o $@ -c $<
%.o: %.S
	$(TARGETCC) $(TARGETCFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(SYSCALLBIN).bin

clean:
	rm -f $(SYSCALLOBJS) $(SYSCALLBIN).elf $(SYSCALLBIN).bin

$(SYSCALLBIN).elf: $(SYSCALLOBJS)
	$(TARGETCC) $(TARGETCFLAGS) $(TARGETLDFLAGS) -o $(SYSCALLBIN).elf $(SYSCALLOBJS) $(LIBS)

install: $(SYSCALLBIN).bin
	-mkdir -p $(INSTALL_PATH)
	cp $(SYSCALLBIN).bin $(INSTALL_PATH)
