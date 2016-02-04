
## Makefile.am for the main SDL library

# These are the subdirectories that are always built
CORE_SUBDIRS =			\
	main

# These are the subdirectories which may be built
EXTRA_SUBDIRS =			\
	 audio video events joystick cdrom thread timer endian file cpuinfo hermes

# These are the subdirectories which will be built now
SUBDIRS = $(CORE_SUBDIRS) @SDL_EXTRADIRS@

# These are the subdirectories which will be distributed with "make dist"
DIST_SUBDIRS = $(CORE_SUBDIRS) $(EXTRA_SUBDIRS)

# The SDL library target
lib_LTLIBRARIES = libSDL.la

libSDL_la_SOURCES = $(GENERAL_SRCS)
libSDL_la_LDFLAGS = 		\
	-no-undefined		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDL_la_LIBADD =		\
	main/libarch.la		\
	@SDL_EXTRALIBS@		\
	@SYSTEM_LIBS@
libSDL_la_DEPENDENCIES =	\
	main/libarch.la		\
	@SDL_EXTRALIBS@

# The SDL library sources
GENERAL_SRCS = 		\
	SDL.c			\
	SDL_error.c		\
	SDL_error_c.h		\
	SDL_fatal.c		\
	SDL_fatal.h		\
	SDL_getenv.c		\
	SDL_loadso.c

