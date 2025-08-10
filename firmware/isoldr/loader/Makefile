#
# DreamShell ISO Loader
# (c) 2009-2025 SWAT
#

include Makefile.cfg

DEFS = ENABLE_CISO=1 ENABLE_MULTI_DISC=1
DEFS_CDDA = ENABLE_CDDA=1 ENABLE_IRQ=1 ENABLE_LIMIT=1
DEFS_VMU = ENABLE_WRITE=1 ENABLE_MAPLE=1 ENABLE_LIMIT=1 ENABLE_MULTI_DISC=1
DEFS_FEAT = ENABLE_CDDA=1 ENABLE_MAPLE=1 ENABLE_IRQ=1 ENABLE_WRITE=1 ENABLE_MULTI_DISC=1
DEFS_FULL = $(DEFS_FEAT) ENABLE_ESC=1 ENABLE_SCR=1 ENABLE_BLEEM=1

TARGETS_CD = cd_full cd_feat cd_vmu cd_cdda cd
TARGETS_SD = sd_full sd_feat sd_vmu sd_cdda sd
TARGETS_IDE = ide_full ide_feat ide_vmu ide_cdda ide
# TARGETS_DCL = dcl_full dcl_feat dcl_vmu dcl_cdda dcl
# TARGETS_NET = net_full net_feat net_vmu net_cdda net

all: $(TARGETS_CD) $(TARGETS_SD) $(TARGETS_IDE)

sd: $(BUILD)/sd.bin
$(BUILD)/sd.bin:
	make -f Makefile.sd $(DEFS) NAME=sd

sd_cdda: $(BUILD)/sd_cdda.bin
$(BUILD)/sd_cdda.bin:
	make -f Makefile.sd $(DEFS_CDDA) NAME=sd_cdda

sd_vmu: $(BUILD)/sd_vmu.bin
$(BUILD)/sd_vmu.bin:
	make -f Makefile.sd $(DEFS_VMU) NAME=sd_vmu

sd_feat: $(BUILD)/sd_feat.bin
$(BUILD)/sd_feat.bin:
	make -f Makefile.sd $(DEFS_FEAT) NAME=sd_feat

sd_full: $(BUILD)/sd_full.bin
$(BUILD)/sd_full.bin:
	make -f Makefile.sd $(DEFS_FULL) NAME=sd_full

ide: $(BUILD)/ide.bin
$(BUILD)/ide.bin:
	make -f Makefile.ide $(DEFS) NAME=ide

ide_cdda: $(BUILD)/ide_cdda.bin
$(BUILD)/ide_cdda.bin:
	make -f Makefile.ide $(DEFS_CDDA) NAME=ide_cdda

ide_vmu: $(BUILD)/ide_vmu.bin
$(BUILD)/ide_vmu.bin:
	make -f Makefile.ide $(DEFS_VMU) NAME=ide_vmu

ide_feat: $(BUILD)/ide_feat.bin
$(BUILD)/ide_feat.bin:
	make -f Makefile.ide $(DEFS_FEAT) NAME=ide_feat

ide_full: $(BUILD)/ide_full.bin
$(BUILD)/ide_full.bin:
	make -f Makefile.ide $(DEFS_FULL) NAME=ide_full

cd: $(BUILD)/cd.bin
$(BUILD)/cd.bin:
	make -f Makefile.cd $(DEFS) NAME=cd

cd_cdda: $(BUILD)/cd_cdda.bin
$(BUILD)/cd_cdda.bin:
	make -f Makefile.cd $(DEFS_CDDA) NAME=cd_cdda

cd_vmu: $(BUILD)/cd_vmu.bin
$(BUILD)/cd_vmu.bin:
	make -f Makefile.cd $(DEFS_VMU) NAME=cd_vmu

cd_feat: $(BUILD)/cd_feat.bin
$(BUILD)/cd_feat.bin:
	make -f Makefile.cd $(DEFS_FEAT) NAME=cd_feat

cd_full: $(BUILD)/cd_full.bin
$(BUILD)/cd_full.bin:
	make -f Makefile.cd $(DEFS_FULL) NAME=cd_full

# dcl: $(BUILD)/dcl.bin
# $(BUILD)/dcl.bin:
# 	make -f Makefile.dcl $(DEFS)

# net: $(BUILD)/net.bin
# $(BUILD)/net.bin:
# 	make -f Makefile.net $(DEFS)

clean: 
	rm -f $(KOS) $(EXEC) $(MAIN) $(SD) $(IDE) $(CD) #$(DCL) $(NET)
	rm -f $(BUILD)/*.elf
	rm -f $(BUILD)/*.bin
	rm -rf $(BUILD)/obj

install: 
	-mkdir -p $(INSTALL_PATH)
	cp $(BUILD)/*.bin $(INSTALL_PATH)

test_cd: clean $(TARGETS_CD)
	-mkdir -p $(INSTALL_PATH)
	cp $(BUILD)/cd*.bin $(INSTALL_PATH)

test_cd_cdda: clean cd_cdda
	-mkdir -p $(INSTALL_PATH)
	cp $(BUILD)/cd*.bin $(INSTALL_PATH)

test_ide: clean $(TARGETS_IDE)
	-mkdir -p $(INSTALL_PATH)
	cp $(BUILD)/ide*.bin $(INSTALL_PATH)

test_ide_cdda: clean ide_cdda
	-mkdir -p $(INSTALL_PATH)
	cp $(BUILD)/ide*.bin $(INSTALL_PATH)

test_sd: clean $(TARGETS_SD)
	-mkdir -p $(INSTALL_PATH)
	cp $(BUILD)/sd*.bin $(INSTALL_PATH)
