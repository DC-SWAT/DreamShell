#
# DreamShell ISO Loader
# dc-load
# (c) 2009-2020 SWAT
#

include Makefile.cfg

TARGETCFLAGS += $(DCL_CFLAGS)
LOBJECTS += $(DCL)

all: rm-elf $(BUILD)/dcl.bin

rm-elf:
	rm -f $(LOBJECTS) $(BUILD)/dcl.bin $(BUILD)/dcl.elf

$(BUILD)/dcl.elf: $(LOBJECTS)
	$(TARGETCC) $(TARGETCFLAGS) $(TARGETLDFLAGS) -o $@ $(LOBJECTS) $(LIBS)
	$(TARGETSIZE) $@
