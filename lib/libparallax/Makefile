# Parallax for DreamShell ##version##
#
# Makefile
# (c)2002 Dan Potter
# (c)2014, 2024 SWAT

TARGET = ../libparallax_2.0.0.a
OBJS := $(patsubst %.c,%.o,$(wildcard src/*.c))

KOS_CFLAGS += -I../../include -I./include

# Grab the shared Makefile pieces
include ../../sdk/Makefile.library
