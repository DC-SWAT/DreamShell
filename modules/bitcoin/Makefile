#
# Bitcoin module for DreamShell
# Copyright (C)2022 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = bitcoin
OBJS = module.o
DBG_LIBS = -lds
LIBS = -lsecp256k1
EXPORTS_FILE = exports.txt

VER_MAJOR = 0
VER_MINOR = 1
VER_MICRO = 0

KOS_CFLAGS += -I./secp256k1/include
KOS_LIB_PATHS += -L./secp256k1/.libs

all: rm-elf build_lib

include ../../sdk/Makefile.loadable

secp256k1/autogen.sh:
	git clone https://github.com/bitcoin-core/secp256k1.git
	cd ./secp256k1 && git checkout 485f608fa9e28f132f127df97136617645effe81

secp256k1/Makefile: secp256k1/autogen.sh
	cd ./secp256k1 && ./autogen.sh && ./configure \
		--prefix="`pwd`/build" --host=$(KOS_CC_PREFIX) LIBS="$(KOS_LIBS)" \
		CFLAGS="$(KOS_CFLAGS)" LDFLAGS="$(KOS_LDFLAGS)" \
		--without-asm --disable-exhaustive-tests \
		--disable-tests --disable-examples --disable-shared --disable-benchmark \
		--with-ecmult-window=5 --with-ecmult-gen-precision=2 \
		--enable-external-default-callbacks --enable-module-recovery \
		--enable-module-ecdh --enable-module-schnorrsig

build_lib: secp256k1/Makefile
	cd ./secp256k1 && make

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	-rm -f $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
