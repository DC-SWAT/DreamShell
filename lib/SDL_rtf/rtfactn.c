/*
 * This file was adapted from Microsoft Rich Text Format Specification 1.6
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnrtfspec/html/rtfspec.asp
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include "rtftype.h"
#include "rtfdecl.h"

/* RTF parser tables */

/* Property descriptions */
PROP rgprop[ipropMax] =
{
    {actnSpec, propChp, 0},     /* ipropFontFamily */
    {actnWord, propChp, offsetof(CHP, fFontCharset)},   /* ipropFontCharset */
    {actnSpec, propChp, 0},     /* ipropColorRed */
    {actnSpec, propChp, 0},     /* ipropColorGreen */
    {actnSpec, propChp, 0},     /* ipropColorBlue */
    {actnWord, propChp, offsetof(CHP, fFont)},      /* ipropFont */
    {actnWord, propChp, offsetof(CHP, fFontSize)},  /* ipropFontSize */
    {actnByte, propChp, offsetof(CHP, fBgColor)},   /* ipropBgColor */
    {actnByte, propChp, offsetof(CHP, fFgColor)},   /* ipropFgColor */
    {actnByte, propChp, offsetof(CHP, fBold)},      /* ipropBold */
    {actnByte, propChp, offsetof(CHP, fItalic)},    /* ipropItalic */
    {actnByte, propChp, offsetof(CHP, fUnderline)}, /* ipropUnderline */
    {actnWord, propPap, offsetof(PAP, xaLeft)},     /* ipropLeftInd */
    {actnWord, propPap, offsetof(PAP, xaRight)},    /* ipropRightInd */
    {actnWord, propPap, offsetof(PAP, xaFirst)},    /* ipropFirstInd */
    {actnWord, propSep, offsetof(SEP, cCols)},      /* ipropCols */
    {actnWord, propSep, offsetof(SEP, xaPgn)},      /* ipropPgnX */
    {actnWord, propSep, offsetof(SEP, yaPgn)},      /* ipropPgnY */
    {actnWord, propDop, offsetof(DOP, xaPage)},     /* ipropXaPage */
    {actnWord, propDop, offsetof(DOP, yaPage)},     /* ipropYaPage */
    {actnWord, propDop, offsetof(DOP, xaLeft)},     /* ipropXaLeft */
    {actnWord, propDop, offsetof(DOP, xaRight)},    /* ipropXaRight */
    {actnWord, propDop, offsetof(DOP, yaTop)},      /* ipropYaTop */
    {actnWord, propDop, offsetof(DOP, yaBottom)},   /* ipropYaBottom */
    {actnWord, propDop, offsetof(DOP, pgnStart)},   /* ipropPgnStart */
    {actnByte, propSep, offsetof(SEP, sbk)},        /* ipropSbk */
    {actnByte, propSep, offsetof(SEP, pgnFormat)},  /* ipropPgnFormat */
    {actnByte, propDop, offsetof(DOP, fFacingp)},   /* ipropFacingp */
    {actnByte, propDop, offsetof(DOP, fLandscape)}, /* ipropLandscape */
    {actnByte, propPap, offsetof(PAP, just)},       /* ipropJust */
    {actnSpec, propPap, 0},                         /* ipropPard */
    {actnSpec, propChp, 0},                         /* ipropPlain */
    {actnSpec, propSep, 0},                         /* ipropSectd */
};

