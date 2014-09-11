/*
    SDL_rtf:  A companion library to SDL for displaying RTF format text
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

/* $Id: SDL_rtfreadr.c 4211 2008-12-08 00:27:32Z slouken $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "SDL_rtf.h"
#include "SDL_rtfreadr.h"

#include "rtftype.h"
#include "rtfdecl.h"

/* static function prototypes */
static int TwipsToPixels(int twips);
static RTF_Surface *CreateSurface(RTF_Context *ctx,
        RTF_TextBlock *textBlock, int offset, int numChars);
static int TextWithinWidth(RTF_TextBlock *textBlock, int offset,
        int width, int *wrapped);
static int ReflowLine(RTF_Context *ctx, RTF_Line *line, int width);
static void RenderLine(RTF_Line *line, SDL_Surface *target,
        const SDL_Rect *rect, int yOffset);

/*
 * %%Function: RTF_CreateFont
 */
void *RTF_CreateFont(void *fontEngine, const char *name, int family,
        int charset, int size, int style)
{
    return ((RTF_FontEngine *) fontEngine)->CreateFont(name, family,
            charset, size / 2, style);
}

/*
 * %%Function: RTF_FreeFont
 */
void RTF_FreeFont(void *fontEngine, void *font)
{
    ((RTF_FontEngine *) fontEngine)->FreeFont(font);
}

/*
 * %%Function: RTF_FreeSurface
 */
void RTF_FreeSurface(void *surface)
{
    SDL_FreeSurface(surface);
}

/*
 * %%Function: RTF_CreateColor
 */
void *RTF_CreateColor(int r, int g, int b)
{
    SDL_Color *color = (SDL_Color *) malloc(sizeof(*color));
    if (!color)
        return NULL;
    color->r = r;
    color->g = g;
    color->b = b;
    color->unused = 0;
    return color;
}

/*
 * %%Function: RTF_FreeColor
 */
void RTF_FreeColor(void *color)
{
    if (color)
        free(color);
}

/*
 * %%Function: RTF_GetLineSpacing
 */
int RTF_GetLineSpacing(void *fontEngine, void *font)
{
    return ((RTF_FontEngine *) fontEngine)->GetLineSpacing(font);
}

/*
 * &&Function: RTF_GetCharacterOffsets
 */
int RTF_GetCharacterOffsets(void *fontEngine, void *font,
        const char *text, int *byteOffsets, int *pixelOffsets,
        int maxOffsets)
{
    return ((RTF_FontEngine *) fontEngine)->GetCharacterOffsets(font,
            text, byteOffsets, pixelOffsets, maxOffsets);
}

/*
 * %%Function: RTF_GetChar
 */
int RTF_GetChar(void *stream, unsigned char *c)
{
    return ((SDL_RWops *) stream)->read((SDL_RWops *) stream, c, 1, 1);
}

/*
 * %%Function: ecReflowText
 *
 * Reflow the text to a new width
 */
int ecReflowText(RTF_Context *ctx, int width)
{
    RTF_Line *line;

    if (ctx->displayWidth == width)
        return ecOK;

    /* Reflow the text to the new width */
    ctx->displayWidth = width;
    ctx->displayHeight = 0;
    for (line = ctx->start; line; line = line->next)
    {
        ctx->displayHeight += ReflowLine(ctx, line, width);
    }
    return ecOK;
}

/*
 * %%Function: ecReflowText
 *
 * Render the text to a surface
 */
int ecRenderText(RTF_Context *ctx, SDL_Surface *target,
        const SDL_Rect *rect, int yOffset)
{
    RTF_Line *line;
    SDL_Rect savedRect;

    ecReflowText(ctx, rect->w);

    SDL_GetClipRect(target, &savedRect);
    SDL_SetClipRect(target, rect);
    for (line = ctx->start; line && yOffset < rect->h; line = line->next)
    {
        if (yOffset + line->lineHeight > 0)
            RenderLine(line, target, rect, yOffset);
        yOffset += line->lineHeight;
    }
    SDL_SetClipRect(target, &savedRect);

    return ecOK;
}

static int TwipsToPixels(int twips)
{
    /* twips are 1/20 of a pointsize, calculate pixels at 72 dpi */
    return (((twips * 64 * 72 + (36 + 32 * 72)) / 72) / 20) / 64;
}

static RTF_Surface *CreateSurface(RTF_Context *ctx,
        RTF_TextBlock *textBlock, int offset, int numChars)
{
    RTF_Surface *surface = (RTF_Surface *) malloc(sizeof(*surface));
    SDL_Color color;

    if (surface)
    {
        char *text = &textBlock->text[textBlock->byteOffsets[offset]];
        char *end =
            &textBlock->text[textBlock->byteOffsets[offset + numChars]];
        char ch;

        ch = *end;
        *end = '\0';
        if (textBlock->color)
            color = *(SDL_Color *) textBlock->color;
        else
            memset(&color, 0, sizeof(color));
        surface->surface = ((RTF_FontEngine *) ctx->fontEngine)->RenderText(textBlock->font,
                text, color);
        *end = ch;
        if (!surface->surface)
        {
            free(surface);
            return (NULL);
        }
        surface->x = 0;
        surface->y = 0;
        surface->next = NULL;
    }
    return surface;
}

