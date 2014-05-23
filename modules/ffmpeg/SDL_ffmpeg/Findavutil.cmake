# Locate AVUTIL library
# This module defines
# AVUTIL_LIBRARY, the name of the library to link against
# AVUTIL_FOUND, if false, do not try to link to AVUTIL
# AVUTIL_INCLUDE_DIR, where to find avutil.h
#

set( AVUTIL_FOUND "NO" )

find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES include 
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include 
  /opt/include
  /mingw/include
)

#message( "AVUTIL_INCLUDE_DIR is ${AVUTIL_INCLUDE_DIR}" )

find_library( AVUTIL_LIBRARY
  NAMES avutil
  HINTS
  $ENV{AVUTILDIR}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

#message( "AVUTIL_LIBRARY is ${AVUTIL_LIBRARY}" )

set( AVUTIL_FOUND "YES" )

#message( "AVUTIL_LIBRARY is ${AVUTIL_LIBRARY}" )
