# KallistiOS ##version##
#
# libmp3/Makefile
# (c)2001 Dan Potter

OBJS = main.o mpg123_snddrv.o snddrv.o

BUILD_TARGET = addons/libmp3
#KOS_CFLAGS += -I../xingmp3 
KOS_CFLAGS += -I../mpg123/src -I../mpg123/src/libmpg123 -I../../../../include/audio

all: $(OBJS)
	cp $(OBJS) ../build/

include $(KOS_BASE)/Makefile.prefab