/* Keyword descriptions */
SYM rgsymRtf[] =
{
    /* keyword, dflt, fPassDflt, kwd, idx */
    {"fonttbl", 0, fFalse, kwdDest, idestFontTable},
    {"fnil", fnil, fTrue, kwdProp, ipropFontFamily},
    {"froman", froman, fTrue, kwdProp, ipropFontFamily},
    {"fswiss", fswiss, fTrue, kwdProp, ipropFontFamily},
    {"fmodern", fmodern, fTrue, kwdProp, ipropFontFamily},
    {"fscript", fscript, fTrue, kwdProp, ipropFontFamily},
    {"fdecor", fdecor, fTrue, kwdProp, ipropFontFamily},
    {"ftech", ftech, fTrue, kwdProp, ipropFontFamily},
    {"fbidi", fbidi, fTrue, kwdProp, ipropFontFamily},
    {"fcharset", 0, fFalse, kwdProp, ipropFontCharset},
    {"colortbl", 0, fFalse, kwdDest, idestColorTable},
    {"red", 0, fFalse, kwdProp, ipropColorRed},
    {"green", 0, fFalse, kwdProp, ipropColorGreen},
    {"blue", 0, fFalse, kwdProp, ipropColorBlue},
    {"info", 0, fFalse, kwdDest, idestInfo},
    {"title", 0, fFalse, kwdDest, idestTitle},
    {"subject", 0, fFalse, kwdDest, idestSubject},
    {"author", 0, fFalse, kwdDest, idestAuthor},
    {"f", 0, fFalse, kwdProp, ipropFont},
    {"fs", 24, fFalse, kwdProp, ipropFontSize},
    {"cb", 1, fFalse, kwdProp, ipropBgColor},
    {"cf", 1, fFalse, kwdProp, ipropFgColor},
    {"b", 1, fFalse, kwdProp, ipropBold},
    {"ul", 1, fFalse, kwdProp, ipropUnderline},
    {"ulnone", 0, fTrue, kwdProp, ipropUnderline},
    {"i", 1, fFalse, kwdProp, ipropItalic},
    {"li", 0, fFalse, kwdProp, ipropLeftInd},
    {"ri", 0, fFalse, kwdProp, ipropRightInd},
    {"fi", 0, fFalse, kwdProp, ipropFirstInd},
    {"cols", 1, fFalse, kwdProp, ipropCols},
    {"sbknone", sbkNon, fTrue, kwdProp, ipropSbk},
    {"sbkcol", sbkCol, fTrue, kwdProp, ipropSbk},
    {"sbkeven", sbkEvn, fTrue, kwdProp, ipropSbk},
    {"sbkodd", sbkOdd, fTrue, kwdProp, ipropSbk},
    {"sbkpage", sbkPg, fTrue, kwdProp, ipropSbk},
    {"pgnx", 0, fFalse, kwdProp, ipropPgnX},
    {"pgny", 0, fFalse, kwdProp, ipropPgnY},
    {"pgndec", pgDec, fTrue, kwdProp, ipropPgnFormat},
    {"pgnucrm", pgURom, fTrue, kwdProp, ipropPgnFormat},
    {"pgnlcrm", pgLRom, fTrue, kwdProp, ipropPgnFormat},
    {"pgnucltr", pgULtr, fTrue, kwdProp, ipropPgnFormat},
    {"pgnlcltr", pgLLtr, fTrue, kwdProp, ipropPgnFormat},
    {"qc", justC, fTrue, kwdProp, ipropJust},
    {"ql", justL, fTrue, kwdProp, ipropJust},
    {"qr", justR, fTrue, kwdProp, ipropJust},
    {"qj", justF, fTrue, kwdProp, ipropJust},
    {"paperw", 12240, fFalse, kwdProp, ipropXaPage},
    {"paperh", 15480, fFalse, kwdProp, ipropYaPage},
    {"margl", 1800, fFalse, kwdProp, ipropXaLeft},
    {"margr", 1800, fFalse, kwdProp, ipropXaRight},
    {"margt", 1440, fFalse, kwdProp, ipropYaTop},
    {"margb", 1440, fFalse, kwdProp, ipropYaBottom},
    {"pgnstart", 1, fTrue, kwdProp, ipropPgnStart},
    {"facingp", 1, fTrue, kwdProp, ipropFacingp},
    {"landscape", 1, fTrue, kwdProp, ipropLandscape},
    {"line", 0, fFalse, kwdChar, '\n'},
    {"par", 0, fFalse, kwdChar, '\n'},
    {"\0x0a", 0, fFalse, kwdChar, '\n'},
    {"\r", 0, fFalse, kwdChar, '\n'},
    {"\0x0d", 0, fFalse, kwdChar, '\n'},
    {"\n", 0, fFalse, kwdChar, '\n'},
    {"tab", 0, fFalse, kwdChar, '\t'},
    {"ldblquote", 0, fFalse, kwdChar, '"'},
    {"rdblquote", 0, fFalse, kwdChar, '"'},
    {"bin", 0, fFalse, kwdSpec, ipfnBin},
    {"*", 0, fFalse, kwdSpec, ipfnSkipDest},
    {"'", 0, fFalse, kwdSpec, ipfnHex},
    {"bkmkend", 0, fFalse, kwdDest, idestSkip},
    {"bkmkstart", 0, fFalse, kwdDest, idestSkip},
    {"buptim", 0, fFalse, kwdDest, idestSkip},
    {"colortbl", 0, fFalse, kwdDest, idestSkip},
    {"comment", 0, fFalse, kwdDest, idestSkip},
    {"creatim", 0, fFalse, kwdDest, idestSkip},
    {"doccomm", 0, fFalse, kwdDest, idestSkip},
    {"fonttbl", 0, fFalse, kwdDest, idestSkip},
    {"footer", 0, fFalse, kwdDest, idestSkip},
    {"footerf", 0, fFalse, kwdDest, idestSkip},
    {"footerl", 0, fFalse, kwdDest, idestSkip},
    {"footerr", 0, fFalse, kwdDest, idestSkip},
    {"footnote", 0, fFalse, kwdDest, idestSkip},
    {"ftncn", 0, fFalse, kwdDest, idestSkip},
    {"ftnsep", 0, fFalse, kwdDest, idestSkip},
    {"ftnsepc", 0, fFalse, kwdDest, idestSkip},
    {"header", 0, fFalse, kwdDest, idestSkip},
    {"headerf", 0, fFalse, kwdDest, idestSkip},
    {"headerl", 0, fFalse, kwdDest, idestSkip},
    {"headerr", 0, fFalse, kwdDest, idestSkip},
    {"info", 0, fFalse, kwdDest, idestSkip},
    {"keywords", 0, fFalse, kwdDest, idestSkip},
    {"operator", 0, fFalse, kwdDest, idestSkip},
    {"pict", 0, fFalse, kwdDest, idestSkip},
    {"printim", 0, fFalse, kwdDest, idestSkip},
    {"private1", 0, fFalse, kwdDest, idestSkip},
    {"revtim", 0, fFalse, kwdDest, idestSkip},
    {"rxe", 0, fFalse, kwdDest, idestSkip},
    {"stylesheet", 0, fFalse, kwdDest, idestSkip},
    {"tc", 0, fFalse, kwdDest, idestSkip},
    {"txe", 0, fFalse, kwdDest, idestSkip},
    {"xe", 0, fFalse, kwdDest, idestSkip},
    {"{", 0, fFalse, kwdChar, '{'},
    {"}", 0, fFalse, kwdChar, '}'},
    {"\\", 0, fFalse, kwdChar, '\\'},
    {"pard", 0, fFalse, kwdProp, ipropPard},
    {"plain", 0, fFalse, kwdProp, ipropPlain},
    {"sectd", 0, fFalse, kwdProp, ipropSectd}
};
int isymMax = sizeof(rgsymRtf) / sizeof(SYM);

