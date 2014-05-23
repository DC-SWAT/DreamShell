##############################################################################
#
# - Unified Makefile for Xvid for *nix environments -
#
# Copyright(C) 2003-2004 Edouard Gomez <ed.gomez@free.fr>
#
#
# Description:
#  This Makefile allows building Xvid sources to obtain a shared library
#  and a static library. This Makefile uses variables defined in the
#  platform.inc file. This platform.inc file is usually created by the
#  ./configure script whenever a unix shell is available.
#
# Makefile functional dependencies:
#  - echo
#  - rm (with option -r and -f)
#  - cd
#  - make VPATH support (eg: GNU make, solaris 8 make)
#  - ar
#
# Building output:
#  - C means "_C_ompiling"
#  - A means "_A_ssembling"
#  - I means "_I_nstalling"
#  - D means "creating _D_irectory"
#  - Cl means "_Cl_eaning"
#
# NB: (for mingw32/djgpp users)
#   These 2 environments do not provide a shell by default. So it's impossible
#   to use the configure script to generate a platform.inc file suitable for
#   your machine. You have two choices:
#    - install minsys from the mingw project or install cygwin and then use
#      the configure script as on a unix system.
#    - write a platform.inc file by hand.
#
# PS: default build directory is "=build", it fits naming conventions that
#     make the arch/tla revision control program ignore files contained in
#     this directory during commits operations. This choice is completly
#     arbitrary, but try not to change it.
#
##############################################################################

include sources.inc
ifeq ($(findstring $(MAKECMDGOALS), clean distclean mrproper),)
include platform.inc
endif

RM = rm -rf

##############################################################################
#
# Build rules
#
##############################################################################

# Their Objects
OBJECTS  = $(GENERIC_OBJECTS)
OBJECTS += $(ASSEMBLY_OBJECTS)
OBJECTS += $(DCT_IA64_OBJECTS)
OBJECTS += $(PPC_ALTIVEC_OBJECTS)

# The VPATH mechanism could use a "per target" build directory
# To keep it simple at the moment, the directory is fixed to "build"
BUILD_DIR = =build
VPATH     = $(SRC_DIR):$(BUILD_DIR)

#-----------------------------------------------------------------------------
# The default rule
#-----------------------------------------------------------------------------

.SUFFIXES: .$(OBJECT_EXTENSION) .$(ASSEMBLY_EXTENSION) .c

all: info $(STATIC_LIB) $(SHARED_LIB)
	@echo
	@echo "---------------------------------------------------------------"
	@echo " Xvid has been successfully built."
	@echo
	@echo " * Binaries are currently located in the '$(BUILD_DIR)' directory"
	@echo " * To install them on your system, you can run '# make install'"
	@echo "   as root."
	@echo "---------------------------------------------------------------"
	@echo

$(OBJECTS): platform.inc

$(BUILD_DIR):
	@echo "  D: $(BUILD_DIR)"
	@$(INSTALL) -d $(BUILD_DIR)

#-----------------------------------------------------------------------------
# Generic assembly rule
#-----------------------------------------------------------------------------

.$(ASSEMBLY_EXTENSION).$(OBJECT_EXTENSION):
	@echo "  A: $(@D)/$(<F)"
	@$(INSTALL) -d $(BUILD_DIR)/$(@D)
	@$(AS) $(AFLAGS) $< -o $(BUILD_DIR)/$@

#-----------------------------------------------------------------------------
# Generic C rule
#-----------------------------------------------------------------------------

$(PPC_ALTIVEC_OBJECTS): CFLAGS+= $(ALTIVEC_CFLAGS)

.c.$(OBJECT_EXTENSION):
	@echo "  C: $(@D)/$(<F)"
	@$(INSTALL) -d $(BUILD_DIR)/$(@D)
	@$(CC) -c $(ARCHITECTURE) $(BUS) $(ENDIANNESS) $(FEATURES) $(SPECIFIC_CFLAGS) $(CFLAGS) $< -o $(BUILD_DIR)/$@

#-----------------------------------------------------------------------------
# Static Library
#-----------------------------------------------------------------------------

$(STATIC_LIB): $(BUILD_DIR) $(OBJECTS)
	@echo "  L: $(@F)"
	@cd $(BUILD_DIR) && $(AR) rc $(@F) $(OBJECTS) && $(RANLIB) $(@F)

