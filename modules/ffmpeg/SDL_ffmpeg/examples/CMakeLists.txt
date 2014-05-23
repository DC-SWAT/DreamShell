cmake_minimum_required( VERSION 2.6 )

project( EXAMPLES )

if( CMAKE_GENERATOR MATCHES "Visual .*" )

    add_definitions( "/TP -DMSVC" )

else()

    add_definitions( "-Wall -ansi -pedantic -std=c99 -O2 -D_ISOC99_SOURCE" )

endif()

find_package( SDL REQUIRED )

find_package( SDL_ffmpeg REQUIRED )

add_executable(	example         ${EXAMPLES_SOURCE_DIR}/example.c )

add_executable(	record          ${EXAMPLES_SOURCE_DIR}/record.c )

add_executable(	multichannel	${EXAMPLES_SOURCE_DIR}/multichannel.c )

add_executable(	audioplayer     ${EXAMPLES_SOURCE_DIR}/audioplayer.c )

add_executable(	extractaudio	${EXAMPLES_SOURCE_DIR}/extractaudio.c )

include_directories( ${SDL_FFMPEG_INCLUDE_DIR}
					 ${SDL_INCLUDE_DIR}
)

target_link_libraries(	example
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	record
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	multichannel
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	audioplayer
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	extractaudio
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )
