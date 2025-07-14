
TARGET = dc-img-tool
OBJS = main.c ../../../../modules/isofs/fs_iso9660.c ../../../../modules/isofs/cdi.c \
			  ../../../../modules/isofs/gdi.c  ../../../../modules/isofs/module.c
CC = gcc
EXTRA_FLAGS = -I../../../../modules/isofs/ -I../../../../include/

ifeq ($(OS),Windows_NT)
TARGET := $(TARGET).exe
EXTRA_FLAGS += -I/local/include -L/local/lib -lmingw32 -luser32 -lgdi32 -lole32 -luuid -lcomctl32 -static -mno-ms-bitfields
endif

INSTALL = install
DESTDIR = ../..

all: clean $(TARGET)

$(TARGET):
	$(CC) $(OBJS) -o $(TARGET) $(LIBS) $(EXTRA_FLAGS)
	strip $(TARGET)

install : $(TARGET)
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)/$(TARGET)

clean:
	rm -f $(TARGET) *.o
