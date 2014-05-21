# proggys
CC = gcc
RM = rm
STRIP = strip
WINDRES = windres
PACKER = upx

# flags
CFLAGS = 
PACKEROPT = -9

# files
BIN = lbacalc
TARGET = bin/$(BIN)$(OS:Windows_NT=.exe)
LIBS = $(OSTYPE:linux=-lncurses)

OBJSDIR = obj/
OBJS = $(OBJSDIR)main.o $(OBJSDIR)tools.o $(OBJSDIR)rsrc.o

#go !
all : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LIBS)
	$(STRIP) $(TARGET)
#	$(PACKER) $(PACKEROPT) $(TARGET)
		
$(OBJSDIR)tools.o: tools.c
	$(CC) $(CFLAGS) -c tools.c -o $(OBJSDIR)tools.o
		
$(OBJSDIR)main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $(OBJSDIR)main.o

$(OBJSDIR)rsrc.o: version.rc
	$(WINDRES) -i version.rc -o $(OBJSDIR)rsrc.o
	
clean:
	$(RM) $(OBJS)
	
clean-all:
	$(RM) $(OBJS) $(TARGET)