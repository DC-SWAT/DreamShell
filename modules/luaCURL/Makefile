#
# luaCURL module for DreamShell
# Copyright (C) 2026 SWAT
#

TARGET_NAME = luaCURL
LIBRARY_NAME = Lua-cURLv3

OBJS = module.o \
	$(LIBRARY_NAME)/src/l52util.o \
	$(LIBRARY_NAME)/src/lceasy.o \
	$(LIBRARY_NAME)/src/lcerror.o \
	$(LIBRARY_NAME)/src/lchttppost.o \
	$(LIBRARY_NAME)/src/lcmime.o \
	$(LIBRARY_NAME)/src/lcmulti.o \
	$(LIBRARY_NAME)/src/lcshare.o \
	$(LIBRARY_NAME)/src/lcutils.o \
	$(LIBRARY_NAME)/src/lcurl.o \
	$(LIBRARY_NAME)/src/lcurlapi.o

DBG_LIBS = -lds -lcurl-shared
KOS_CFLAGS += -I$(DS_SDK)/include/lua -I$(LIBRARY_NAME)/src

VER_MAJOR = 0
VER_MINOR = 3
VER_MICRO = 13

all: rm-elf $(LIBRARY_NAME)/Makefile

include ../../sdk/Makefile.loadable

$(LIBRARY_NAME)/Makefile:
	git clone --depth 1 https://github.com/Lua-cURL/Lua-cURLv3.git $(LIBRARY_NAME)

clean: rm-elf
	-rm -f $(OBJS)
	-rm -rf $(LIBRARY_NAME)

rm-elf:
	-rm -f $(TARGET) || true

install: $(TARGET) $(TARGET_LIB)
	-rm -f $(DS_BUILD)/modules/$(TARGET) || true
	cp $(TARGET) $(DS_BUILD)/modules/$(TARGET)
	cp $(LIBRARY_NAME)/src/lua/cURL.lua $(DS_BUILD)/lua/lib/
	cp -r $(LIBRARY_NAME)/src/lua/cURL $(DS_BUILD)/lua/lib/
