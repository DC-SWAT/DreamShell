#
# DreamShell ISO Loader
# dc-load
# (c) 2009-2016 SWAT
#

include Makefile.cfg

TARGETCFLAGS += $(DCL_CFLAGS) #-DLOG #-DDEBUG
LOBJECTS += $(DCL) 
LOBJECTS += $(KOS_DIR)/src/printf.o

all: rm-elf $(BUILD)/dcl.bin

rm-elf:
	rm -f $(LOBJECTS) $(BUILD)/dcl.bin $(BUILD)/dcl.elf

$(BUILD)/dcl.elf: $(LOBJECTS)
	$(TARGETCC) $(TARGETCFLAGS) $(TARGETLDFLAGS) -o $@ $(LOBJECTS) $(LIBS)
	$(TARGETSIZE) $@
