TARGET = libbz2_1.0.6.a
OBJS= blocksort.o  \
      huffman.o    \
      crctable.o   \
      randtable.o  \
      compress.o   \
      decompress.o \
      bzlib.o

KOS_CFLAGS += -I../include/bzlib #-D_FILE_OFFSET_BITS=64

include $(KOS_BASE)/ds/sdk/Makefile.library

