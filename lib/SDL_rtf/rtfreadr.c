/*
 * This file was adapted from Microsoft Rich Text Format Specification 1.6
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec.asp
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rtftype.h"
#include "rtfdecl.h"

/* static function prototypes */
static void FreeLine(RTF_Line *line);
static void FreeTextBlock(RTF_TextBlock *text);

/*
 * %%Function: ecAddFontEntry
 */
int ecAddFontEntry(RTF_Context *ctx, int number, const char *name,
        int family, int charset)
{
    RTF_FontEntry *entry = (RTF_FontEntry *) malloc(sizeof(*entry));

    if (!entry)
        return ecStackOverflow;

    entry->number = number;
    entry->name = strdup(name);
    entry->family = (RTF_FontFamily) family;
    entry->charset = charset;
    entry->fonts = NULL;
    entry->next = ctx->fontTable;
    ctx->fontTable = entry;
    return ecOK;
}

/*
 * %%Function: ecLookupFont
 */
void *ecLookupFont(RTF_Context *ctx)
{
    RTF_FontEntry *entry;
    RTF_Font *font;
    int size;
    int style;

    /* Figure out what size we should use */
    size = ctx->chp.fFontSize;
    if (!size)
        size = 24;

    /* Figure out what style we should use */
    style = RTF_FontNormal;
    if (ctx->chp.fBold)
        style |= RTF_FontBold;
    if (ctx->chp.fItalic)
        style |= RTF_FontItalic;
    if (ctx->chp.fUnderline)
        style |= RTF_FontUnderline;

    /* Search for the correct font */
    for (entry = ctx->fontTable; entry; entry = entry->next)
    {
        if (entry->number == ctx->chp.fFont)
            break;
    }
    if (!entry)
    {
        /* Search for the first default font */
        for (entry = ctx->fontTable; entry; entry = entry->next)
        {
            if (entry->family == RTF_FontDefault)
                break;
        }
    }
    if (!entry)
    {
        /* If we still didn't find a font, just use the first font */
        if (ctx->fontTable)
            entry = ctx->fontTable;
        else
            return NULL;
    }

    /* We found a font entry, now find the font */
    for (font = entry->fonts; font; font = font->next)
    {
        if (size == font->size && style == font->style)
            return font->font;
    }

    /* Create a new font entry */
    font = (RTF_Font *) malloc(sizeof(*font));
    if (!font)
        return NULL;

    font->font = RTF_CreateFont(ctx->fontEngine, entry->name,
            entry->family, entry->charset, size, style);
    if (!font->font)
    {
        free(font);
        return NULL;
    }
    font->size = size;
    font->style = style;
    font->next = entry->fonts;
    entry->fonts = font;
    return font->font;
}

/*
 * %%Function: ecClearFonts
 */
int ecClearFonts(RTF_Context *ctx)
{
    while (ctx->fontTable)
    {
        RTF_FontEntry *entry = ctx->fontTable;

        ctx->fontTable = entry->next;
        free(entry->name);
        while (entry->fonts)
        {
            RTF_Font *font = entry->fonts;

            entry->fonts = font->next;
            RTF_FreeFont(ctx->fontEngine, font->font);
            free(font);
        }
        free(entry);
    }
    return ecOK;
}

/*
 * %%Function: ecAddColorEntry
 */
int ecAddColorEntry(RTF_Context *ctx, int r, int g, int b)
{
    RTF_ColorEntry *ptr;
    RTF_ColorEntry *entry = (RTF_ColorEntry *) malloc(sizeof(*entry));

    if (!entry)
        return ecStackOverflow;

    entry->color = RTF_CreateColor(r, g, b);
    entry->r = r & 0xFF;
    entry->g = g & 0xFF;
    entry->b = b & 0xFF;
    entry->a = 0;
    entry->next = NULL;
    if (!ctx->colorTable)
        ctx->colorTable = entry;
    else
    {
        for (ptr = ctx->colorTable; ptr->next; ptr = ptr->next);
        ptr->next = entry;
    }
    return ecOK;
}

/*
 * %%Function: ecLookupColor
 */
void *ecLookupColor(RTF_Context *ctx)
{
    RTF_ColorEntry *ptr;
    int index = ctx->chp.fFgColor;

    for (ptr = ctx->colorTable; ptr && index > 0; ptr = ptr->next,
            index--);
    if (ptr && index >= 0)
        return ptr->color;
    return NULL;
}

