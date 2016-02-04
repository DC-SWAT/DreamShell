
## Makefile.am for the SDL video library

noinst_LTLIBRARIES = libvideo.la

# Define which subdirectories need to be built
SUBDIRS = @VIDEO_SUBDIRS@
DIST_SUBDIRS = dummy x11 dga nanox fbcon directfb vgl svga ggi aalib \
               wincommon windib windx5 \
               maccommon macdsp macrom riscos quartz \
               bwindow ps2gs photon cybergfx epoc picogui \
               ataricommon xbios gem dc qtopia XFree86

DRIVERS = @VIDEO_DRIVERS@

# Include the architecture-independent sources
COMMON_SRCS =			\
	SDL_RLEaccel.c		\
	SDL_RLEaccel_c.h	\
	SDL_blit.c		\
	SDL_blit.h		\
	SDL_blit_0.c		\
	SDL_blit_1.c		\
	SDL_blit_A.c		\
	SDL_blit_N.c		\
	SDL_bmp.c		\
	SDL_cursor.c		\
	SDL_cursor_c.h		\
	SDL_gamma.c		\
	SDL_glfuncs.h		\
	SDL_leaks.h		\
	SDL_memops.h		\
	SDL_pixels.c		\
	SDL_pixels_c.h		\
	SDL_surface.c		\
	SDL_stretch.c		\
	SDL_stretch_c.h		\
	SDL_sysvideo.h		\
	SDL_video.c		\
	SDL_yuv.c		\
	SDL_yuvfuncs.h		\
	SDL_yuv_sw.c		\
	SDL_yuv_sw_c.h		\
	SDL_yuv_mmx.c		\
	mmx.h			\
	blank_cursor.h		\
	default_cursor.h

libvideo_la_SOURCES = $(COMMON_SRCS)
libvideo_la_LIBADD = $(DRIVERS)
libvideo_la_DEPENDENCIES = $(DRIVERS)
