#
# Bootstrap for BIOS ROM 
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

all: font.s boot.bin

boot.bin: boot.o
	$(KOS_OBJCOPY) -R .stack -O binary boot.o boot.bin

boot.o: boot.s font.s
	$(KOS_AS) -little boot.s -o boot.o

font.s: font.bin
	./makefonts.py > font.s

clean:
	-rm -f boot.o boot.bin
