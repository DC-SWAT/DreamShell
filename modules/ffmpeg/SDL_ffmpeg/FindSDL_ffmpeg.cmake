# Locate SDL_FFMPEG library
# This module defines
# SDL_FFMPEG_LIBRARY, the name of the library to link against
# SDL_FFMPEG_FOUND, if false, do not try to link to SDL_FFMPEG
# SDL_FFMPEG_INCLUDE_DIR, where to find SDL_ffmpeg.h
#

set( SDL_FFMPEG_FOUND "NO" )

find_path( SDL_FFMPEG_INCLUDE_DIR SDL_ffmpeg.h
  HINTS
  $ENV{SDL_FFMPEGDIR}
  PATH_SUFFIXES include lib SDL
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
  "C:/Program Files/SDL_FFMPEG/include"
)

#message( "SDL_FFMPEG_INCLUDE_DIR is ${SDL_FFMPEG_INCLUDE_DIR}" )

find_library( SDL_FFMPEG_LIBRARY
  NAMES SDL_ffmpeg
  HINTS
  $ENV{SDL_FFMPEGDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
  "C:/Program Files/SDL_FFMPEG/lib"
)

#message( "SDL_FFMPEG_LIBRARY is ${SDL_FFMPEG_LIBRARY}" )

set( SDL_FFMPEG_FOUND "YES" )

#message( "SDL_FFMPEG_LIBRARY is ${SDL_FFMPEG_LIBRARY}" )
