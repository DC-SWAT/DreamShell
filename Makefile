#
# DreamShell Makefile (very truncated)
# Copyright (C) 2004-2014 SWAT
# http://www.dc-swat.ru
#
# This makefile can build CDI image (type "make cdi"),
# launch DS on DC through dc-tool-ip (type "make run")
# or launch DS on emulators nullDC and lxdream 
# (type "make nulldc" or "make lxdream").
#
# If you want to build an image for DC, 
# just leave empty TRAGET_PREFIX variable, otherwise
# you build an image for test on emulator (by default).
#

TARGET = DS
TRAGET_PREFIX = EMU_
DC_IP = 192.168.1.110

include sdk/Makefile.cfg

$(DS_BUILD)/1$(TARGET)_CORE.BIN: $(DS_BUILD)/$(TRAGET_PREFIX)$(TARGET)_CORE.BIN
	@echo Scramble binary file...
	@-rm -f $(DS_BUILD)/1$(TARGET)_CORE.BIN
	@$(DS_SDK)/bin/scramble $(DS_BUILD)/$(TRAGET_PREFIX)$(TARGET)_CORE.BIN $(DS_BUILD)/1$(TARGET)_CORE.BIN

$(TARGET).cdi: $(DS_BUILD)/1$(TARGET)_CORE.BIN
	@echo Creating ISO...
	@$(DS_SDK)/bin/mkisofs -V DreamShell -G $(DS_BUILD)/IP.BIN -joliet -rock -l -o $(TARGET).iso $(DS_BUILD)
	@-rm -f $(DS_BUILD)/1$(TARGET)_CORE.BIN
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d > conv_log.txt
	@-rm -f conv_log.txt
	@-rm -f $(TARGET).iso
	
cdi: $(TARGET).cdi

run: $(DS_BUILD)/DEBUG_$(TARGET)_CORE.BIN
	$(DS_SDK)/bin/dc-tool -c $(DS_BUILD) -t $(DC_IP) -x $(DS_BUILD)/DEBUG_$(TARGET)_CORE.BIN

nulldc: $(TARGET).cdi
	@echo Running DreamShell...
	@./emu/nullDC.exe -serial "debug.log"

lxdream: $(TARGET).cdi
	lxdream -p $(TARGET).cdi

clean:
	-rm -f $(DS_BUILD)/1$(TARGET)_CORE.BIN 1$(TARGET)_CORE.BIN $(TARGET).cdi