#-----------------------------------------------------------------------------
# Shared Library
#
# NB: This rule is used a nasty way by the MacOSX module build process
#     In this only case, it uses the SPECIFIC_LDFLAGS to append an additionnal
#     linking step:
#      1/ it links a pre shared lib (libxvidcore.so-temp.4)
#      2/ it links that pre shared lib outputing the real shared lib (module)
#     In all other cases this rule is straight forward and simple.
#     PRE_SHARED_LIB == SHARED_LIB and no nasty command appending.
#
# NB': we copy the def file for the win32 target, the file is unused on other
#      platforms
#-----------------------------------------------------------------------------

$(SHARED_LIB): $(BUILD_DIR) $(OBJECTS)
	@echo "  L: $(@F)"
	@$(INSTALL) -m 644 libxvidcore.def $(BUILD_DIR)/libxvidcore.def
	@$(INSTALL) -m 644 libxvidcore.ld $(BUILD_DIR)/libxvidcore.ld
	@cd $(BUILD_DIR) && $(CC) $(LDFLAGS) $(OBJECTS) -o $(PRE_SHARED_LIB) $(SPECIFIC_LDFLAGS)

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

install: $(BUILD_DIR)/$(STATIC_LIB) $(BUILD_DIR)/$(SHARED_LIB)
	@echo "  D: $(libdir)"
	@$(INSTALL) -d $(DESTDIR)$(libdir)
	@echo "  I: $(libdir)/$(SHARED_LIB)"
	@$(INSTALL) -m 644 $(BUILD_DIR)/$(SHARED_LIB) $(DESTDIR)$(libdir)/$(SHARED_LIB)
	@echo "  I: $(libdir)/$(STATIC_LIB)"
	@$(INSTALL) -m 644 $(BUILD_DIR)/$(STATIC_LIB) $(DESTDIR)$(libdir)/$(STATIC_LIB)
	@echo "  D: $(includedir)"
	@$(INSTALL) -d $(DESTDIR)$(includedir)
	@echo "  I: $(includedir)/xvid.h"
	@$(INSTALL) -m 644 $(SRC_DIR)/xvid.h $(DESTDIR)$(includedir)/xvid.h

#-----------------------------------------------------------------------------
# Platorm specific file -- dumb rules for people executing make before
# ./configure or even ./bootstrap.sh
#-----------------------------------------------------------------------------

platform.inc: configure platform.inc.in
	./configure

configure:
	./bootstrap.sh

#-----------------------------------------------------------------------------
# .PHONY targets
#-----------------------------------------------------------------------------

.PHONY: mrproper distclean clean info \
	list-objects list-targets list-install-path list-cflags

clean:
	@echo "  Cl: Build directory"
	@$(RM) $(BUILD_DIR)

distclean: clean
	@echo "  Cl: Generated build files"
	@$(RM) platform.inc
	@$(RM) config.log
	@$(RM) config.status
	@$(RM) autom4te.cache

mrproper: distclean
	@echo "  Cl: Bootstrapped build files"
	@$(RM) configure
	@$(RM) install-sh
	@$(RM) missing
	@$(RM) config.guess
	@$(RM) mkinstalldirs
	@$(RM) config.sub

list-objects:
	@echo
	@echo "---------------------------------------------------------------"
	@echo "Object files used for this build"
	@echo "---------------------------------------------------------------"
	@echo
	@echo $(OBJECTS)
	@echo

list-targets:
	@echo
	@echo "---------------------------------------------------------------"
	@echo "Target Libraries"
	@echo "---------------------------------------------------------------"
	@echo
	@echo Shared library: $(SHARED_LIB)
	@echo Static library: $(STATIC_LIB)
	@echo

list-install-path:
	@echo
	@echo "---------------------------------------------------------------"
	@echo "Install Paths"
	@echo "---------------------------------------------------------------"
	@echo
	@echo Include Directory: $(includedir)
	@echo Library Directory: $(libdir)
	@echo

list-cflags:
	@echo
	@echo "---------------------------------------------------------------"
	@echo "Using CFLAGS"
	@echo "---------------------------------------------------------------"
	@echo
	@echo CFLAGS=$(CFLAGS)
	@echo

info: list-objects list-cflags list-targets list-install-path
