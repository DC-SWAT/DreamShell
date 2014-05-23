#
# ffmpeg module for DreamShell
# Copyright (C)2011-2014 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = ffmpeg
OBJS = module.o player.o block8x8.o aica.o mpg123.o oggvorbis.o #xvid.o
#OBJS += sdl_player.o SDL_ffmpeg/src/SDL_ffmpeg.o
DBG_LIBS = -lds -lbzip2 -loggvorbis -lmpg123 #-lxvidcore
LIBS = -lavcodec -lavformat -lavutil #-lswscale
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 6
VER_MICRO = 3

KOS_CFLAGS += -I./include
KOS_LIB_PATHS += -L./lib

all: rm-elf

include ../../sdk/Makefile.loadable

config:
	./config.sh
	
library:
	cd ./ffmpeg-0.6.3 && make clean && make install

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
