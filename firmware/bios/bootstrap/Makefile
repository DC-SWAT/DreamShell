#
# Bootstrap for BIOS ROM 
# Copyright (C) 2009-2014 SWAT
# http://www.dc-swat.ru
#

all: prog.s font.s boot.bin

boot.bin: boot.o
	$(KOS_OBJCOPY) -R .stack -O binary boot.o Dreamshell.bios

boot.o: boot.s font.s prog.s
	$(KOS_AS) -little boot.s -o boot.o

font.s: font.bin
	./makefonts.py > font.s
	
prog.s: prog.bin
	./makeprog.py > prog.s

clean:
	-rm -f boot.o Dreamshell.bios prog.s
