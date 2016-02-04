
## Makefile.am for the SDL thread library

noinst_LTLIBRARIES = libthread.la

ARCH_SUBDIRS = $(srcdir)/generic	\
               $(srcdir)/amigaos	\
               $(srcdir)/beos		\
               $(srcdir)/bsdi		\
               $(srcdir)/dc		\
               $(srcdir)/epoc		\
               $(srcdir)/irix		\
               $(srcdir)/linux		\
               $(srcdir)/pth		\
               $(srcdir)/win32

# Older versions of Linux require an asm clone() implementation
if USE_CLONE
THREAD_ASM_SRC = linux/clone.S
else
THREAD_ASM_SRC =
endif

COMMON_SRCS =			\
	SDL_systhread.h		\
	SDL_thread.c		\
	SDL_thread_c.h
ARCH_SRCS = 			\
	SDL_systhread.c		\
	SDL_systhread_c.h	\
	SDL_sysmutex.c		\
	SDL_sysmutex_c.h	\
	SDL_syssem.c		\
	SDL_syssem_c.h		\
	SDL_syscond.c		\
	SDL_syscond_c.h		\
	$(THREAD_ASM_SRC)

libthread_la_SOURCES = $(COMMON_SRCS) $(ARCH_SRCS)

## Let automake know that it shouldn't distribute linked sources
BUILT_SOURCES = $(ARCH_SRCS)

## Let automake know that it should remove these for distribution
DISTCLEANFILES = $(ARCH_SRCS)

# The architecture specific directories need to be copied into place
# when building a distribution.
dist-hook:
	(cd $(distdir) && rm -f $(BUILT_SOURCES))
	cp -rp $(ARCH_SUBDIRS) $(distdir)
	(cd $(distdir) && rm -rf `find . -name CVS`)