/*
 * %%Function: ecClearColors
 */
int ecClearColors(RTF_Context *ctx)
{
    RTF_ColorEntry *e;
    RTF_ColorEntry *e2;
    for (e = ctx->colorTable; e; e = e2)
    {
        e2 = e->next;
        RTF_FreeColor(e->color);
        free(e);
    }
    return ecOK;
}

/*
 * %%Function: ecAddLine
 */
int ecAddLine(RTF_Context *ctx)
{
    RTF_Line *line;

    /* Lookup the current font */
    void *font = ecLookupFont(ctx);

    if (!font)
        return ecFontNotFound;

    line = (RTF_Line *) malloc(sizeof(*line));
    if (!line)
        return ecStackOverflow;

    line->pap = ctx->pap;
    line->lineWidth = 0;
    line->lineHeight = RTF_GetLineSpacing(ctx->fontEngine, font);
    line->tabs = 0;
    line->start = NULL;
    line->last = NULL;
    line->startSurface = NULL;
    line->lastSurface = NULL;
    line->next = NULL;

#ifdef DEBUG_RTF
    fprintf(stderr, "Added line ---------------------\n");
#endif
    if (ctx->start)
        ctx->last->next = line;
    else
        ctx->start = line;
    ctx->last = line;
    return ecOK;
}

/*
 * %%Function: ecAddTab
 */
int ecAddTab(RTF_Context *ctx)
{
    RTF_Line *line;

    /* Add the tabs to the last line added */
    if (!ctx->last)
    {
        int status = ecAddLine(ctx);

        if (status != ecOK)
            return status;
    }
    line = ctx->last;

    ++line->tabs;
    return ecOK;
}

/*
 * %%Function: ecAddText
 */
int ecAddText(RTF_Context *ctx, const char *text)
{
    RTF_Line *line;
    RTF_TextBlock *textBlock;
    int numChars;

    /* Lookup the current font */
    void *font = ecLookupFont(ctx);

    if (!font)
        return ecFontNotFound;

    /* Add the text to the last line added */
    if (!ctx->last)
    {
        int status = ecAddLine(ctx);

        if (status != ecOK)
            return status;
    }
    line = ctx->last;

    textBlock = (RTF_TextBlock *) malloc(sizeof(*textBlock));
    if (!textBlock)
        return ecStackOverflow;

    textBlock->font = font;
    textBlock->color = ecLookupColor(ctx);
    numChars = strlen(text) + 1;
    textBlock->tabs = line->tabs;
    textBlock->text = strdup(text);
    textBlock->byteOffsets = (int *) malloc(numChars * sizeof(int));
    textBlock->pixelOffsets = (int *) malloc(numChars * sizeof(int));
    if (!textBlock->text || !textBlock->byteOffsets ||
            !textBlock->pixelOffsets)
    {
        FreeTextBlock(textBlock);
        return ecStackOverflow;
    }
    textBlock->numChars = RTF_GetCharacterOffsets(ctx->fontEngine, font,
            text, textBlock->byteOffsets, textBlock->pixelOffsets,
            numChars);
    textBlock->lineHeight = RTF_GetLineSpacing(ctx->fontEngine, font);
    textBlock->next = NULL;

#ifdef DEBUG_RTF
    fprintf(stderr, "Added text: '%s'\n", text);
#endif
    line->pap = ctx->pap;
    line->tabs = 0;
    if (line->start)
        line->last->next = textBlock;
    else
        line->start = textBlock;
    line->last = textBlock;
    return ecOK;
}

/*
 * %%Function: ecClearLines
 */
int ecClearLines(RTF_Context *ctx)
{
    while (ctx->start)
    {
        RTF_Line *line = ctx->start;

        ctx->start = line->next;
        FreeLine(line);
    }
    ctx->last = NULL;
    return ecOK;
}

/*
 * %%Function: ecClearContext
 */
