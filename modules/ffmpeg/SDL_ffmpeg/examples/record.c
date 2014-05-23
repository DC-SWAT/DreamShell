/*******************************************************************************
*                                                                              *
*   SDL_ffmpeg is a library for basic multimedia functionality.                *
*   SDL_ffmpeg is based on ffmpeg.                                             *
*                                                                              *
*   Copyright (C) 2007  Arjan Houben                                           *
*                                                                              *
*   SDL_ffmpeg is free software: you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published   *
*	by the Free Software Foundation, either version 3 of the License, or any   *
*   later version.                                                             *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
*   GNU Lesser General Public License for more details.                        *
*                                                                              *
*   You should have received a copy of the GNU Lesser General Public License   *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
*                                                                              *
*******************************************************************************/

#include "SDL.h"
#include "SDL_ffmpeg.h"

#include <math.h>

int main( int argc, char** argv )
{
    SDL_ffmpegFile  *file = 0;

    /* check if we got an argument */
    if ( argc < 2 )
    {
        printf( "usage: \"%s\" \"filename\"\n", argv[0] );
        return -1;
    }

    /* open file from arg[1] */
    file = SDL_ffmpegCreate( argv[1] );
    if ( !file )
    {
        fprintf( stderr, "error creating file\n" );
        SDL_Quit();
        return -1;
    }

    /* add a video stream to the output */
    SDL_ffmpegAddVideoStream( file, SDL_ffmpegCodecAUTO );

    /* select videostream we just created */
    SDL_ffmpegSelectVideoStream( file, 0 );

    /* standard SDL initialization stuff */
    if ( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 )
    {
        fprintf( stderr, "problem initializing SDL: %s\n", SDL_GetError() );
        return -1;
    }

    int w, h;
    /* try to get size of encoding */
    SDL_ffmpegGetVideoSize( file, &w, &h );

    /* create a window */
    SDL_Surface *screen = SDL_SetVideoMode( w, h, 32, SDL_DOUBLEBUF );

    /* create a block of color, so we have something to look at */
    SDL_Surface *block = SDL_CreateRGBSurface( 0, 20, 20, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 );

    int         done = 0;
    uint8_t     red = 0,
                green = 0,
                blue = 0;
    SDL_Rect    r;

    while ( !done )
    {
        /* just some standard SDL event stuff */
        SDL_Event event;
        while ( SDL_PollEvent( &event ) )
        {
            if ( event.type == SDL_QUIT )
            {
                done = 1;
                break;
            }

            /* mouse moved, store new position */
            if ( event.type == SDL_MOUSEMOTION )
            {
                r.x = event.motion.x - 10;
                r.y = event.motion.y - 10;
                r.w = 20;
                r.h = 20;
            }
        }

        /* fade out current screen */
        uint8_t *src = ( uint8_t* )screen->pixels;
        int i;
        for ( i = 0; i < screen->h*screen->w; i++ )
        {
            *src = 0xFF;
            src++;
            if ( *src )( *src )--;
            src++;
            if ( *src )( *src )--;
            src++;
            if ( *src )( *src )--;
            src++;
        }

        /* create new color */
        int c = block->format->Amask |
                red << block->format->Rshift |
                green << block->format->Gshift |
                blue << block->format->Bshift;

        /* set new color */
        SDL_FillRect( block, 0, c );

        /* change color for next frame */
        red     += 1;
        green   += 3;
        blue    += 7;

        /* copy block to screen */
        SDL_BlitSurface( block, 0, screen, &r );

        /* store video frame in file */
        SDL_ffmpegAddVideoFrame( file, screen );

        /* flip screen, so we can see what is happening */
        SDL_Flip( screen );

        int64_t delay = SDL_ffmpegVideoDuration( file ) - SDL_GetTicks();
        if ( delay > 0 ) SDL_Delay( delay );
    }

    /* after all is said and done, we should call this */
    SDL_ffmpegFree( file );

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
