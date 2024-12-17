#
# ffmpeg module for DreamShell
# Copyright (C)2011-2024 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = ffmpeg
OBJS = module.o player.o aica.o mpg123.o oggvorbis.o #xvid.o
DBG_LIBS = -lds -lbzip2 -loggvorbis -lmpg123 #-lxvidcore
LIBS = -lavcodec -lavformat -lavutil #-lswscale
EXPORTS_FILE = exports.txt
GCC_LIB =

VER_MAJOR = 0
VER_MINOR = 6
VER_MICRO = 3

KOS_CFLAGS += -I./include
KOS_LIB_PATHS += -L./lib

all: rm-elf ffmpeg-0.6.3/config.h include/libavcodec/avcodec.h

include ../../sdk/Makefile.loadable

ffmpeg-0.6.3/config.h:
	-mv $(KOS_PORTS)/lib/libogg.a $(KOS_PORTS)/lib/libogg-kos.a
	-mv $(KOS_PORTS)/lib/libvorbis.a $(KOS_PORTS)/lib/libvorbis-kos.a
	./config.sh

include/libavcodec/avcodec.h:
	@-mv $(KOS_PORTS)/lib/libogg.a $(KOS_PORTS)/lib/libogg-kos.a
	@-mv $(KOS_PORTS)/lib/libvorbis.a $(KOS_PORTS)/lib/libvorbis-kos.a
	cd ./ffmpeg-0.6.3 && make install
	-mv $(KOS_PORTS)/lib/libogg-kos.a $(KOS_PORTS)/lib/libogg.a
	-mv $(KOS_PORTS)/lib/libvorbis-kos.a $(KOS_PORTS)/lib/libvorbis.a

clean: rm-elf
	-rm -f $(OBJS)
	cd ./ffmpeg-0.6.3 && make clean
	-rm -R ./include
	-rm -R ./lib
	-rm -f ./ffmpeg-0.6.3/config.h

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET) || true
	-rm $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
