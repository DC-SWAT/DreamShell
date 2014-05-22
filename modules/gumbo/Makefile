#
# gumbo-parser module for DreamShell
# Copyright (C) 2013 SWAT
#

TARGET_NAME = gumbo

OBJS = ./$(TARGET_NAME)/src/tag.o ./$(TARGET_NAME)/src/utf8.o \
		./$(TARGET_NAME)/src/util.o ./$(TARGET_NAME)/src/error.o \
		./$(TARGET_NAME)/src/parser.o ./$(TARGET_NAME)/src/vector.o \
		./$(TARGET_NAME)/src/char_ref.o ./$(TARGET_NAME)/src/attribute.o \
		./$(TARGET_NAME)/src/tokenizer.o ./$(TARGET_NAME)/src/string_piece.o \
		./$(TARGET_NAME)/src/string_buffer.o module.o

DBG_LIBS = -lds
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 0
VER_MICRO = 0

KOS_CFLAGS += $(KOS_CSTD) -L./$(TARGET_NAME)/include
KOS_LIB_PATHS += -L./$(TARGET_NAME)/lib

all: rm-elf

include ../../sdk/Makefile.loadable

library: ./$(TARGET_NAME)/Makefile
	cd ./$(TARGET_NAME) && make clean && make install

./$(TARGET_NAME)/Makefile:
	cd ./$(TARGET_NAME) && \
	./configure --prefix=`pwd` --build=sh-elf --host=sh4 --disable-shared \
	CC=$(KOS_CC) CXX=$(KOS_CCPLUS) LD=$(KOS_LD) CCLD=$(KOS_LD) AR=$(KOS_AR) RUNLIB=$(KOS_RANLIB) \
	CFLAGS="$(KOS_CFLAGS)" LDFLAGS="$(KOS_LDFLAGS)" LIBS="$(KOS_LIBS)"

rm-elf:
	-rm -f $(TARGET)
	-rm -f $(TARGET_LIB)

install: $(TARGET) $(TARGET_LIB) opkg
	-rm $(DS_BUILD)/modules/$(TARGET)
	-rm $(DS_SDK)/lib/$(TARGET_LIB)
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)

