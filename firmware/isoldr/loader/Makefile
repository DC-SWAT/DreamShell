#
# DreamShell ISO Loader
# (c) 2009-2022 SWAT
#

include Makefile.cfg

DEFS = ENABLE_CISO=1
DEFS_EXTENDED = ENABLE_CDDA=1 ENABLE_IRQ=1
DEFS_FULL = $(DEFS) $(DEFS_EXTENDED) ENABLE_MAPLE=1 ENABLE_WRITE=1 ENABLE_ESC=1

TARGETS = cd_full cd_ext cd dcl_full dcl_ext dcl sd_full sd_ext sd ide_full ide_ext ide
# TARGETS += net_full net_ext net

all: clean $(TARGETS)

sd: $(BUILD)/sd.bin
$(BUILD)/sd.bin:
	make -f Makefile.sd $(DEFS)

sd_ext: $(BUILD)/sd_ext.bin
$(BUILD)/sd_ext.bin:
	make -f Makefile.sd $(DEFS_EXTENDED)
	mv $(BUILD)/sd.bin $(BUILD)/sd_ext.bin

sd_full: $(BUILD)/sd_full.bin
$(BUILD)/sd_full.bin:
	make -f Makefile.sd $(DEFS_FULL)
	mv $(BUILD)/sd.bin $(BUILD)/sd_full.bin

ide: $(BUILD)/ide.bin
$(BUILD)/ide.bin:
	make -f Makefile.ide $(DEFS)

ide_ext: $(BUILD)/ide_ext.bin
$(BUILD)/ide_ext.bin:
	make -f Makefile.ide $(DEFS_EXTENDED)
	mv $(BUILD)/ide.bin $(BUILD)/ide_ext.bin

ide_full: $(BUILD)/ide_full.bin
$(BUILD)/ide_full.bin:
	make -f Makefile.ide $(DEFS_FULL)
	mv $(BUILD)/ide.bin $(BUILD)/ide_full.bin

net: $(BUILD)/net.bin
$(BUILD)/net.bin:
	make -f Makefile.net $(DEFS)

net_ext: $(BUILD)/net_ext.bin
$(BUILD)/net_ext.bin:
	make -f Makefile.net $(DEFS_EXTENDED)
	mv $(BUILD)/net.bin $(BUILD)/net_ext.bin

net_full: $(BUILD)/net_full.bin
$(BUILD)/net_full.bin:
	make -f Makefile.net $(DEFS_FULL)
	mv $(BUILD)/net.bin $(BUILD)/net_full.bin

cd: $(BUILD)/cd.bin
$(BUILD)/cd.bin:
	make -f Makefile.cd $(DEFS)

cd_ext: $(BUILD)/cd_ext.bin
$(BUILD)/cd_ext.bin:
	make -f Makefile.cd $(DEFS_EXTENDED)
	mv $(BUILD)/cd.bin $(BUILD)/cd_ext.bin

cd_full: $(BUILD)/cd_full.bin
$(BUILD)/cd_full.bin:
	make -f Makefile.cd $(DEFS_FULL)
	mv $(BUILD)/cd.bin $(BUILD)/cd_full.bin

dcl: $(BUILD)/dcl.bin
$(BUILD)/dcl.bin:
	make -f Makefile.dcl $(DEFS)

dcl_ext: $(BUILD)/dcl_ext.bin
$(BUILD)/dcl_ext.bin:
	make -f Makefile.dcl $(DEFS_EXTENDED)
	mv $(BUILD)/dcl.bin $(BUILD)/dcl_ext.bin

dcl_full: $(BUILD)/dcl_full.bin
$(BUILD)/dcl_full.bin:
	make -f Makefile.dcl $(DEFS_FULL)
	mv $(BUILD)/dcl.bin $(BUILD)/dcl_full.bin

clean: 
	rm -f $(KOS) $(EXEC) $(MAIN) $(SD) $(IDE) $(CD) $(DCL) #$(NET) 
	rm -R -f $(BUILD)/*.elf
	rm -R -f $(BUILD)/*.bin
	
install: 
	-mkdir -p $(INSTALL_PATH)
	cp -R $(BUILD)/*.bin $(INSTALL_PATH)
	
test: clean cd
	-mkdir -p $(INSTALL_PATH)
	-rm -f $(INSTALL_PATH)/cd.bin
	cp $(BUILD)/cd.bin $(INSTALL_PATH)/cd.bin

test_ide: clean ide
	-mkdir -p $(INSTALL_PATH)
	-rm -f $(INSTALL_PATH)/ide.bin
	cp $(BUILD)/ide.bin $(INSTALL_PATH)/ide.bin

test_sd: clean sd
	-mkdir -p $(INSTALL_PATH)
	-rm -f $(INSTALL_PATH)/sd.bin
	cp $(BUILD)/sd.bin $(INSTALL_PATH)/sd.bin