int ecClearContext(RTF_Context *ctx)
{
    if (ctx->data)
    {
        free(ctx->data);
        ctx->data = NULL;
        ctx->datapos = 0;
        ctx->datamax = 0;
    }
    memset(ctx->values, 0, sizeof(ctx->values));

    ecClearFonts(ctx);
    ecClearColors(ctx);

    if (ctx->title)
    {
        free(ctx->title);
        ctx->title = NULL;
    }
    if (ctx->subject)
    {
        free(ctx->subject);
        ctx->subject = NULL;
    }
    if (ctx->author)
    {
        free(ctx->author);
        ctx->author = NULL;
    }

    memset(&ctx->chp, 0, sizeof(ctx->chp));
    memset(&ctx->pap, 0, sizeof(ctx->pap));
    memset(&ctx->sep, 0, sizeof(ctx->sep));
    memset(&ctx->dop, 0, sizeof(ctx->dop));

    ecClearLines(ctx);

    ctx->displayWidth = 0;
    ctx->displayHeight = 0;

    return ecOK;
}

/*
 * %%Function: ecRtfGetChar
 */
int ecRtfGetChar(RTF_Context *ctx, int *ch)
{
    if (ctx->nextch >= 0)
    {
        *ch = ctx->nextch;
        ctx->nextch = -1;
    }
    else
    {
        unsigned char c;

        if (RTF_GetChar(ctx->stream, &c) != 1)
            return ecEndOfFile;
        *ch = c;
    }
    return ecOK;
}

/*
 * %%Function: ecRtfUngetChar
 */
int ecRtfUngetChar(RTF_Context *ctx, int ch)
{
    ctx->nextch = ch;
    return ecOK;
}

/*
 * %%Function: ecRtfParse
 *
 * Step 1:
 * Isolate RTF keywords and send them to ecParseRtfKeyword;
 * Push and pop state at the start and end of RTF groups;
 * Send text to ecParseChar for further processing.
 */
int ecRtfParse(RTF_Context *ctx)
{
    int ch;
    int ec;
    int cNibble = 2;
    int b = 0;

    while (ecRtfGetChar(ctx, &ch) == ecOK)
    {
        if (ctx->cGroup < 0)
            return ecStackUnderflow;
        if (ctx->ris == risBin) /* if we're parsing binary data, handle it directly */
        {
            if ((ec = ecParseChar(ctx, ch)) != ecOK)
                return ec;
        }
        else
        {
            switch (ch)
            {
                case '{':
                    ecProcessData(ctx);
                    if ((ec = ecPushRtfState(ctx)) != ecOK)
                        return ec;
                    break;
                case '}':
                    ecProcessData(ctx);
                    if ((ec = ecPopRtfState(ctx)) != ecOK)
                        return ec;
                    break;
                case '\\':
                    ecProcessData(ctx);
                    if ((ec = ecParseRtfKeyword(ctx)) != ecOK)
                        return ec;
                    break;
                case 0x0d:
                case 0x0a:     /* cr and lf are noise characters... */
                    break;
                default:
                    if (ctx->ris == risNorm)
                    {
                        if ((ec = ecParseChar(ctx, ch)) != ecOK)
                            return ec;
                    }
                    else
                    {           /* parsing hex data */
                        if (ctx->ris != risHex)
                            return ecAssertion;
                        b = b << 4;
                        if (isdigit(ch))
                            b += (char) ch - '0';
                        else
                        {
                            if (islower(ch))
                            {
                                if (ch < 'a' || ch > 'f')
                                    return ecInvalidHex;
                                b += 10 + (ch - 'a');
                            }
                            else
                            {
                                if (ch < 'A' || ch > 'F')
                                    return ecInvalidHex;
                                b += 10 + (ch - 'A');
                            }
                        }
                        cNibble--;
                        if (!cNibble)
                        {
                            if ((ec = ecParseChar(ctx, b)) != ecOK)
                                return ec;
                            cNibble = 2;
                            b = 0;
                            ctx->ris = risNorm;
                        }
                    }           /* end else (ris != risNorm) */
                    break;
            }                   /* switch */
        }                       /* else (ris != risBin) */
    }                           /* while */
    if (ctx->cGroup < 0)
        return ecStackUnderflow;
    if (ctx->cGroup > 0)
        return ecUnmatchedBrace;
    return ecOK;
}

/*
 * %%Function: ecPushRtfState
 *
 * Save relevant info on a linked list of SAVE structures.
 */
