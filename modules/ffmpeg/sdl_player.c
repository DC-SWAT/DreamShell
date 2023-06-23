/* DreamShell ##version##

   sdl_player.c - SDL_ffmpeg player
   Copyright (C)2011-2014 SWAT
*/
            
#include "ds.h"
#include "SDL.h"
#include "SDL_ffmpeg.h"

#include <string.h>

/* as an example we create an audio buffer consisting of BUF_SIZE frames */
#define BUF_SIZE 3

/* pointer to file we will be opening */
static SDL_ffmpegFile *file = 0;

/* create a buffer for audio frames */
static SDL_ffmpegAudioFrame *audioFrame[BUF_SIZE];

/* simple way of syncing, just for example purposes */
static uint64_t sync = 0,
         offset = 0;

/* returns the current position the file should be at */
static uint64_t getSync()
{
    if ( file )
    {
        if ( SDL_ffmpegValidAudio( file ) )
        {
            return sync;
        }
        if ( SDL_ffmpegValidVideo( file ) )
        {
            return ( SDL_GetTicks() % SDL_ffmpegDuration( file ) ) + offset;
        }
    }
    return 0;
}

/* use a mutex to prevent errors due to multithreading */
static SDL_mutex *mutex = 0;

static void audioCallback( void *data, Uint8 *stream, int length )
{
    /* lock mutex, so audioFrame[0] will not be changed from another thread */
    SDL_LockMutex( mutex );

    if ( audioFrame[0]->size == length )
    {
        /* update sync */
        sync = audioFrame[0]->pts;

        /* copy the data to the output */
        memcpy( stream, audioFrame[0]->buffer, audioFrame[0]->size );

        /* mark data as used */
        audioFrame[0]->size = 0;

        /* move frames in buffer */
        SDL_ffmpegAudioFrame *f = audioFrame[0];
        int i;
        for ( i = 1; i < BUF_SIZE; i++ ) audioFrame[i-1] = audioFrame[i];
        audioFrame[BUF_SIZE-1] = f;
    }
    else
    {
        /* no data available, just set output to zero */
        memset( stream, 0, length );
    }

    /* were done with the audio frame, release lock */
    SDL_UnlockMutex( mutex );

    return;
}



