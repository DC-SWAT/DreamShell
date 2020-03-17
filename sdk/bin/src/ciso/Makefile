
CFLAGS ?= -O2  # can be overriden by environment variable
DEBUGFLAGS = -g -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -Wdeclaration-after-statement -Werror
LDLIBS  = -lz -llzo2

INSTALL = install
DESTDIR = ../..

.PHONY: release
release : CPPFLAGS += -DNDEBUG
release : ciso

.PHONY: all
all : ciso

.PHONY: debug
debug: ciso-debug

.PHONY: ciso-debug
ciso-debug: CFLAGS = $(DEBUGFLAGS)
ciso-debug: ciso

.PHONY: install
install : ciso
	$(INSTALL) -m 755 ciso $(DESTDIR)/ciso

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) ciso
