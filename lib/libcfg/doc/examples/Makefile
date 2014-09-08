#
# Makefile for example of libcfg+ library
#
# Developed by Lubomir Host 'rajo' <rajo AT platon.sk>
# Copyright (c) 2003-2004 Platon SDG, http://platon.sk/
# Licensed under terms of GNU General Public License.
# All rights reserved.
#

# $Platon: libcfg+/doc/examples/Makefile,v 1.2 2004/01/12 06:03:08 nepto Exp $ 


top_srcdir   = ../../src

PROGRAM   = example
SOURCES   = example.c
OBJECTS   = example.o


CC       = cc

#DEBUG_FLAGS = -g -ggdb -DDEBUG
# if compilation with -pedantic fails, remove this flag
#CFLAGS   = -O2 -Wall -Wshadow -pedantic -ansi $(DEBUG_FLAGS)
CFLAGS   = -O2 -Wall -Wshadow -ansi $(DEBUG_FLAGS)
LDFLAGS  = -L$(top_srcdir)
DEFS     =
INCLUDES = -I$(top_srcdir)
LIBS     = -lcfg+

LIBRARY  = $(top_srcdir)/libcfg+.a

COMPILE  = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS)
CCLD     = $(CC)
LINK     = $(CCLD) $(CFLAGS) $(LDFLAGS) -o $@


all: $(PROGRAM)
	@echo
	@echo "If you don't have libcfg+ installed yet, you can try"
	@echo "export LD_LIBRARY_PATH variable by running command"
	@echo "    export LD_LIBRARY_PATH=`cd ../../src; pwd`"
	@echo "and then run ./example"
	@echo "    ./example [-h] [your_command]"
	
$(PROGRAM): $(OBJECTS) $(LIBRARY)
	$(LINK) $(OBJECTS) $(LIBS)

%.o: %.c
	$(COMPILE) -c -o $@ $<

# build libcfg+ if no't builded yet
$(LIBRARY):
	@echo "We need build library first, trying ..."
	-cd $(top_srcdir)/.. \
	&& ./configure \
	&& make

clean:
	-rm -f *.o core $(PROGRAM)

# vim600: fdm=marker fdc=3

