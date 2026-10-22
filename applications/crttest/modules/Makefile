#
# CRT Test app module for DreamShell
#

APP_NAME = crttest
TARGET_NAME = app_$(APP_NAME)
OBJS = module.o draw.o patterns.o scroll_art.o gpu.o view.o
DBG_LIBS = -lds -lwave
EXPORTS_FILE = exports.txt

KOS_CFLAGS += -I./

VER_MAJOR = 1
VER_MINOR = 4
VER_MICRO = 0

all: rm-elf

scroll_art.o: scroll_art.s scroll_near.rgb scroll_sky.rgb scroll_mid.rgb scroll_cloud.rgb scroll_floor.rgb scroll_vert.rgb
	kos-cc -c scroll_art.s -o scroll_art.o

gpu.o: gpu.cpp gpu.h
	kos-c++ -fno-exceptions -fno-rtti $(KOS_CFLAGS) -I. -c gpu.cpp -o gpu.o

include ../../../sdk/Makefile.loadable

$(TARGET): ../../../sdk/lib/libwave.a

../../../sdk/lib/libwave.a:
	mkdir -p ../../../build/modules ../../../sdk/lib
	$(MAKE) -C ../../../modules/wav install

rm-elf:
	-rm -f $(TARGET)

install: $(TARGET)
	-rm $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
	cp $(TARGET) $(DS_BUILD)/apps/$(APP_NAME)/modules/$(TARGET)
