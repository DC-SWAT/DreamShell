#
# ffmpeg module for DreamShell
# Copyright (C)2011-2025 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = ffmpeg
OBJS = module.o player.o pcm.o
DBG_LIBS = -lds
LIBS = -lavcodec -lavformat -lavutil
EXPORTS_FILE = exports.txt
# EXPORTS_FILE = exports_ffmpeg.txt
GCC_LIB =
FFMPEG_PATH = ffmpeg-0.6.3
# FFMPEG_PATH = FFmpeg

VER_MAJOR = 2
VER_MINOR = 0
VER_MICRO = 0

KOS_CFLAGS += -I./include -O3
KOS_LIB_PATHS += -L./lib

all: rm-elf $(FFMPEG_PATH)/config.h include/libavcodec/avcodec.h

include ../../sdk/Makefile.loadable

$(FFMPEG_PATH)/config.h:
	./config.sh
# git clone --depth 1 --branch release/2.2 https://github.com/FFmpeg/FFmpeg.git

include/libavcodec/avcodec.h:
	cd ./$(FFMPEG_PATH) && make install

clean: rm-elf
	-rm -f $(OBJS)
	cd ./$(FFMPEG_PATH) && make clean
	-rm -R ./include
	-rm -R ./lib
	-rm -f ./$(FFMPEG_PATH)/config.h

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm $(DS_BUILD)/modules/$(TARGET) || true
	-rm $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
