cmake_minimum_required( VERSION 2.6 )

project( SDL_FFMPEG )

set( PROJECT_VERSION "1.1.1" )


if( CMAKE_GENERATOR MATCHES "Visual .*" )

    add_definitions( "/TP -DMSVC" )

else()

    add_definitions( "-Wall -ansi -pedantic -std=c99 -O2 -D_ISOC99_SOURCE" )

endif()


if( WIN32 )

	add_definitions( "-DWIN32" )

endif( WIN32 )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} )

add_subdirectory( lib )

add_subdirectory( doc )