int ecPushRtfState(RTF_Context *ctx)
{
    SAVE *psaveNew = malloc(sizeof(SAVE));

    if (!psaveNew)
        return ecStackOverflow;

    psaveNew->pNext = ctx->psave;
    psaveNew->chp = ctx->chp;
    psaveNew->pap = ctx->pap;
    psaveNew->sep = ctx->sep;
    psaveNew->dop = ctx->dop;
    psaveNew->rds = ctx->rds;
    psaveNew->ris = ctx->ris;
    ctx->ris = risNorm;
    ctx->psave = psaveNew;
    ctx->cGroup++;
    return ecOK;
}

/*
 * %%Function: ecPopRtfState
 *
 * If we're ending a destination (that is, the destination is changing),
 * call ecEndGroupAction.
 * Always restore relevant info from the top of the SAVE list.
 */
int ecPopRtfState(RTF_Context *ctx)
{
    SAVE *psaveOld;
    int ec;

    if (!ctx->psave)
        return ecStackUnderflow;

    if (ctx->rds != ctx->psave->rds)
    {
        if ((ec = ecEndGroupAction(ctx, ctx->rds)) != ecOK)
            return ec;
    }

    ctx->chp = ctx->psave->chp;
    ctx->pap = ctx->psave->pap;
    ctx->sep = ctx->psave->sep;
    ctx->dop = ctx->psave->dop;
    ctx->rds = ctx->psave->rds;
    ctx->ris = ctx->psave->ris;

    psaveOld = ctx->psave;
    ctx->psave = ctx->psave->pNext;
    ctx->cGroup--;
    free(psaveOld);
    return ecOK;
}

/*
 * %%Function: ecParseRtfKeyword
 *
 * Step 2:
 * get a control word (and its associated value) and
 * call ecTranslateKeyword to dispatch the control.
 */
int ecParseRtfKeyword(RTF_Context *ctx)
{
    int ch;
    char fParam = fFalse;
    char fNeg = fFalse;
    int param = 0;
    char *pch;
    char szKeyword[30];
    char szParameter[20];

    szKeyword[0] = '\0';
    szParameter[0] = '\0';
    if (ecRtfGetChar(ctx, &ch) != ecOK)
        return ecEndOfFile;
    if (!isalpha(ch))           /* a control symbol; no delimiter. */
    {
        szKeyword[0] = (char) ch;
        szKeyword[1] = '\0';
        return ecTranslateKeyword(ctx, szKeyword, 0, fParam);
    }
    for (pch = szKeyword; isalpha(ch);)
    {
        *pch++ = (char) ch;
        if (ecRtfGetChar(ctx, &ch) != ecOK)
            return ecEndOfFile;
    }
    *pch = '\0';
    if (ch == '-')
    {
        fNeg = fTrue;
        if (ecRtfGetChar(ctx, &ch) != ecOK)
            return ecEndOfFile;
    }
    if (isdigit(ch))
    {
        fParam = fTrue;         /* a digit after the control means we have a parameter */
        for (pch = szParameter; isdigit(ch);)
        {
            *pch++ = (char) ch;
            if (ecRtfGetChar(ctx, &ch) != ecOK)
                return ecEndOfFile;
        }
        *pch = '\0';
        param = atoi(szParameter);
        if (fNeg)
            param = -param;
        ctx->lParam = atol(szParameter);
        if (fNeg)
            ctx->lParam = -ctx->lParam;
    }
    if (ch != ' ')
        ecRtfUngetChar(ctx, ch);
    return ecTranslateKeyword(ctx, szKeyword, param, fParam);
}

/*
 * %%Function: ecParseChar
 *
 * Route the character to the appropriate destination stream.
 */
int ecParseChar(RTF_Context *ctx, int ch)
{
    if (ctx->ris == risBin && --ctx->cbBin <= 0)
        ctx->ris = risNorm;
    switch (ctx->rds)
    {
        case rdsNorm:
            /* Output a character. Properties are valid at this point. */
            if (ch == '\t')
            {
                return ecTabstop(ctx);
            }
            if (ch == '\r')
            {
                return ecLinebreak(ctx);
            }
            if (ch == '\n')
            {
                return ecParagraph(ctx);
            }
            return ecPrintChar(ctx, ch);
        case rdsSkip:
            /* Toss this character. */
            return ecOK;
        case rdsFontTable:
            if (ch == ';')
            {
                ctx->data[ctx->datapos] = '\0';
                ecAddFontEntry(ctx, ctx->chp.fFont, ctx->data,
                               (FFAM) ctx->values[0],
                               ctx->chp.fFontCharset);
                ctx->datapos = 0;
                ctx->values[0] = 0;
            }
            else
            {
                return ecPrintChar(ctx, ch);
            }
            return ecOK;
        case rdsColorTable:
            if (ch == ';')
            {
                ecAddColorEntry(ctx, ctx->values[0], ctx->values[1],
                                ctx->values[2]);
                ctx->values[0] = 0;
                ctx->values[1] = 0;
                ctx->values[2] = 0;
            }
            return ecOK;
        case rdsTitle:
        case rdsSubject:
        case rdsAuthor:
            return ecPrintChar(ctx, ch);
        default:
            /* handle other destinations.... */
            return ecOK;
    }
}