/*
 * %%Function: ecApplyPropChange
 *
 * Set the property identified by _iprop_ to the value _val_.
 *
 */
int ecApplyPropChange(RTF_Context *ctx, IPROP iprop, int val)
{
    char *pb;

    if (ctx->rds == rdsSkip)    /* If we're skipping text, */
        return ecOK;            /* don't do anything. */

    switch (rgprop[iprop].prop)
    {
        case propDop:
            pb = (char *) &ctx->dop;
            break;
        case propSep:
            pb = (char *) &ctx->sep;
            break;
        case propPap:
            pb = (char *) &ctx->pap;
            break;
        case propChp:
            pb = (char *) &ctx->chp;
            break;
        default:
            pb = NULL;
            if (rgprop[iprop].actn != actnSpec)
                return ecBadTable;
            break;
    }
    switch (rgprop[iprop].actn)
    {
        case actnByte:
            pb[rgprop[iprop].offset] = (unsigned char) val;
            break;
        case actnWord:
            (*(int *) (pb + rgprop[iprop].offset)) = val;
            break;
        case actnSpec:
            return ecParseSpecialProperty(ctx, iprop, val);
            break;
        default:
            return ecBadTable;
    }
    return ecOK;
}

/*
 * %%Function: ecParseSpecialProperty
 *
 * Set a property that requires code to evaluate.
 */
int ecParseSpecialProperty(RTF_Context *ctx, IPROP iprop, int val)
{
    switch (iprop)
    {
        case ipropFontFamily:
            ctx->values[0] = val;
            return ecOK;
        case ipropColorRed:
            ctx->values[0] = val;
            return ecOK;
        case ipropColorGreen:
            ctx->values[1] = val;
            return ecOK;
        case ipropColorBlue:
            ctx->values[2] = val;
            return ecOK;
        case ipropPard:
            memset(&ctx->pap, 0, sizeof(ctx->pap));
            return ecOK;
        case ipropPlain:
            memset(&ctx->chp, 0, sizeof(ctx->chp));
            return ecOK;
        case ipropSectd:
            memset(&ctx->sep, 0, sizeof(ctx->sep));
            return ecOK;
        default:
            return ecBadTable;
    }
    return ecBadTable;
}

/*
 * %%Function: ecTranslateKeyword.
 *
 * Step 3.
 * Search rgsymRtf for szKeyword and evaluate it appropriately.
 *
 * Inputs:
 * szKeyword:   The RTF control to evaluate.
 * param:       The parameter of the RTF control.
 * fParam:      fTrue if the control had a parameter; (that is, if param
 *                    is valid)
 *              fFalse if it did not.
 */
