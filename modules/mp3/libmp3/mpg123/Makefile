OBJS =  src/libmpg123/compat.o src/libmpg123/parse.o src/libmpg123/frame.o src/libmpg123/format.o \
	src/libmpg123/index.o src/libmpg123/synth.o src/libmpg123/synth_8bit.o \
	src/libmpg123/ntom.o src/libmpg123/icy2utf8.o src/libmpg123/synth_real.o src/libmpg123/synth_s32.o\
	src/libmpg123/equalizer.o src/libmpg123/icy.o src/libmpg123/id3.o src/libmpg123/layer1.o \
	src/libmpg123/layer2.o src/libmpg123/layer3.o src/libmpg123/optimize.o src/libmpg123/readers.o \
	src/libmpg123/tabinit.o src/libmpg123/stringbuf.o src/libmpg123/libmpg123.o src/libmpg123/dct64_sh4.o

all: libmpg123.a

KOS_CFLAGS += -I./src/libmpg123 -I./src -DOPT_GENERIC -DREAL_IS_FLOAT -DHAVE_CONFIG_H -ffast-math

libmpg123.a: $(OBJS)
#	$(KOS_AR) rcs libmpg123.a $(OBJS)
	cp $(OBJS) ../build

*.o: src/config.h

clean:
	rm -f $(OBJS)

include $(KOS_BASE)/Makefile.rules