/*
 * %%Function: ecPrintChar
 *
 * Add a character to the output text
 */
int ecPrintChar(RTF_Context *ctx, int ch)
{
    if (ctx->datapos >= (ctx->datamax - 4))
    {
        ctx->datamax += 256;    /* 256 byte chunk size */
        ctx->data = (char *) realloc(ctx->data, ctx->datamax);
        if (!ctx->data)
        {
            return ecStackOverflow;
        }
    }
    /* Some common characters aren't in TrueType font maps */
    if (ch == 147 || ch == 148)
        ch = '"';

    /* Convert character into UTF-8 */
    if (ch <= 0x7fUL)
    {
        ctx->data[ctx->datapos++] = ch;
    }
    else if (ch <= 0x7ffUL)
    {
        ctx->data[ctx->datapos++] = 0xc0 | (ch >> 6);
        ctx->data[ctx->datapos++] = 0x80 | (ch & 0x3f);
    }
    else if (ch <= 0xffffUL)
    {
        ctx->data[ctx->datapos++] = 0xe0 | (ch >> 12);
        ctx->data[ctx->datapos++] = 0x80 | ((ch >> 6) & 0x3f);
        ctx->data[ctx->datapos++] = 0x80 | (ch & 0x3f);
    }
    else
    {
        ctx->data[ctx->datapos++] = 0xf0 | (ch >> 18);
        ctx->data[ctx->datapos++] = 0x80 | ((ch >> 12) & 0x3f);
        ctx->data[ctx->datapos++] = 0x80 | ((ch >> 6) & 0x3f);
        ctx->data[ctx->datapos++] = 0x80 | (ch & 0x3f);
    }
    return ecOK;
}

/*
 * %%Function: ecProcessData
 *
 * Flush the output text
 */
int ecProcessData(RTF_Context *ctx)
{
    int status = ecOK;

    if (ctx->rds == rdsNorm)
    {
        if (ctx->datapos > 0)
        {
            ctx->data[ctx->datapos] = '\0';
            status = ecAddText(ctx, ctx->data);
            ctx->datapos = 0;
        }
    }
    return status;
}

/*
 * %%Function: ecTabstop
 *
 * Flush the output text and add a tabstop
 */
int ecTabstop(RTF_Context *ctx)
{
    int status;

    status = ecProcessData(ctx);
    if (status == ecOK)
        status = ecAddTab(ctx);
    return status;
}

/*
 * %%Function: ecLinebreak
 *
 * Flush the output text and move to the next line
 */
int ecLinebreak(RTF_Context *ctx)
{
    int status;

    status = ecProcessData(ctx);
    if (status == ecOK)
        status = ecAddLine(ctx);
    return status;
}

/*
 * %%Function: ecParagraph
 *
 * Flush the output text and start a new paragraph
 */
int ecParagraph(RTF_Context *ctx)
{
    return ecLinebreak(ctx);
}

static void FreeLine(RTF_Line *line)
{
    while (line->startSurface)
    {
        RTF_Surface *surface = line->startSurface;
        line->startSurface = surface->next;
        RTF_FreeSurface(surface->surface);
        free(surface);
    }
    while (line->start)
    {
        RTF_TextBlock *text = line->start;

        line->start = text->next;
        FreeTextBlock(text);
    }
    free(line);
}

static void FreeTextBlock(RTF_TextBlock *text)
{
    if (text->text)
        free(text->text);
    if (text->byteOffsets)
        free(text->byteOffsets);
    if (text->pixelOffsets)
        free(text->pixelOffsets);
    free(text);
}

