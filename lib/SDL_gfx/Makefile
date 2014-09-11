# DreamShell ##version##
# (с) 2008-2014 SWAT
# http://www.dc-swat.ru
# 

TARGET = libSDL_gfx_2.0.25.a

SRCS =	\
	SDL_framerate.c \
	SDL_gfxPrimitives.c \
	SDL_imageFilter.c \
	SDL_rotozoom.c \
	SDL_gfxBlitFunc.c

KOS_CFLAGS += -I../../include -I../../include/SDL

OBJS = $(SRCS:.c=.o)


include ../../sdk/Makefile.library