int ecTranslateKeyword(RTF_Context *ctx, char *szKeyword, int param,
        bool fParam)
{
    int isym;

    /* search for szKeyword in rgsymRtf */
    for (isym = 0; isym < isymMax; isym++)
        if (strcmp(szKeyword, rgsymRtf[isym].szKeyword) == 0)
            break;
    if (isym == isymMax)        /* control word not found */
    {
#ifdef DEBUG_RTF
        fprintf(stderr, "Ignoring unknown keyword: %s\n", szKeyword);
#endif
        if (ctx->fSkipDestIfUnk)     /* if this is a new destination */
            ctx->rds = rdsSkip; /* skip the destination */
        /* else just discard it */
        ctx->fSkipDestIfUnk = fFalse;
        return ecOK;
    }

    /* found it!  use kwd and idx to determine what to do with it. */
    ctx->fSkipDestIfUnk = fFalse;
    switch (rgsymRtf[isym].kwd)
    {
        case kwdProp:
            if (rgsymRtf[isym].fPassDflt || !fParam)
                param = rgsymRtf[isym].dflt;
            return ecApplyPropChange(ctx, rgsymRtf[isym].idx, param);
        case kwdChar:
            return ecParseChar(ctx, rgsymRtf[isym].idx);
        case kwdDest:
            return ecChangeDest(ctx, rgsymRtf[isym].idx);
        case kwdSpec:
            return ecParseSpecialKeyword(ctx, rgsymRtf[isym].idx);
        default:
            return ecBadTable;
    }
    return ecBadTable;
}

/*
 * %%Function: ecChangeDest
 *
 * Change to the destination specified by idest.
 * There's usually more to do here than this...
 */
int ecChangeDest(RTF_Context *ctx, IDEST idest)
{
    if (ctx->rds == rdsSkip)    /* if we're skipping text, */
        return ecOK;            /* don't do anything */

    switch (idest)
    {
        case idestFontTable:
            ctx->rds = rdsFontTable;
            ctx->datapos = 0;
            break;
        case idestColorTable:
            ctx->rds = rdsColorTable;
            ctx->values[0] = 0;
            ctx->values[1] = 0;
            ctx->values[2] = 0;
            break;
        case idestInfo:
            ctx->rds = rdsInfo;
            ctx->datapos = 0;
            break;
        case idestTitle:
            ctx->rds = rdsTitle;
            ctx->datapos = 0;
            break;
        case idestSubject:
            ctx->rds = rdsSubject;
            ctx->datapos = 0;
            break;
        case idestAuthor:
            ctx->rds = rdsAuthor;
            ctx->datapos = 0;
            break;
        default:
            ctx->rds = rdsSkip; /* when in doubt, skip it... */
            break;
    }
    return ecOK;
}

/*
 * %%Function: ecEndGroupAction
 *
 * The destination specified by rds is coming to a close.
 * If there's any cleanup that needs to be done, do it now.
 */
int ecEndGroupAction(RTF_Context *ctx, RDS rds)
{
    switch (ctx->rds)
    {
        case rdsTitle:
            if (ctx->title)
                free(ctx->title);
            ctx->data[ctx->datapos] = '\0';
            ctx->title = *ctx->data ? strdup(ctx->data) : NULL;
            ctx->datapos = 0;
            break;
        case rdsSubject:
            if (ctx->subject)
                free(ctx->subject);
            ctx->data[ctx->datapos] = '\0';
            ctx->subject = *ctx->data ? strdup(ctx->data) : NULL;
            ctx->datapos = 0;
            break;
        case rdsAuthor:
            if (ctx->author)
                free(ctx->author);
            ctx->data[ctx->datapos] = '\0';
            ctx->author = *ctx->data ? strdup(ctx->data) : NULL;
            ctx->datapos = 0;
            break;
        default:
            break;
    }
    return ecOK;
}

/*
 * %%Function: ecParseSpecialKeyword
 *
 * Evaluate an RTF control that needs special processing.
 */
int ecParseSpecialKeyword(RTF_Context *ctx, IPFN ipfn)
{
    if (ctx->rds == rdsSkip && ipfn != ipfnBin) /* if we're skipping, and it's not */
        return ecOK;            /* the \bin keyword, ignore it. */
    switch (ipfn)
    {
        case ipfnBin:
            ctx->ris = risBin;
            ctx->cbBin = ctx->lParam;
            break;
        case ipfnSkipDest:
            ctx->fSkipDestIfUnk = fTrue;
            break;
        case ipfnHex:
            ctx->ris = risHex;
            break;
        default:
            return ecBadTable;
    }
    return ecOK;
}

