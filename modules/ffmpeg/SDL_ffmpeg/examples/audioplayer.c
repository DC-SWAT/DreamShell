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

#include <string.h>

#define BUF_SIZE 10

/* prepare to buffer 10 frames */
SDL_ffmpegAudioFrame *frame[BUF_SIZE];

/* use a mutex to prevent errors due to multithreading */
SDL_mutex *mutex = 0;

/* use this value to check if we reached the end of our file */
int done = 0;

void audioCallback( void *udata, Uint8 *stream, int len )
{

    /* lock mutex, so frame[0] will not be changed from another thread */
    SDL_LockMutex( mutex );

    if ( frame[0]->size == len )
    {

        /* copy the data to the output */
        memcpy( stream, frame[0]->buffer, frame[0]->size );

        /* mark data as used */
        frame[0]->size = 0;

        /* move frames in buffer */
        SDL_ffmpegAudioFrame *f = frame[0];
        int i;
        for ( i = 1; i < BUF_SIZE; i++ ) frame[i-1] = frame[i];
        frame[BUF_SIZE-1] = f;

        if ( frame[0]->last ) done = 1;
    }
    else
    {
        /* no data available, just set output to zero */
        memset( stream, 0, len );
    }

    /* were done with frame[0], release lock */
    SDL_UnlockMutex( mutex );

    return;
}

int main( int argc, char** argv )
{
    /* check if we got an argument */
    if ( argc < 2 )
    {
        printf( "usage: \"%s\" \"filename\"\n", argv[0] );
        return -1;
    }

    /* standard SDL initialization stuff */
    if ( SDL_Init( SDL_INIT_AUDIO ) < 0 )
    {
        fprintf( stderr, "problem initializing SDL: %s\n", SDL_GetError() );
        return -1;
    }

    /* open file from arg[1] */
    SDL_ffmpegFile *audioFile = SDL_ffmpegOpen( argv[1] );
    if ( !audioFile )
    {
        fprintf( stderr, "could not open %s: %s\n", argv[ 1 ], SDL_ffmpegGetError() );

        return -1;
    }

    /* select the stream you want to decode (example just uses 0 as a default) */
    if ( SDL_ffmpegSelectAudioStream( audioFile, 0 ) )
    {
        fprintf( stderr, "could select audio stream: %s\n", SDL_ffmpegGetError() );

        goto CLEANUP_DATA;
    }

    /* get the audiospec which fits the selected audiostream, if no audiostream
       is selected, default values are used (2 channel, 48Khz) */
    SDL_AudioSpec specs = SDL_ffmpegGetAudioSpec( audioFile, 512, audioCallback );

    /* Open the Audio device */
    if ( SDL_OpenAudio( &specs, 0 ) < 0 )
    {
        printf( "Couldn't open audio: %s\n", SDL_GetError() );
        goto CLEANUP_DATA;
    }

    /* calculate the amount of bytes required for every callback */
    int bytes = specs.samples * specs.channels * 2;

    /* create audio frames to store data received from SDL_ffmpegGetAudioFrame */
    int i;
    for ( i = 0; i < BUF_SIZE; i++ )
    {
        frame[i] = SDL_ffmpegCreateAudioFrame( audioFile, bytes );
        if ( !frame[i] )
        {
            printf( "couldn't prepare frame buffer\n" );
            return -1;
        }
        /* fill the frames */
        SDL_ffmpegGetAudioFrame( audioFile, frame[i] );
    }

    /* initialize mutex */
    mutex = SDL_CreateMutex();

    /* we unpause the audio so our audiobuffer gets read */
    SDL_PauseAudio( 0 );

    done = 0;

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
        }

        /* lock mutex before working on buffer */
        SDL_LockMutex( mutex );

        /* check for empty places in buffer */
        int i;
        for ( i = 0; i < BUF_SIZE; i++ )
        {

            /* if an empty space is found, fill it again */
            if ( !frame[i]->size )
            {
                SDL_ffmpegGetAudioFrame( audioFile, frame[i] );
            }
        }

        /* done with buffer, release lock */
        SDL_UnlockMutex( mutex );

        /* we wish not to kill our poor cpu, so we give it some timeoff */
        SDL_Delay( 5 );
    }

CLEANUP_DATA:

    /* cleanup our buffer */
    for ( i = 0; i < BUF_SIZE; i++ )
    {
        SDL_ffmpegFreeAudioFrame( frame[i] );
    }

    /* when we are done with the file, we free it */
    SDL_ffmpegFree( audioFile );

    /* cleanup mutex */
    SDL_DestroyMutex( mutex );

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
