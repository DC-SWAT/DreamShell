# The top-level input Makefile for SDL

# require automake 1.4
AUTOMAKE_OPTIONS = 1.4

## Any directories that you want built and installed should go here.
SUBDIRS = src include docs

## Any directories you want a part of the distribution should be listed
## here, as well as have a Makefile generated at the end of configure.in
##
## This only works for subdirectories one level deep.
DIST_SUBDIRS = $(SUBDIRS)

# SDL runtime configuration script
bin_SCRIPTS = sdl-config

# All the rest of the distributed files
EXTRA_DIST =		\
	BUGS		\
	TODO		\
	COPYING		\
	CREDITS		\
	INSTALL		\
	README		\
	README.AmigaOS	\
	README.CVS	\
	README.DC	\
	README.Epoc	\
	README.MacOS	\
	README.MacOSX	\
	README.MiNT	\
	README.NanoX	\
	README.PicoGUI	\
	README.QNX	\
	README.Qtopia	\
	README.WinCE	\
	README-SDL.txt	\
	Borland.html	\
	Borland.zip	\
	VisualC.html	\
	VisualC.zip	\
	VisualCE.zip	\
	Makefile.dc	\
	MPWmake.sea.bin	\
	CWprojects.sea.bin \
	PBProjects.tar.gz \
	EpocBuildFiles.zip \
	Xcode.tar.gz	\
	Xcode21.tar.gz	\
	WhatsNew	\
	docs.html	\
	sdl.m4		\
	SDL.spec	\
	autogen.sh	\
	strip_fPIC.sh

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = sdl.m4

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

# Rule to rebuild the export lists for BeOS, MacOS and Win32.
exports:
	(cd src/main/beos/exports; $(MAKE))
	(cd src/main/macos/exports; $(MAKE))
	(cd src/main/macosx/exports; $(MAKE))
	(cd src/main/win32/exports; $(MAKE))

# Rule to build the Project Builder archive in MacOS X
PBProjects.tar.gz:
	rm -f `find . -name .DS_Store`
	if [ -d PBProjects ]; then \
	    tar zcvf $@ PBProjects; \
	fi

# Rule to install the libraries only - prevent rebuilding apps
install-lib:
	cd src && $(MAKE) install-libLTLIBRARIES

# Run ldconfig after installing the library:
install-hook:
	-ldconfig

# Grab the test programs for the distribution:
dist-hook:
	if test -f test/Makefile; then (cd test; make distclean); fi
	rm -rf $(srcdir)/test/autom4te*
	cp -rp $(srcdir)/test $(distdir)
	rm -rf `find $(distdir) -type d -name CVS -print`

# Create a CVS snapshot that people can run update -d on
CVSROOT = :pserver:guest@libsdl.org:/home/sdlweb/libsdl.org/cvs
snapshot:
	cvs -d $(CVSROOT) login
	cvs -d $(CVSROOT) checkout SDL12
	(cd SDL12 && ./autogen.sh && rm -rf autom4te.cache)
	mv SDL12 SDL-1.2
	tar zcvf $(HOME)/SDL-1.2.tar.gz SDL-1.2
	rm -rf SDL-1.2
snapshot13:
	cvs -d $(CVSROOT) login
	cvs -d $(CVSROOT) checkout -r branch_1_3_x SDL12
	(cd SDL12 && ./autogen.sh && rm -rf autom4te.cache)
	mv SDL12 SDL-1.3
	tar zcvf $(HOME)/SDL-1.3.tar.gz SDL-1.3
	rm -rf SDL-1.3
