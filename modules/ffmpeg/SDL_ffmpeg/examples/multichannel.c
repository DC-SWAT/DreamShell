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

/* as an example we create an audio buffer consisting of BUF_SIZE frames */
#define BUF_SIZE 10

SDL_ffmpegFile *audioFile[10];

SDL_ffmpegAudioFrame *audioFrame[10][BUF_SIZE];

int playing[10];

int16_t clamp( int a, int b )
{
    int c = a + b;
    if ( c > 0x7FFF ) return 0x7FFF;
    if ( c < -0x7FFF ) return -0x7FFF;
    return c;
}

void audioCallback( void *udata, Uint8 *stream, int len )
{

    /* zero output data */
    memset( stream, 0, len );

    int f;
    for ( f = 0; f < 10 && audioFile[f]; f++ )
    {
        if ( playing[f] )
        {
            if ( audioFrame[f][0]->size == len )
            {
                /* add audio data to output */
                int16_t *dest = ( int16_t* )stream;
                int16_t *src = ( int16_t* )audioFrame[f][0]->buffer;

                int i = len / 2;
                while ( i-- )
                {
                    *dest = clamp( *dest, *src );
                    dest++;
                    src++;
                }

                /* mark data as used */
                audioFrame[f][0]->size = 0;

                /* move frames in buffer */
                SDL_ffmpegAudioFrame *frame = audioFrame[f][0];
                for ( i = 1; i < BUF_SIZE; i++ ) audioFrame[f][i-1] = audioFrame[f][i];
                audioFrame[f][BUF_SIZE-1] = frame;
            }
        }
    }

    return;
}

int main( int argc, char** argv )
{
    SDL_AudioSpec specs;
    int i, f, done;

    /* check if we got an argument */
    if ( argc < 2 )
    {
        printf( "usage: %s <file1> <file2> ...\n", argv[0] );
        return -1;
    }

    /* standard SDL initialization stuff */
    if ( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_VIDEO ) < 0 )
    {
        fprintf( stderr, "problem initializing SDL: %s\n", SDL_GetError() );
        return -1;
    }

    /* open window to capture keypresses */
    SDL_SetVideoMode( 320, 240, 0, SDL_DOUBLEBUF | SDL_HWSURFACE );

    /* reset audiofile pointers */
    memset( audioFile, 0, sizeof( SDL_ffmpegFile* )*10 );
    memset( audioFrame, 0, sizeof( SDL_ffmpegAudioFrame* )*10 );
    memset( playing, 0, sizeof( int )*10 );

    int getSpecs = 1;

    f = 0;
    for ( i = 1; i < argc && i < 10; i++ )
    {
        /* open file from argument */
        audioFile[f] = SDL_ffmpegOpen( argv[i] );

        if ( !audioFile[f] )
        {
            printf( "error opening file \"%s\"\n", argv[i] );
            continue;
        }

        /* select the stream you want to decode (example just uses 0 as a default) */
        if ( SDL_ffmpegSelectAudioStream( audioFile[f], 0 ) )
        {
            SDL_ffmpegFree( audioFile[f] );
            printf( "error opening audiostream\n" );
            continue;
        }

        if ( getSpecs )
        {
            /* get the audiospec which fits the selected audiostream */
            specs = SDL_ffmpegGetAudioSpec( audioFile[f], 512, audioCallback );
            getSpecs = 0;
        }

        /* calculate frame size ( 2 bytes per sample ) */
        int frameSize = specs.channels * specs.samples * 2;

        /* prepare audio buffer */
        int i;
        for ( i = 0; i < BUF_SIZE; i++ )
        {
            /* create frame */
            audioFrame[f][i] = SDL_ffmpegCreateAudioFrame( audioFile[f], frameSize );

            /* check if we got a frame */
            if ( !audioFrame[f][i] )
            {
                /* no frame could be created, this is fatal */
                goto CLEANUP_DATA;
            }

            /* fill frame with data */
            SDL_ffmpegGetAudioFrame( audioFile[f], audioFrame[f][i] );
        }

        printf( "added \"%s\" at key %i\n", argv[i], i );

        f++;
    }

    /* check if at least one file could be opened */
    if ( !audioFile[0] ) goto CLEANUP_DATA;

    /* Open the Audio device */
    if ( SDL_OpenAudio( &specs, 0 ) < 0 )
    {
        printf( "Couldn't open audio: %s\n", SDL_GetError() );
        goto CLEANUP_DATA;
    }

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
            else if ( event.type == SDL_KEYDOWN )
            {
                /* check al files, and play if needed */
                int f;
                for ( f = 0; audioFile[f]; f++ )
                {
                    if ( event.key.keysym.sym == SDLK_1 + f )
                    {
                        playing[f] = 1;
                    }
                }
            }
            else if ( event.type == SDL_KEYUP )
            {
                /* check al files, and play if needed */
                int f;
                for ( f = 0; audioFile[f]; f++ )
                {
                    if ( event.key.keysym.sym == SDLK_1 + f )
                    {
                        playing[f] = 0;
                        /* invalidate buffer */
                        int i;
                        for ( i = 0; i < BUF_SIZE; i++ )
                        {
                            audioFrame[f][i]->size = 0;
                        }
                        SDL_ffmpegSeek( audioFile[f], 0 );
                    }
                }
            }
        }

        int f;
        for ( f = 0; f < 10 && audioFile[f]; f++ )
        {
            /* update audio buffer */
            int i;
            for ( i = 0; i < BUF_SIZE; i++ )
            {
                /* check if frame is empty */
                if ( !audioFrame[f][i]->size )
                {
                    /* fill frame with data */
                    SDL_ffmpegGetAudioFrame( audioFile[f], audioFrame[f][i] );
                    if ( audioFrame[f][i]->last )
                    {
                        SDL_ffmpegSeek( audioFile[f], 0 );
                    }
                }
            }
        }

        /* we wish not to kill our poor cpu, so we give it some timeoff */
        SDL_Delay( 5 );
    }

CLEANUP_DATA:

    /* stop audio callback */
    SDL_PauseAudio( 1 );

    /* free all frames / files */
    for ( f = 0; audioFile[f]; f++ )
    {
        int i;
        for ( i = 0; i < BUF_SIZE; i++ )
        {
            SDL_ffmpegFreeAudioFrame( audioFrame[f][i] );
        }

        SDL_ffmpegFree( audioFile[f] );
    }

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
