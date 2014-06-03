# The libraries

lib_LTLIBRARIES   = libini.la libtclini.la
libini_la_LDFLAGS = -version-info $(LIBVERSION)
libini_la_SOURCES = headings.h ini.cpp ini.h \
keys.h list.h

EXTRA_DIST = headings.i keys.i list.i types.i

# Add support for tcl, other languages can be supported
# here.  Swig should be called using the following:
#    swig -<lang> -module <lang>ini -o libini_<lang>.c libini.h
# The chances are that this can be automated although
# somehow we have to deal with specific language compile
# flags.


# TCL language support
libtclini_la_LDFLAGS = -version-info $(LIBVERSION)
libtclini_la_LIBADD  = ./libini.la
libtclini_la_SOURCES = libini_tcl.c

libini_tcl.c : ../include/libini.h
	swig -tcl -module tclini -o $@ $<
