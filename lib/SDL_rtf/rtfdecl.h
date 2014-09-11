/*
 * This file was adapted from Microsoft Rich Text Format Specification 1.6
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec.asp
 */

#ifndef _RTFDECL_H
#define _RTFDECL_H

#include "rtftype.h"

/* rtfactn.c prototypes */

int ecApplyPropChange(RTF_Context *ctx, IPROP iprop, int val);
int ecParseSpecialProperty(RTF_Context *ctx, IPROP iprop, int val);
int ecTranslateKeyword(RTF_Context *ctx, char *szKeyword, int param,
        bool fParam);
int ecChangeDest(RTF_Context *ctx, IDEST idest);
int ecEndGroupAction(RTF_Context *ctx, RDS rds);
int ecParseSpecialKeyword(RTF_Context *ctx, IPFN ipfn);
int ecParseHexByte(RTF_Context * ctx);

/* rtfreadr.c prototypes */

int ecAddFontEntry(RTF_Context *ctx, int number, const char *name,
        int family, int charset);
void *ecLookupFont(RTF_Context *ctx);
int ecClearFonts(RTF_Context *ctx);

int ecAddColorEntry(RTF_Context *ctx, int r, int g, int b);
void *ecLookupColor(RTF_Context *ctx);
int ecClearColors(RTF_Context *ctx);

int ecAddLine(RTF_Context *ctx);
int ecAddTab(RTF_Context *ctx);
int ecAddText(RTF_Context *ctx, const char *text);

int ecClearLines(RTF_Context *ctx);
int ecClearContext(RTF_Context *ctx);

int ecRtfGetChar(RTF_Context *ctx, int *ch);
int ecRtfUngetChar(RTF_Context *ctx, int ch);

int ecRtfParse(RTF_Context *ctx);
int ecPushRtfState(RTF_Context *ctx);
int ecPopRtfState(RTF_Context *ctx);
int ecParseRtfKeyword(RTF_Context *ctx);
int ecParseChar(RTF_Context *ctx, int c);
int ecPrintChar(RTF_Context *ctx, int ch);

int ecProcessData(RTF_Context *ctx);
int ecTabstop(RTF_Context *ctx);
int ecLinebreak(RTF_Context *ctx);
int ecParagraph(RTF_Context *ctx);

/* custom rtfreader.c prototypes (defined per library) */

void *RTF_CreateFont(void *fontEngine, const char *name, int family,
        int charset, int size, int style);
void RTF_FreeFont(void *fontEngine, void *font);
void *RTF_CreateColor(int r, int g, int b);
void RTF_FreeColor(void *color);
int RTF_GetLineSpacing(void *fontEngine, void *font);
int RTF_GetCharacterOffsets(void *fontEngine, void *font,
        const char *text, int *byteOffsets, int *pixelOffsets,
        int maxOffsets);
void RTF_FreeSurface(void *surface);
int RTF_GetChar(void *stream, unsigned char *c);

/* RTF parser error codes */

#define ecOK              0  /* Everything's fine! */
#define ecStackUnderflow  1  /* Unmatched '}' */
#define ecStackOverflow   2  /* Too many '{' -- memory exhausted */
#define ecUnmatchedBrace  3  /* RTF ended during an open group. */
#define ecInvalidHex      4  /* invalid hex character found in data */
#define ecBadTable        5  /* RTF table (sym or prop) invalid */
#define ecAssertion       6  /* Assertion failure */
#define ecEndOfFile       7  /* End of file reached while reading RTF */
#define ecFontNotFound    8  /* Couldn't find font for text */

#endif /* _RTFDECL_H */

