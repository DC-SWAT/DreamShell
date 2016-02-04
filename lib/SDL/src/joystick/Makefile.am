
## Makefile.am for the SDL joystick library

noinst_LTLIBRARIES = libjoystick.la

# Define which subdirectories need to be built
SUBDIRS = @JOYSTICK_SUBDIRS@
DIST_SUBDIRS = dummy amigaos beos bsd darwin dc linux macos mint riscos win32

DRIVERS = @JOYSTICK_DRIVERS@

# Include the architecture-independent sources
COMMON_SRCS =			\
	SDL_joystick.c		\
	SDL_joystick_c.h	\
	SDL_sysjoystick.h

libjoystick_la_SOURCES = $(COMMON_SRCS)
libjoystick_la_LIBADD = $(DRIVERS)
libjoystick_la_DEPENDENCIES = $(DRIVERS)
