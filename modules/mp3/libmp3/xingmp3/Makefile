OBJS = cdct.o csbt.o cup.o cupl3.o
OBJS += cwinm.o dec8.o hwin.o icdct.o isbt.o
OBJS += iup.o iwinm.o l3dq.o l3init.o
OBJS += mdct.o mhead.o msis.o uph.o upsf.o
INCS += -I. -DLITTLE_ENDIAN=1
CFLAGS += -O2

all: libxingmp3.a
libxingmp3.a: $(OBJS)
	$(KOS_AR) rcs libxingmp3.a $(OBJS)
	cp $(OBJS) ../build

clean:
	-rm -f $(OBJS) libxingmp3.a

include $(KOS_BASE)/Makefile.rules

