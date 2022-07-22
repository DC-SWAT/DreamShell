#
# openssl module for DreamShell
# Copyright (C)2022 SWAT
# http://www.dc-swat.ru
#

TARGET_NAME = openssl
OBJS = module.o
DBG_LIBS = -lds -lz
LIBS = -lcrypto -lssl
GCC_LIB = -lgcc
EXPORTS_FILE = exports.txt

VER_MAJOR = 1
VER_MINOR = 1
VER_MICRO = 1

TARGET_FULLNAME = $(TARGET_NAME)-$(VER_MAJOR).$(VER_MINOR).$(VER_MICRO)n

KOS_CFLAGS += -I./$(TARGET_FULLNAME)
KOS_LIB_PATHS += -L./$(TARGET_FULLNAME)

all: rm-elf build

include ../../sdk/Makefile.loadable

$(TARGET_FULLNAME).tar.gz:
	wget https://www.openssl.org/source/$(TARGET_FULLNAME).tar.gz

$(TARGET_FULLNAME)/Configure: $(TARGET_FULLNAME).tar.gz
	tar zxf $(TARGET_FULLNAME).tar.gz

$(TARGET_FULLNAME)/Makefile: $(TARGET_FULLNAME)/Configure
	cd ./$(TARGET_FULLNAME) && ./Configure linux-generic32 \
		--cross-compile-prefix=${KOS_CC_BASE}/bin/$(KOS_CC_PREFIX)- \
		no-hw no-threads no-shared no-asm no-ui-console no-tests \
		no-stdio no-sock no-deprecated no-dso no-pic zlib \
		-DL_ENDIAN -UHAVE_CRYPTODEV -DNO_SYS_UN_H -DNO_SYSLOG -DPOSIX_TIMERS=0L \
		--prefix="`pwd`/build" --openssldir="`pwd`/build"
	perl -i -pe's/_POSIX_TIMERS/POSIX_TIMERS/g' $(TARGET_FULLNAME)/crypto/rand/rand_unix.c

build: $(TARGET_FULLNAME)/Makefile
	cd ./$(TARGET_FULLNAME) && make -j4 \
		CFLAGS="$(KOS_CFLAGS) -I$(KOS_PORTS)/include/zlib" \
		CXXFLAGS="$(KOS_CXXFLAGS)" LDFLAGS="$(KOS_LDFLAGS)"

rm-elf:
	-rm -f $(TARGET) || true
	-rm -f $(TARGET_LIB) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	-rm -f $(DS_SDK)/lib/$(TARGET_LIB) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(TARGET_LIB) $(DS_SDK)/lib/$(TARGET_LIB)