int sdl_ffplay(const char *filename) {

	char fn[NAME_MAX];
	sprintf(fn, "ds:%s", filename);

    /* open file from arg[1] */
    file = SDL_ffmpegOpen(fn);
    if ( !file )
    {
        ds_printf("DS_ERROR: Could not open %s: %s\n", fn, SDL_ffmpegGetError() );
        return -1;
    }

    /* initialize the mutex */
    mutex = SDL_CreateMutex();

    /* select the stream you want to decode (example just uses 0 as a default) */
    SDL_ffmpegSelectVideoStream( file, 0 );

    /* print frame rate of video stream */
    SDL_ffmpegStream *stream = SDL_ffmpegGetVideoStream( file, 0 );
    if ( stream )
    {
        ds_printf("DS_ERROR: Selected video stream 0, frame rate: %.2f\n", SDL_ffmpegGetFrameRate( stream, 0, 0 ) );
    }

    /* if no audio can be selected, audio will not be used in this example */
    SDL_ffmpegSelectAudioStream( file, 0 );

    /* print frame rate of audio stream */
    stream = SDL_ffmpegGetAudioStream( file, 0 );
    if ( stream )
    {
        ds_printf("DS_ERROR: Selected audio stream 0, frame rate: %.2f\n", SDL_ffmpegGetFrameRate( stream, 0, 0 ) );
    }

    /* get the audiospec which fits the selected audiostream, if no audiostream
       is selected, default values are used (2 channel, 48Khz) */
    SDL_AudioSpec specs = SDL_ffmpegGetAudioSpec( file, 512, audioCallback );

    /* we get the size from our active video stream, if no active video stream
       exists, width and height are set to zero */
    int w, h;
    SDL_ffmpegGetVideoSize( file, &w, &h );

    /* Open the Video device */
    SDL_Surface *screen = GetScreen();

    /* create a video frame which will be used to receive the video data. */
    SDL_ffmpegVideoFrame *videoFrame = SDL_ffmpegCreateVideoFrame();

    /* depending on which type of surface you want to use, you either create an RGB or an YUV buffer */

    /* RGB */
    videoFrame->surface = SDL_CreateRGBSurface( 0, screen->w, screen->h, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0 );

    /* YUV */
/*
    videoFrame->overlay = SDL_CreateYUVOverlay( screen->w, screen->h, SDL_YUY2_OVERLAY, screen );
*/

    /* create a SDL_Rect for blitting of image data */
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h;

    /* check if a valid audio stream was selected */
    if ( SDL_ffmpegValidAudio( file ) )
    {

        /* Open the Audio device */
        if ( SDL_OpenAudio( &specs, 0 ) < 0 )
        {
            fprintf( stderr, "Couldn't open audio: %s\n", SDL_GetError() );
            SDL_Quit();
            return -1;
        }

        /* calculate frame size ( 2 bytes per sample ) */
        int frameSize = specs.channels * specs.samples * 2;

        /* prepare audio buffer */
        int i;
        for ( i = 0; i < BUF_SIZE; i++ )
        {
            /* create frame */
            audioFrame[i] = SDL_ffmpegCreateAudioFrame( file, frameSize );

            /* check if we got a frame */
            if ( !audioFrame[i] )
            {
                /* no frame could be created, this is fatal */
                goto CLEANUP_DATA;
            }

            /* fill frame with data */
            SDL_ffmpegGetAudioFrame( file, audioFrame[i] );
        }

        /* we unpause the audio so our audiobuffer gets read */
        SDL_PauseAudio( 0 );
    }

    int done = 0,
        mouseState = 0;

    while ( !done )
    {
        /* just some standard SDL event stuff */
        SDL_Event event;
        while ( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
                case SDL_QUIT:

                    done = 1;
                    break;

                case SDL_MOUSEBUTTONUP:

                    mouseState = 0;
                    break;

                case SDL_MOUSEBUTTONDOWN:

                    mouseState = 1;
                    break;

                case SDL_KEYDOWN:

                    if ( event.key.keysym.sym == SDLK_ESCAPE )
                    {
                        done = 1;
                    }
            }
        }

        if ( mouseState )
        {
            int x, y;
            SDL_GetMouseState( &x, &y );
            /* by clicking you turn on the stream, seeking to the percentage
               in time, based on the x-position you clicked on */
            uint64_t time = ( uint64_t )((( double )x / ( double )w ) * SDL_ffmpegDuration( file ) );

            /* lock mutex when working on data which is shared with the audiocallback */
            SDL_LockMutex( mutex );

            /* invalidate current video frame */
            if ( videoFrame ) videoFrame->ready = 0;

            /* invalidate buffered audio frames */
            if ( SDL_ffmpegValidAudio( file ) )
            {
                int i;
                for ( i = 0; i < BUF_SIZE; i++ )
                {
                    audioFrame[i]->size = 0;
                }
            }

            /* we seek to time (milliseconds) */
            SDL_ffmpegSeek( file, time );

            /* store new offset */
            offset = time - ( getSync() - offset );

            /* we release the mutex so the new data can be handled */
            SDL_UnlockMutex( mutex );
        }

        /* check if we need to decode audio data */
        if ( SDL_ffmpegValidAudio( file ) )
        {
            /* lock mutex when working on data which is shared with the audiocallback */
            SDL_LockMutex( mutex );

            /* fill empty spaces in audio buffer */
            int i;
            for ( i = 0; i < BUF_SIZE; i++ )
            {
                /* check if frame is empty */
                if ( !audioFrame[i]->size )
                {
                    /* fill frame with new data */
                    SDL_ffmpegGetAudioFrame( file, audioFrame[i] );
                }
            }

            SDL_UnlockMutex( mutex );
        }

        if ( videoFrame )
        {
            /* check if video frame is ready */
            if ( !videoFrame->ready )
            {
                /* not ready, try to get a new frame */
                SDL_ffmpegGetVideoFrame( file, videoFrame );
            }
            else if ( videoFrame->pts <= getSync() )
            {
                /* video frame ready and in sync */
                if ( videoFrame->overlay )
                {
                    /* blit overlay */
                    //SDL_DisplayYUVOverlay( videoFrame->overlay, &rect );
                }
                else if ( videoFrame->surface )
                {
                    /* blit RGB surface */
					LockVideo();
                    SDL_BlitSurface( videoFrame->surface, 0, screen, 0 );
					UnlockVideo();

                    /* flip screen */
                    //SDL_Flip( screen );
                }

                /* video frame is displayed, make sure we don't show it again */
                videoFrame->ready = 0;
            }
        }
    }

CLEANUP_DATA:

    /* cleanup audio related data */
    if ( SDL_ffmpegValidAudio( file ) )
    {

        /* stop audio callback */
        SDL_PauseAudio( 1 );

        /* clean up frames */
        int i;
        for ( i = 0; i < BUF_SIZE; i++ )
        {
            SDL_ffmpegFreeAudioFrame( audioFrame[i] );
        }
    }

    /* cleanup video data */
    if ( videoFrame )
    {
        SDL_ffmpegFreeVideoFrame( videoFrame );
    }

    /* after all is said and done, we should call this */
    SDL_ffmpegFree( file );

    return 0;
}

