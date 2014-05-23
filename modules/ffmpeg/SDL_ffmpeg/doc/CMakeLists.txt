
find_package( Doxygen )

if( DOXYGEN_FOUND )

	set( DOCUMENTATION_SOURCE   "${SDL_FFMPEG_SOURCE_DIR}/src ${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDL_ffmpeg.h" )

	set( DOCUMENTATION_HTML_FOOTER ${SDL_FFMPEG_SOURCE_DIR}/doc/footer.html )

	configure_file( ${SDL_FFMPEG_SOURCE_DIR}/doc/doxygen.in
					${SDL_FFMPEG_SOURCE_DIR}/doc/doxygen.config
					@ONLY IMMEDIATE )

	add_custom_target( documentation
					   ${DOXYGEN_EXECUTABLE} ${SDL_FFMPEG_SOURCE_DIR}/doc/doxygen.config )

endif( DOXYGEN_FOUND )
