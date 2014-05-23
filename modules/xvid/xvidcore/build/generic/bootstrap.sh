#!/bin/sh
#
# - Bootstrap script -
#
# Copyright(C) 2003-2004 Edouard Gomez <ed.gomez@free.fr>
#
# This file builds the configure script and copies all needed files
# provided by automake/libtoolize
#
# $Id: bootstrap.sh,v 1.7 2005-05-23 09:29:43 Skal Exp $


##############################################################################
# Detect the right autoconf script
##############################################################################

# Find a suitable autoconf
AUTOCONF="autoconf2.50"
$AUTOCONF --version 1>/dev/null 2>&1

if [ $? -ne 0 ] ; then
    AUTOCONF="autoconf"
    $AUTOCONF --version 1>/dev/null 2>&1

	if [ $? -ne 0 ] ; then
        echo "ERROR: 'autoconf' not found"
        exit -1
    fi
fi

# Tests the autoconf version
AC_VER=`$AUTOCONF --version | head -1 | sed 's/'^[^0-9]*'/''/'`
AC_MAJORVER=`echo $AC_VER | cut -f1 -d'.'`
AC_MINORVER=`echo $AC_VER | cut -f2 -d'.'`

if [ "$AC_MAJORVER" -lt "2" ]; then
    echo "ERROR: This bootstrapper requires Autoconf >= 2.50 (detected $AC_VER)"
    exit -1
fi

if [ "$AC_MINORVER" -lt "50" ]; then
    echo "ERROR: This bootstrapper requires Autoconf >= 2.50 (detected $AC_VER)"
    exit -1
fi

LIBTOOLIZE="libtoolize"
$LIBTOOLIZE --version 1>/dev/null 2>&1

if [ $? -ne 0 ] ; then
    LIBTOOLIZE="glibtoolize"
    $LIBTOOLIZE --version 1>/dev/null 2>&1

    if [ $? -ne 0 ] ; then
        echo "ERROR: 'libtoolize' not found"
        exit -1
    fi
fi

AUTOMAKE="automake"
$AUTOMAKE --version 1>/dev/null 2>&1

if [ $? -ne 0 ] ; then
    echo "ERROR: 'automake' not found"
	exit -1
fi

##############################################################################
# Bootstraps the configure script
##############################################################################

echo "Creating ./configure"
$AUTOCONF

echo "Copying files provided by automake"
$AUTOMAKE -c -a 1>/dev/null 2>&1

echo "Copying files provided by libtool"
$LIBTOOLIZE -f -c 1>/dev/null 2>&1

echo "Removing files that are not needed"
rm -rf autom4* 1>/dev/null 2>&1 
rm -rf ltmain.sh 1>/dev/null 2>&1 
rm -rf *.m4 1>/dev/null 2>&1