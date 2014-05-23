add_library( 	
	SDL_ffmpeg
	SHARED
	${SDL_FFMPEG_SOURCE_DIR}/src/SDL_ffmpeg.c
)

if( WIN32 )

	set_target_properties( SDL_ffmpeg PROPERTIES PREFIX "" )

	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBUILD_DLL" )

endif( WIN32 )
	
if( MINGW )

	set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--output-def,SDL_ffmpeg.def" )

endif( MINGW )

find_package( SDL )
find_package( avformat )
find_package( avcodec )
find_package( avutil )
find_package( swscale )

include_directories(	
	${SDL_INCLUDE_DIR}
	${AVFORMAT_INCLUDE_DIR}
	${AVCODEC_INCLUDE_DIR}
	${AVUTIL_INCLUDE_DIR}
	${SWSCALE_INCLUDE_DIR}
	${SDL_FFMPEG_SOURCE_DIR}/include/SDL
)

if( WIN32 )

	set(
		EXTRA_LIBRARIES
		${EXTRA_LIBRARIES}
		wsock32
	)

endif( WIN32)


set( 
	SDL_FFMPEG_LIBRARIES	
	${AVFORMAT_LIBRARY}
	${AVCODEC_LIBRARY}
	${AVUTIL_LIBRARY}
	${SWSCALE_LIBRARY}
	${EXTRA_LIBRARIES}
	${SDL_LIBRARY}
)

target_link_libraries(
	SDL_ffmpeg
	${SDL_FFMPEG_LIBRARIES}
)

include_directories(
	${SDL_FFMPEG_SOURCE_DIR}/include
)

link_directories(
	${SDL_FFMPEG_SOURCE_DIR}/lib
)

install(
	FILES
    ${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDL_ffmpeg.h
	DESTINATION
	include/SDL/
)

install(
	TARGETS SDL_ffmpeg
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

install(
    FILES
    ${SDL_FFMPEG_SOURCE_DIR}/FindSDL_ffmpeg.cmake
    DESTINATION
    ${CMAKE_ROOT}/Modules/
)
