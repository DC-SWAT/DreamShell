# KallistiOS ##version##
#
# SDL_image Makefile
# SWAT
#

FMT_INCS = -I$(KOS_BASE)/../kos-ports/include/jpeg \
			-I$(KOS_BASE)/../kos-ports/include/png \
			-I$(KOS_BASE)/../kos-ports/include/zlib

FMTS = -DLOAD_BMP=1 -DLOAD_PNG=1 -DLOAD_TGA=1 -DLOAD_PNM=1 -DLOAD_GIF=1 \
		-DLOAD_PCX=1 -DLOAD_JPG=1 -DLOAD_LBM=1 -DLOAD_XCF=1 -DLOAD_XPM=1 \
		-DLOAD_XV=1 -DLOAD_PVR=1 #-DLOAD_WEBP=1

KOS_CFLAGS += $(FMT_INCS) $(FMTS) $(KOS_CSTD) -I./ -I../../include/SDL
#KOS_CFLAGS += -DDEBUG_IMGLIB=1
KOS_CFLAGS += -Wno-unused-but-set-variable \
			-Wno-unused-variable \
			-Wno-incompatible-pointer-types

TARGET = ../libSDL_image_1.2.12.a

OBJS = \
	IMG.o			\
	IMG_bmp.o		\
	IMG_gif.o		\
	IMG_jpg.o		\
	IMG_lbm.o		\
	IMG_pcx.o		\
	IMG_png.o		\
	IMG_pnm.o		\
	IMG_tga.o		\
	IMG_tif.o		\
	IMG_xcf.o		\
	IMG_xpm.o		\
	IMG_xv.o		\
	IMG_webp.o		\
	IMG_pvr.o
	
#IMG_ImageIO.o

include ../../sdk/Makefile.library
