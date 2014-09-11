/*
    showrtf:  An example of using the SDL_rtf library
    Copyright (C) 2003-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id: showrtf.c 4211 2008-12-08 00:27:32Z slouken $ */

/* A simple program to test the RTF rendering of the SDL_rtf library */

#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_rtf.h"

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

static const char *FontList[8];

/* Note, this is only one way of looking up fonts */
static int FontFamilyToIndex(RTF_FontFamily family)
{
    switch(family) {
        case RTF_FontDefault:
            return 0;
        case RTF_FontRoman:
            return 1;
        case RTF_FontSwiss:
            return 2;
        case RTF_FontModern:
            return 3;
        case RTF_FontScript:
            return 4;
        case RTF_FontDecor:
            return 5;
        case RTF_FontTech:
            return 6;
        case RTF_FontBidi:
            return 7;
        default:
            return 0;
    }
}

static Uint16 UTF8_to_UNICODE(const char *utf8, int *advance)
{
    int i = 0;
    Uint16 ch;

    ch = ((const unsigned char *)utf8)[i];
    if ( ch >= 0xF0 ) {
        ch  =  (Uint16)(utf8[i]&0x07) << 18;
        ch |=  (Uint16)(utf8[++i]&0x3F) << 12;
        ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
        ch |=  (Uint16)(utf8[++i]&0x3F);
    } else if ( ch >= 0xE0 ) {
        ch  =  (Uint16)(utf8[i]&0x3F) << 12;
        ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
        ch |=  (Uint16)(utf8[++i]&0x3F);
    } else if ( ch >= 0xC0 ) {
        ch  =  (Uint16)(utf8[i]&0x3F) << 6;
        ch |=  (Uint16)(utf8[++i]&0x3F);
    }
    *advance = (i+1);
    return ch;
}
static void *CreateFont(const char *name, RTF_FontFamily family, int charset, int size, int style)
{
    int index;
    TTF_Font *font;

    index = FontFamilyToIndex(family);
    if (!FontList[index])
        index = 0;

    font = TTF_OpenFont(FontList[index], size);
    if (font) {
        int TTF_style = TTF_STYLE_NORMAL;
        if ( style & RTF_FontBold )
            TTF_style |= TTF_STYLE_BOLD;
        if ( style & RTF_FontItalic )
            TTF_style |= TTF_STYLE_ITALIC;
        if ( style & RTF_FontUnderline )
            TTF_style |= TTF_STYLE_UNDERLINE;
        TTF_SetFontStyle(font, style);
    }

    /* FIXME: What do we do with the character set? */

    return font;
}

static int GetLineSpacing(void *_font)
{
    TTF_Font *font = (TTF_Font *)_font;
    return TTF_FontLineSkip(font);
}

static int GetCharacterOffsets(void *_font, const char *text, int *byteOffsets, int *pixelOffsets, int maxOffsets)
{
    TTF_Font *font = (TTF_Font *)_font;
    int i = 0;
    int bytes = 0;
    int pixels = 0;
    int advance;
    Uint16 ch;
    while ( *text && i < maxOffsets ) {
        byteOffsets[i] = bytes;
        pixelOffsets[i] = pixels;
        ++i;

        ch = UTF8_to_UNICODE(text, &advance);
        text += advance;
        bytes += advance;
        TTF_GlyphMetrics(font, ch, NULL, NULL, NULL, NULL, &advance);
        pixels += advance;
    }
    if ( i < maxOffsets ) {
        byteOffsets[i] = bytes;
        pixelOffsets[i] = pixels;
    }
    return i;
}

static SDL_Surface *RenderText(void *_font, const char *text, SDL_Color fg)
{
    TTF_Font *font = (TTF_Font *)_font;
    return TTF_RenderUTF8_Blended(font, text, fg);
}

static void FreeFont(void *_font)
{
    TTF_Font *font = (TTF_Font *)_font;
    TTF_CloseFont(font);
}

static void LoadRTF(RTF_Context *ctx, const char *file)
{
    if ( RTF_Load(ctx, file) < 0 ) {
        fprintf(stderr, "Couldn't load %s: %s\n", file, RTF_GetError());
        return;
    }
    SDL_WM_SetCaption(RTF_GetTitle(ctx), file);
}

static void PrintUsage(const char *argv0)
{
    printf("Usage: %s -fdefault font.ttf [-froman font.ttf] [-fswiss font.ttf] [-fmodern font.ttf] [-fscript font.ttf] [-fdecor font.ttf] [-ftech font.ttf] file.rtf\n", argv0);
}

