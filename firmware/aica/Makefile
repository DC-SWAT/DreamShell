
TARGET = ds_stream

all: $(TARGET).drv

CODEC = codec

ASRC = $(CODEC)/mp3/real/arm/asmpoly_gcc.o \
	$(CODEC)/aac/real/asm/armgcc_nosyms/sbrcov.o \
	$(CODEC)/aac/real/asm/armgcc_nosyms/sbrqmfak.o \
	$(CODEC)/aac/real/asm/armgcc_nosyms/sbrqmfsk.o

MP3_SRC = mp3.o \
	$(CODEC)/mp3/real/polyphase.o \
	$(CODEC)/mp3/real/huffman.o \
	$(CODEC)/mp3/real/hufftabs.o \
	$(CODEC)/mp3/real/buffers.o \
	$(CODEC)/mp3/mp3dec.o \
	$(CODEC)/mp3/mp3tabs.o \
	$(CODEC)/mp3/real/dqchan.o \
	$(CODEC)/mp3/real/imdct.o \
	$(CODEC)/mp3/real/stproc.o \
	$(CODEC)/mp3/real/bitstream.o \
	$(CODEC)/mp3/real/dequant.o \
	$(CODEC)/mp3/real/scalfact.o \
	$(CODEC)/mp3/real/subband.o \
	$(CODEC)/mp3/real/trigtabs.o \
	$(CODEC)/mp3/real/dct32.o

AAC_SRC = aac.o \
	$(CODEC)/aac/real/dct4.o \
	$(CODEC)/aac/real/dequant.o \
	$(CODEC)/aac/real/fft.o \
	$(CODEC)/aac/real/imdct.o \
	$(CODEC)/aac/real/pns.o \
	$(CODEC)/aac/real/sbrfft.o \
	$(CODEC)/aac/real/sbrfreq.o \
	$(CODEC)/aac/real/sbrhfadj.o \
	$(CODEC)/aac/real/sbrhfgen.o \
	$(CODEC)/aac/real/sbrhuff.o \
	$(CODEC)/aac/real/sbrimdct.o \
	$(CODEC)/aac/real/sbrmath.o \
	$(CODEC)/aac/real/sbrqmf.o \
	$(CODEC)/aac/real/sbrside.o \
	$(CODEC)/aac/real/stproc.o \
	$(CODEC)/aac/real/tns.o \
	$(CODEC)/aac/aacdec.o \
	$(CODEC)/aac/aactabs.o \
	$(CODEC)/aac/real/bitstream.o \
	$(CODEC)/aac/real/buffers.o \
	$(CODEC)/aac/real/decelmnt.o \
	$(CODEC)/aac/real/filefmt.o \
	$(CODEC)/aac/real/huffman.o \
	$(CODEC)/aac/real/hufftabs.o \
	$(CODEC)/aac/real/noiseless.o \
	$(CODEC)/aac/real/sbr.o \
	$(CODEC)/aac/real/sbrtabs.o \
	$(CODEC)/aac/real/trigtabs.o \
	$(CODEC)/aac/real/trigtabs_fltgen.o

OBJ = crt0.o main.o aica.o $(MP3_SRC) $(CODEC)/filter.o #$(AAC_SRC) $(ASRC)

INCS = -I./codec/mp3/pub -I./codec/aac/pub -I./codec

include ../../sdk/Makefile.cfg

DC_ARM_CFLAGS += $(INCS) -DARM -DARM7DI -DUSE_DEFAULT_STDLIB #-mcpu=arm7tdmi
DC_ARM_AFLAGS += $(INCS) #-mcpu=arm7tdmi

$(TARGET): $(TARGET).drv

$(TARGET).drv: $(TARGET).elf
	$(DC_ARM_OBJCOPY) -O binary $(TARGET).elf $(TARGET).drv

$(TARGET).elf: $(OBJ)
	$(DC_ARM_CC) -Wl,-Ttext,0x00000000 -nostartfiles -nostdlib -e reset -o $(TARGET).elf $(OBJ) -lgcc

%.o: %.c
	$(DC_ARM_CC) $(DC_ARM_CFLAGS) $(DC_ARM_INCS) -c $< -o $@
%.o: %.C
	$(DC_ARM_CC) $(DC_ARM_CFLAGS) $(DC_ARM_INCS) -c $< -o $@
%.o: %.s
	$(DC_ARM_AS) $(DC_ARM_AFLAGS) $< -o $@
%.o: %.S
	$(DC_ARM_AS) $(DC_ARM_AFLAGS) $< -o $@

clean:
	-rm -f $(TARGET).elf $(TARGET).drv $(TARGET).srec $(OBJ)

install: $(TARGET).drv
	-rm $(DS_BUILD)/firmware/aica/$(TARGET).drv
	cp $(TARGET).drv $(DS_BUILD)/firmware/aica/$(TARGET).drv


