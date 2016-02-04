
## Makefile.am for the SDL cdrom library

noinst_LTLIBRARIES = libcdrom.la

# Define which subdirectories need to be built
SUBDIRS = @CDROM_SUBDIRS@
DIST_SUBDIRS =	\
	aix	\
	beos	\
	bsdi	\
	dc	\
	dummy	\
	freebsd	\
	linux	\
	macos	\
	macosx	\
	mint	\
	openbsd	\
	osf	\
	qnx	\
	win32

DRIVERS = @CDROM_DRIVERS@

# Include the architecture-independent sources
COMMON_SRCS =			\
	SDL_cdrom.c		\
	SDL_syscdrom.h

libcdrom_la_SOURCES = $(COMMON_SRCS)
libcdrom_la_LIBADD = $(DRIVERS)
libcdrom_la_DEPENDENCIES = $(DRIVERS)
