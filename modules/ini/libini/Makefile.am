AUTOMAKE_OPTIONS = foreign

SUBDIRS = include src

EXTRA_DIST = unix win libini.spec

# Remove CVS directories from any subdirs that
# get included through EXTRA_DIST

dist-hook:
	rm -rf 'find $(distdir) -path \*/CVS'
