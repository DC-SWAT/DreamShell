OBJS =  common.o dct64_sh4.o decode_i386.o decode_2to1.o \
   layer1.o layer2.o layer3.o tabinit.o \
   interface.o VbrTag.o mp3junk.o

all: libmpg.a

KOS_CFLAGS += -DUSE_LAYER_1 -DUSE_LAYER_2 -DUSE_LAYER_3 -DNOANALYSIS -DHAVE_CONFIG_H -ffast-math

libmpg.a: $(OBJS)
#	$(KOS_AR) rcs libmpg.a $(OBJS)
	cp $(OBJS) ../build

*.o: mpg123.h mpglib.h

clean:
	rm -f $(OBJS)

include $(KOS_BASE)/Makefile.rules

