###############################################################################
##          S i Z i O U S    G E N E R I C    M A K E F I L E                ##
###############################################################################

# proggys
CC = gcc
AR = ar
RM = rm
STRIP = strip
WINDRES = windres
PACKER = upx

# flags
CFLAGS = -I"../libedc/inc" -L"../libedc/lib" -O3 -fexpensive-optimizations
PACKEROPT = -9

# files
BIN = mds4dc
TARGET = bin/$(BIN)$(OS:Windows_NT=.exe)
LIBS = -ledc $(OSTYPE:linux=-lncurses)

OBJSDIR = obj/
OBJS = $(OBJSDIR)main.o $(OBJSDIR)console.o $(OBJSDIR)mdsaudio.o $(OBJSDIR)mdsdata.o $(OBJSDIR)tools.o $(OBJSDIR)mdfwrt.o $(OBJSDIR)imgwrite.o $(OS:Windows_NT=$(OBJSDIR)version.o) 

# go !	
all : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LIBS)
	$(STRIP) $(TARGET)
#	$(PACKER) $(PACKEROPT) $(TARGET)
	
# compile source
$(OBJSDIR)%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# include win32 resources
$(OBJSDIR)%.o: %.rc
	$(WINDRES) -i $< -o $@
	
clean:
	$(RM) $(OBJS)
	
clean-all:
	$(RM) $(OBJS) $(TARGET)