static int TextWithinWidth(RTF_TextBlock *textBlock, int offset,
        int width, int *wrapped)
{
    int num = 0;
    int wrapIndex;

    /* Fit as many characters as possible into the available width */
    *wrapped = 0;
    if (offset + num < textBlock->numChars)
    {
        while ((textBlock->pixelOffsets[offset + num + 1] -
                textBlock->pixelOffsets[offset]) <= width)
        {
            ++num;
            if (offset + num == textBlock->numChars)
                return num;
        }

        /* Do word wrapping */
        for (wrapIndex = offset + num - 1; wrapIndex > offset;
             --wrapIndex)
        {
            if (isspace(
                    textBlock->text[textBlock->byteOffsets[wrapIndex]]))
                break;
        }
        if (wrapIndex > offset)
        {
            num = wrapIndex - offset + 1;
            *wrapped = 1;
        }
    }
    return num;
}

static int ReflowLine(RTF_Context *ctx, RTF_Line *line, int width)
{
    while (line->startSurface)
    {
        RTF_Surface *surface = line->startSurface;
        line->startSurface = surface->next;
        SDL_FreeSurface(surface->surface);
        free(surface);
    }
    if (line->start)
    {
        int leftMargin = TwipsToPixels(line->pap.xaLeft);
        int rightMargin = TwipsToPixels(line->pap.xaRight);
        int tabStop = TwipsToPixels(720);
        RTF_TextBlock *textBlock;
        RTF_Surface *surface;
        RTF_Surface *rowStart = NULL;
        int lineHeight = 0;
        int lineWidth = TwipsToPixels(line->pap.xaFirst);

        width -= leftMargin;
        width -= rightMargin;
        line->lineWidth = 0;
        line->lineHeight = 0;
        for (textBlock = line->start; textBlock;
             textBlock = textBlock->next)
        {
            int num, wrapped, numChars = 0;
            int tab;

            for (tab = 0; tab < textBlock->tabs; ++tab)
            {
                int nextTab = (((leftMargin + lineWidth) / tabStop) +
                        1) * tabStop;
                lineWidth = (nextTab - leftMargin);
            }
            do
            {
                num = TextWithinWidth(textBlock, numChars,
                        (width - lineWidth), &wrapped);
                if (num > 0)
                {
                    surface = CreateSurface(ctx, textBlock, numChars,
                            num);
                    if (surface)
                    {
                        if (!rowStart)
                        {
                            rowStart = surface;
                        }
                        surface->x = leftMargin + lineWidth;
                        surface->y = line->lineHeight;
                        if (line->startSurface)
                            line->lastSurface->next = surface;
                        else
                            line->startSurface = surface;
                        line->lastSurface = surface;
                    }
                    if (lineHeight < textBlock->lineHeight)
                        lineHeight = textBlock->lineHeight;
                    lineWidth +=
                            (textBlock->pixelOffsets[numChars + num] -
                            textBlock->pixelOffsets[numChars]);
                    numChars += num;
                }
                if (wrapped)
                {
                    if (lineWidth > line->lineWidth)
                        line->lineWidth = lineWidth;
                    line->lineHeight += lineHeight;

                    if (line->pap.just == justC)
                    {
                        int offset = (width - lineWidth) / 2;

                        while (rowStart)
                        {
                            rowStart->x += offset;
                            rowStart = rowStart->next;
                        }
                    }
                    else if (line->pap.just == justR)
                    {
                        int offset = (leftMargin + width - lineWidth);

                        while (rowStart)
                        {
                            rowStart->x += offset;
                            rowStart = rowStart->next;
                        }
                    }
                    rowStart = NULL;

                    lineWidth = 0;
                    lineHeight = 0;
                }
            }
            while (num > 0);
        }
        if (lineWidth > line->lineWidth)
        {
            line->lineWidth = lineWidth;
        }
        line->lineHeight += lineHeight;

        if (line->pap.just == justC)
        {
            int offset = (width - lineWidth) / 2;

            while (rowStart)
            {
                rowStart->x += offset;
                rowStart = rowStart->next;
            }
        }
        else if (line->pap.just == justR)
        {
            int offset = (leftMargin + width - lineWidth);

            while (rowStart)
            {
                rowStart->x += offset;
                rowStart = rowStart->next;
            }
        }
    }
    return line->lineHeight;
}

static void RenderLine(RTF_Line *line, SDL_Surface *target,
        const SDL_Rect *rect, int yOffset)
{
    SDL_Rect srcRect;
    SDL_Rect dstRect;
    RTF_Surface *surface;

    for (surface = line->startSurface; surface; surface = surface->next)
    {
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = ((SDL_Surface *) surface->surface)->w;
        srcRect.h = ((SDL_Surface *) surface->surface)->h;

        dstRect.x = rect->x + surface->x;
        dstRect.y = rect->y + yOffset + surface->y;
        dstRect.w = ((SDL_Surface *) surface->surface)->w;
        dstRect.h = ((SDL_Surface *) surface->surface)->h;

        SDL_BlitSurface(surface->surface, &srcRect, target, &dstRect);
    }
}

