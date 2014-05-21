#
# DreamShell Makefile (truncated)
# Copyright (C) 2004-2014 SWAT
# http://www.dc-swat.ru
#
# Note: This file just for info, it's not working!
#

TARGET = DS
DC_IP = 192.168.1.110

include sdk/Makefile.cfg

all: $(TARGET)

$(TARGET): $(TARGET)_CORE.BIN

$(TARGET)_CORE.BIN: $(TARGET).elf
	@echo Strip target...
	@$(KOS_STRIP) $(TARGET).elf
	@echo Creating binary file...
	@$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf $(TARGET)_CORE.BIN

1$(TARGET)_CORE.BIN: $(TARGET)_CORE.BIN
	@echo Scramble binary file...
	@$(DS_SDK)/bin/scramble $(TARGET)_CORE.BIN 1$(TARGET)_CORE.BIN

cdi: $(TARGET).cdi

$(TARGET).cdi: 1$(TARGET)_CORE.BIN
	@echo Creating ISO...
	@-rm -f $(BUILD_DIR)/1$(TARGET).BIN
	@cp 1$(TARGET)_CORE.BIN $(BUILD_DIR)/1$(TARGET)_CORE.BIN
	@$(DS_SDK)/bin/mkisofs -V DreamShell -G build/IP.BIN -joliet -rock -l -o $(TARGET).iso $(BUILD_DIR)
	@echo Convert ISO to CDI...
	@-rm -f $(TARGET).cdi
	@$(DS_SDK)/bin/cdi4dc $(TARGET).iso $(TARGET).cdi -d > conv_log.txt
	@-rm -f conv_log.txt
	@-rm -f $(TARGET).iso

emu: $(TARGET).cdi
	@echo Running DreamShell...
	@./emu/nullDC.exe -serial "debug.log"

emul: $(TARGET).cdi
	@echo Running DreamShell with log...
	@run ./emu/nullDC.exe -serial "debug.log" > emu.log

emuc:
	@echo Running DreamShell...
	@./emu/nullDC.exe -serial "debug.log"
	
run: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool -c $(BUILD_DIR) -t $(DC_IP) -x $(TARGET).elf
	
debug: $(TARGET).elf
	$(DS_SDK)/bin/dc-tool -g -c $(BUILD_DIR) -t $(DC_IP) -x $(TARGET).elf
 
gdb: $(TARGET)-DBG.elf
	$(KOS_CC_BASE)/bin/$(KOS_CC_PREFIX)-gdb $(TARGET)-DBG.elf --eval-command "target remote localhost:2159"

lxcd: $(TARGET).cdi
	lxdream -p $(TARGET).cdi
	
lxelf: $(TARGET).elf
	lxdream -u -p -e $(TARGET).elf

lxgdb: $(TARGET).cdi $(TARGET)-DBG.elf
	lxdream -g 2000 -n $(TARGET).cdi &
	sleep 2
	$(KOS_CC_BASE)/bin/$(KOS_CC_PREFIX)-gdb $(TARGET)-DBG.elf --eval-command "target remote localhost:2000"

clean:
	-rm -f $(TARGET).elf $(TARGET)_CORE.BIN 1$(TARGET)_CORE.BIN $(TARGET).cdi
