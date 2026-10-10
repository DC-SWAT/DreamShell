#
# DreamShell MIE firmware
# Build the Z80 image
# (c)2026 SWAT <http://www.dc-swat.ru>
#

include ../../sdk/Makefile.cfg

SDCC ?= sdcc
SDAS ?= sdasz80

BUILD := build
IMAGE := $(BUILD)/mie_z80.bin
INSTALL_BIN := $(DS_RES)/firmware/mie_z80.bin

MIE_SERIAL ?= 0
MIE_JVS_EXTRA ?= 0

CFLAGS := -mz80 --std-sdcc11 -c --opt-code-size --fomit-frame-pointer -Iinclude
ifneq ($(MIE_SERIAL),0)
CFLAGS += -DMIE_SERIAL
endif
ifneq ($(MIE_JVS_EXTRA),0)
CFLAGS += -DMIE_JVS_EXTRA
endif
LDFLAGS := -mz80 --no-std-crt0 --nostdlib --code-loc 0x8010 \
		--data-loc 0xA000 -Wl-b_VECTORS=0x9FF2

CSRCS := main.c maple.c cmd.c eeprom.c jvs.c gpio.c serial.c pad.c util.c
ASMS := crt0.s abs.s

CRELS := $(addprefix $(BUILD)/,$(CSRCS:.c=.rel))
ARELS := $(addprefix $(BUILD)/,$(ASMS:.s=.rel))

.PHONY: all install clean

all: $(IMAGE)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.rel: src/%.c | $(BUILD)
	$(SDCC) $(CFLAGS) -o $@ $<

$(BUILD)/%.rel: src/%.s | $(BUILD)
	$(SDAS) -plosgff -o $@ $<

$(BUILD)/mie.ihx: $(ARELS) $(CRELS)
	$(SDCC) $(LDFLAGS) -o $@ $(BUILD)/crt0.rel $(filter-out $(BUILD)/crt0.rel,$(ARELS) $(CRELS))

$(IMAGE): $(BUILD)/mie.ihx packbin.py
	$(PYTHON) packbin.py $(BUILD)/mie.ihx $(IMAGE)

install: $(IMAGE)
	cp $(IMAGE) $(INSTALL_BIN)

clean:
	rm -rf $(BUILD)
