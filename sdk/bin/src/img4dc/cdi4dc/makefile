###############################################################################
##          S i Z i O U S    G E N E R I C    M A K E F I L E                ##
###############################################################################

# proggys
LBITS := $(shell getconf LONG_BIT)
CC ?= gcc
AR  = ar
STRIP = strip
WINDRES = windres
PACKER = upx

# flags
CFLAGS = -I"../libedc/inc" -L"../libedc/lib" -O3 -fexpensive-optimizations
PACKEROPT = -9

# files
BIN = cdi4dc
TARGET = bin/$(BIN)$(OS:Windows_NT=.exe)
LIBS = -ledc $(OSTYPE:linux=-lncurses)

OBJSDIR = obj/
OBJS = $(OBJSDIR)main.o $(OBJSDIR)cdiaudio.o $(OBJSDIR)cdidata.o $(OBJSDIR)common.o $(OBJSDIR)tools.o $(OS:Windows_NT=$(OBJSDIR)version.o)

# go !
.PHONY: all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LIBS)
	$(STRIP) $(TARGET)
#	$(PACKER) $(PACKEROPT) $(TARGET)

# compile source
$(OBJSDIR)%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# include win32 resources
$(OBJSDIR)%.o: %.rc
	$(WINDRES) -i $< -o $@

.PHONY: install
install : $(TARGET)
	cp $(TARGET) ../../..

.PHONY: clean
clean:
	$(RM) $(OBJS)

.PHONY: clean-all
clean-all:
	$(RM) $(OBJS) $(TARGET)
