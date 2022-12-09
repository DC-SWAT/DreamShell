#
# DreamShell ISO Loader
# (c) 2009-2022 SWAT
#

include Makefile.cfg

# INSTALL_PATH = /Volumes/CF-128GB/DS/firmware/isoldr

all: clean cd dcl sd ide #net

sd: $(BUILD)/sd.bin
$(BUILD)/sd.bin:
	make -f Makefile.sd

ide: $(BUILD)/ide.bin
$(BUILD)/ide.bin:
	make -f Makefile.ide

net: $(BUILD)/net.bin
$(BUILD)/net.bin:
	make -f Makefile.net

cd: $(BUILD)/cd.bin
$(BUILD)/cd.bin:
	make -f Makefile.cd

dcl: $(BUILD)/dcl.bin
$(BUILD)/dcl.bin:
	make -f Makefile.dcl

clean: 
	rm -f $(KOS) $(EXEC) $(MAIN) $(SD) $(IDE) $(CD) $(DCL) #$(NET) 
	rm -R -f $(BUILD)/*.elf
	rm -R -f $(BUILD)/*.bin

rmbin:
	rm -f *.elf
	rm -f *.bin
	
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