static void cleanup(int exitcode)
{
    TTF_Quit();
    SDL_Quit();
    exit(exitcode);
}

int main(int argc, char *argv[])
{
    int i, start, stop;
    int done;
    int height;
    int offset;
    SDL_Surface *screen;
    RTF_Context *ctx;
    RTF_FontEngine fontEngine;
    Uint32 white;
    Uint8 *keystate;

    /* Parse command line arguments */
    for ( i = 1; i < argc; ++i ) {
        if ( strcmp(argv[i], "-fdefault") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontDefault)] = argv[++i];
        } else if ( strcmp(argv[i], "-froman") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontRoman)] = argv[++i];
        } else if ( strcmp(argv[i], "-fswiss") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontSwiss)] = argv[++i];
        } else if ( strcmp(argv[i], "-fmodern") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontModern)] = argv[++i];
        } else if ( strcmp(argv[i], "-fscript") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontScript)] = argv[++i];
        } else if ( strcmp(argv[i], "-fdecor") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontDecor)] = argv[++i];
        } else if ( strcmp(argv[i], "-ftech") == 0 ) {
            FontList[FontFamilyToIndex(RTF_FontTech)] = argv[++i];
        } else {
            break;
        }
    }
    start = i;
    stop = (argc-1);
    if ( !FontList[0] || (start > stop) ) {
        PrintUsage(argv[0]);
        return(1);
    }

    /* Initialize the SDL library */
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return(2);
    }

    /* Initialize the TTF library */
    if ( TTF_Init() < 0 ) {
        fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
        SDL_Quit();
        return(3);
    }

    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 0, SDL_RESIZABLE);
    if ( screen == NULL ) {
        fprintf(stderr, "Couldn't set %dx%d video mode: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        cleanup(4);
    }
    white = SDL_MapRGB(screen->format, 255, 255, 255);

    /* Create and load the RTF document */
    fontEngine.version = RTF_FONT_ENGINE_VERSION;
    fontEngine.CreateFont = CreateFont;
    fontEngine.GetLineSpacing = GetLineSpacing;
    fontEngine.GetCharacterOffsets = GetCharacterOffsets;
    fontEngine.RenderText = RenderText;
    fontEngine.FreeFont = FreeFont;
    ctx = RTF_CreateContext(&fontEngine);
    if ( ctx == NULL ) {
        fprintf(stderr, "Couldn't create RTF context: %s\n", RTF_GetError());
        cleanup(5);
    }
    LoadRTF(ctx, argv[i]);

    /* Render the document to the screen */
    done = 0;
    offset = 0;
    height = RTF_GetHeight(ctx, screen->w);
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_VIDEORESIZE) {
                float ratio = (float)offset / height;
                screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, SDL_RESIZABLE);
                height = RTF_GetHeight(ctx, screen->w);
                offset = (int)(ratio * height);
            }
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        done = 1;
                        break;
                    case SDLK_LEFT:
                        if ( i > start ) {
                            --i;
                            LoadRTF(ctx, argv[i]);
                            offset = 0;
                            height = RTF_GetHeight(ctx, screen->w);
                        }
                        break;
                    case SDLK_RIGHT:
                        if ( i < stop ) {
                            ++i;
                            LoadRTF(ctx, argv[i]);
                            offset = 0;
                            height = RTF_GetHeight(ctx, screen->w);
                        }
                        break;
                    case SDLK_HOME:
                        offset = 0;
                        break;
                    case SDLK_END:
                        offset = (height - screen->h);
                        break;
                    case SDLK_PAGEUP:
                        offset -= screen->h;
                        if ( offset < 0 )
                            offset = 0;
                        break;
                    case SDLK_PAGEDOWN:
                    case SDLK_SPACE:
                        offset += screen->h;
                        if ( offset > (height - screen->h) )
                            offset = (height - screen->h);
                        break;
                    default:
                        break;
                }
            }
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        keystate = SDL_GetKeyState(NULL);
        if ( keystate[SDLK_UP] ) {
            offset -= 1;
            if ( offset < 0 )
                offset = 0;
        }
        if ( keystate[SDLK_DOWN] ) {
            offset += 1;
            if ( offset > (height - screen->h) )
                offset = (height - screen->h);
        }
        SDL_FillRect(screen, NULL, white);
        RTF_Render(ctx, screen, NULL, offset);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
    }

    /* Clean up and exit */
    RTF_FreeContext(ctx);
    cleanup(0);

    /* Not reached, but fixes compiler warnings */
    return 0;
}
