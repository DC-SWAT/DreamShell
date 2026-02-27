/* Parallax for KallistiOS ##version##

   font.c

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024-2026 SWAT

*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kos/fs.h>
#include <kos/dbglog.h>
#include "font.h"
#include "prim.h"

/* See the header file for all comments and documentation */

/*
  Implementation notes...

  TXF Header (thanks to PLIB and gentexfont)

  BYTES         WHAT
  -----------------------------
  4	 	0xff + 'txf'
  4		0x12345678 for endian testing
  4		format: 0 for bytes, 1 for bits
  4		texture width
  4		texture height
  4		maximum font ascent
  4		maximum font descent
  4		glyph count

  TXF Per-Glyph Data

  BYTES		WHAT
  -----------------------------
  2		character index
  1		width in pixels
  1		height in pixels
  1		x offset for left side of char
  1		y offset for bottom of char
  1		advance
  1		char for padding
  2		texture x for left of char
  2		texture y for bottom of char

  We'll mimic PLIB here as far as handling the fonts: each u/v coord
  is offset by half a pixel (which provides a bit of an anti-aliasing effect)
  and all character sizes are scaled to 1 pixel high using the maximum font
  ascent. We then multiply this by the point size to get the real quad
  size. Additionally, we add to the X coordinate of the top of the
  glyph to achieve an oblique effect (it's not a proper italic...)

*/

typedef struct __attribute__((packed)) {
	uint8	magic[4];
	uint32	endian;
	uint32	format;
	uint32	txr_width;
	uint32	txr_height;
	int32	max_ascent;
	int32	max_descent;
	uint32	glyph_cnt;
} txfhdr_t;

typedef struct __attribute__((packed)) {
	int16	idx;
	int8	w;
	int8	h;
	int8	x_offset;
	int8	y_offset;
	int8	advance;
	char	padding;
	uint16	x;
	uint16	y;
} txfglyph_t;

/* This function DEFINITELY has function growth hormone inbalance syndrome,
   but whatever... :P And ohh the smell of gotos in the morning! ;) */
plx_font_t * plx_font_load(const char * fn) {
	plx_font_t	* fnt;
	file_t		f;
	txfhdr_t	hdr __attribute__((aligned(32)));
	txfglyph_t	g __attribute__((aligned(32)));
	int		i, x, y;
	float		xstep, ystep, w, h;
	uint8		* bmtmp = NULL;
	uint32		bmsize;
	uint16		* txrtmp = NULL;
	int		stride;

	/* Open the input file */
	f = fs_open(fn, O_RDONLY);
	if (f == FILEHND_INVALID) {
		dbglog(DBG_WARNING, "plx_font_load: couldn't open file '%s'\n", fn);
		return NULL;
	}

	/* Create a font struct */
	fnt = malloc(sizeof(plx_font_t));
	if (fnt == NULL) {
		dbglog(DBG_WARNING, "plx_font_load: couldn't allocate memory for '%s'\n", fn);
		goto fail_1;	/* bail */
	}
	memset(fnt, 0, sizeof(plx_font_t));

	/* Load up the TXF header */
	if (fs_read(f, &hdr, sizeof(txfhdr_t)) != sizeof(txfhdr_t)) {
		dbglog(DBG_WARNING, "plx_font_load: truncated file '%s'\n", fn);
		goto fail_2;	/* bail */
	}

	if (hdr.magic[0] != 0xff || strncmp("txf", (char *)hdr.magic+1, 3)) {
		dbglog(DBG_WARNING, "plx_font_load: invalid font file '%s'\n", fn);
		goto fail_2;	/* bail */
	}

	if (hdr.endian != 0x12345678) {
		dbglog(DBG_WARNING, "plx_font_load: invalid endianness for '%s'\n", fn);
		goto fail_2;	/* bail */
	}

	/* dbglog(DBG_DEBUG, "plx_font_load:  loading font '%s'\n"
			  "  texture size: %ldx%ld\n"
			  "  max ascent:   %ld\n"
			  "  max descent:  %ld\n"
			  "  glyph count:  %ld\n",
		fn, hdr.txr_width, hdr.txr_height,
		hdr.max_ascent, hdr.max_descent, hdr.glyph_cnt); */

	/* Make sure we can allocate texture space for it */
	fnt->txr = plx_txr_canvas(hdr.txr_width, hdr.txr_height, PVR_TXRFMT_ARGB4444);
	if (fnt->txr == NULL) {
		dbglog(DBG_WARNING, "plx_font_load: can't allocate texture for '%s'\n", fn);
		goto fail_2;	/* bail */
	}

	/* Copy over some other misc housekeeping info */
	fnt->glyph_cnt = hdr.glyph_cnt;
	fnt->map_cnt = 256;	/* Just ASCII for now */

	/* Allocate structs for the various maps */
	fnt->map = malloc(2 * fnt->map_cnt);
	fnt->txr_ll = malloc(sizeof(point_t) * fnt->glyph_cnt * 4);
	if (fnt->map == NULL || fnt->txr_ll == NULL) {
		dbglog(DBG_WARNING, "plx_font_load: can't allocate memory for maps for '%s'\n", fn);
		goto fail_3;	/* bail */
	}
	fnt->txr_ur = fnt->txr_ll + fnt->glyph_cnt * 1;
	fnt->vert_ll = fnt->txr_ll + fnt->glyph_cnt * 2;
	fnt->vert_ur = fnt->txr_ll + fnt->glyph_cnt * 3;

	/* Set all chars as not present */
	for (i=0; i<fnt->map_cnt; i++)
		fnt->map[i] = -1;

	/* Some more helpful values... */
	w = (float)hdr.txr_width;
	h = (float)hdr.txr_height;
	xstep = ystep = 0.0f;

	/* Use this instead to get a pseudo-antialiasing effect */
	/* xstep = 0.5f / w;
	ystep = 0.5f / h; */

	/* Ok, go through and load up each glyph */
	for (i=0; i<fnt->glyph_cnt; i++) {
		/* Load up the glyph info */
		if (fs_read(f, &g, sizeof(txfglyph_t)) != sizeof(txfglyph_t)) {
			dbglog(DBG_WARNING, "plx_font_load: truncated file '%s'\n", fn);
			goto fail_3;	/* bail */
		}

		/* Is it above our limit? If so, ignore it */
		if (g.idx >= fnt->map_cnt)
			continue;

		/* Leave out the space glyph, if we have one */
		if (g.idx == ' ')
			continue;

		/* Pull in all the relevant parameters */
		fnt->map[g.idx] = i;
		fnt->txr_ll[i].x = g.x / w + xstep;
		fnt->txr_ll[i].y = g.y / h + ystep;
		fnt->txr_ur[i].x = (g.x + g.w) / w + xstep;
		fnt->txr_ur[i].y = (g.y + g.h) / h + ystep;
		fnt->vert_ll[i].x = (float)g.x_offset / hdr.max_ascent;
		fnt->vert_ll[i].y = (float)g.y_offset / hdr.max_ascent;
		fnt->vert_ur[i].x = ((float)g.x_offset + g.w) / hdr.max_ascent;
		fnt->vert_ur[i].y = ((float)g.y_offset + g.h) / hdr.max_ascent;

		/* dbglog(DBG_DEBUG, "  loaded glyph %d(%c): uv %.2f,%.2f - %.2f,%.2f, vert %.2f,%.2f - %.2f, %.2f\n",
			g.idx, (char)g.idx,
			(double)fnt->txr_ll[i].x, (double)fnt->txr_ll[i].y,
			(double)fnt->txr_ur[i].x, (double)fnt->txr_ur[i].y,
			(double)fnt->vert_ll[i].x, (double)fnt->vert_ll[i].y,
			(double)fnt->vert_ur[i].x, (double)fnt->vert_ur[i].y); */
	}

	/* What format are we using? */
	switch (hdr.format) {
	case 1:		/* TXF_FORMAT_BITMAP */
		/* Allocate temp texture space */
		bmsize = hdr.txr_width * hdr.txr_height / 8;
		bmtmp = aligned_alloc(32, bmsize);
		txrtmp = aligned_alloc(32, hdr.txr_width * hdr.txr_height * 2);
		if (bmtmp == NULL || txrtmp == NULL) {
			dbglog(DBG_WARNING, "plx_font_load: can't allocate temp texture space for '%s'\n", fn);
			goto fail_3;	/* bail */
		}

		/* Load the bitmap and convert to a texture */
		if (fs_read(f, bmtmp, bmsize) != bmsize) {
			dbglog(DBG_WARNING, "plx_font_load: truncated file '%s'\n", fn);
			goto fail_4;	/* bail */
		}

		stride = hdr.txr_width / 8;
		for (y=0; y<hdr.txr_height; y++) {
			for (x=0; x<hdr.txr_width; x++) {
				if (bmtmp[y * stride + x/8] & (1 << (x%8))) {
					txrtmp[y*hdr.txr_width+x] = 0xffff;
				} else {
					txrtmp[y*hdr.txr_width+x] = 0;
				}
			}
		}

		break;
	case 0:		/* TXF_FORMAT_BYTE */
		/* Allocate temp texture space */
		bmsize = hdr.txr_width * hdr.txr_height;
		txrtmp = aligned_alloc(32, bmsize * 2);
		if (txrtmp == NULL) {
			dbglog(DBG_WARNING, "plx_font_load: can't allocate temp texture space for '%s'\n", fn);
			goto fail_3;	/* bail */
		}

		/* Load the texture */
		if (fs_read(f, txrtmp, bmsize) != bmsize) {
			dbglog(DBG_WARNING, "plx_font_load: truncated file '%s'\n", fn);
			goto fail_4;	/* bail */
		}

		/* Convert to ARGB4444 -- go backwards so we can do it in place */
		/* PLIB seems to duplicate the alpha value into luminance.  I think it
		 * looks nicer to hardcode luminance to 1.0; characters look more robust. */
		bmtmp = (uint8 *)txrtmp;
		for (x=bmsize-1; x>=0; x--) {
			uint8 alpha = (bmtmp[x] & 0xF0) >> 4;
			/* uint8 lum   = alpha; */
			uint8 lum   = 0x0f;
			txrtmp[x] = (alpha << 12) | (lum << 8) | (lum << 4) | (lum << 0);
		}
		bmtmp = NULL;

		break;
	}

	/* dbglog(DBG_DEBUG, "plx_font_load: load done\n"); */

	/* Close the file */
	fs_close(f);

	/* Now load the temp texture into our canvas texture and twiddle it */
	pvr_txr_load_ex(txrtmp, fnt->txr->ptr, hdr.txr_width, hdr.txr_height, PVR_TXRLOAD_16BPP);

	/* Yay! Everything's happy. Clean up our temp textures and return the font. */
	if (bmtmp) free(bmtmp);
	if (txrtmp) free(txrtmp);
	return fnt;


/* Error handlers */
fail_4:			/* Temp texture is allocated */
	if (bmtmp) free(bmtmp);
	if (txrtmp) free(txrtmp);
fail_3:			/* Texture and some maps are allocated */
	if (fnt->map != NULL) free(fnt->map);
	if (fnt->txr_ll != NULL) free(fnt->txr_ll);
	plx_txr_destroy(fnt->txr);
fail_2:			/* Font struct is allocated */
	free(fnt);
fail_1:			/* Only the file is open */
	fs_close(f);
	return NULL;
}

void plx_font_destroy(plx_font_t * fnt) {
	assert( fnt != NULL );
	if (fnt == NULL) return;

	assert( fnt->txr != NULL );
	if (fnt->txr != NULL)
		plx_txr_destroy(fnt->txr);

	assert( fnt->map != NULL );
	if (fnt->map != NULL)
		free(fnt->map);

	assert( fnt->txr_ll != NULL );
	if (fnt->txr_ll != NULL)
		free(fnt->txr_ll);

	free(fnt);
}



plx_fcxt_t * plx_fcxt_create(plx_font_t * fnt, pvr_list_type_t list) {
	plx_fcxt_t * cxt;

	assert( fnt != NULL );
	if (fnt == NULL)
		return NULL;

	/* Allocate a struct */
	cxt = malloc(sizeof(plx_fcxt_t));
	if (cxt == NULL) {
		dbglog(DBG_WARNING, "plx_fcxt_create: couldn't allocate memory for context\n");
		return NULL;
	}

	/* Fill in some default values */
	memset(cxt, 0, sizeof(plx_fcxt_t));
	cxt->fnt = fnt;
	cxt->list = list;
	cxt->slant = 0.0f;
	cxt->size = 24.0f;
	cxt->gap = 0.1f;
	cxt->fixed_width = 1.0f;
	cxt->flags = 0;
	cxt->color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
	cxt->pos.x = 0.0f;
	cxt->pos.y = 0.0f;
	cxt->pos.z = 0.0f;

	return cxt;
}

void plx_fcxt_destroy(plx_fcxt_t * cxt) {
	assert( cxt != NULL );
	if (cxt == NULL)
		return;

	free(cxt);
}

void plx_fcxt_char_metrics(plx_fcxt_t * cxt, uint16 ch,
	float * outleft, float * outup, float * outright, float *outdown)
{
	plx_font_t	* fnt;
	int		g;

	assert( cxt != NULL );
	assert( cxt->fnt != NULL );
	if (cxt == NULL || cxt->fnt == NULL)
		return;

	fnt = cxt->fnt;

	if (ch >= fnt->map_cnt)
		ch = ' ';
	if (fnt->map[ch] == -1)
		ch = ' ';
	if (ch == ' ') {
		*outleft = 0;
		*outup = 0;
		*outright = cxt->gap + cxt->size / 2.0f;
		*outdown = 0;
		return;
	}

	g = fnt->map[ch];
	assert( 0 <= g && g < fnt->glyph_cnt );
	if (g < 0 || g >= fnt->glyph_cnt)
		return;

	*outleft = fnt->vert_ll[g].x * cxt->size;
	*outup = fnt->vert_ur[g].y * cxt->size;
	if (cxt->flags & PLX_FCXT_FIXED)
		*outright = (cxt->gap + cxt->fixed_width) * cxt->size;
	else
		*outright = (cxt->gap + fnt->vert_ur[g].x) * cxt->size;
	*outdown = fnt->vert_ll[g].y * -cxt->size;
}

void plx_fcxt_str_metrics(plx_fcxt_t * cxt, const char * str,
	float * outleft, float * outup, float * outright, float *outdown)
{
	float		l = 0, u = 0, r = 0, d = 0;
	int		i, ch, g, len;
	plx_font_t	* fnt;

	assert( cxt != NULL );
	assert( cxt->fnt != NULL );
	if (cxt == NULL || cxt->fnt == NULL)
		return;

	len = strlen(str);
	fnt = cxt->fnt;
	for (i=0; i<len; i++) {
		/* Find the glyph (if any) */
		ch = str[i];
		if (ch < 0 || ch >= fnt->map_cnt)
			ch = ' ';
		if (fnt->map[ch] == -1)
			ch = ' ';
		if (ch == ' ') {
			r += cxt->gap + cxt->size / 2.0f;
			continue;
		}

		g = fnt->map[ch];
		assert( 0 <= g && g < fnt->glyph_cnt );
		if (g < 0 || g >= fnt->glyph_cnt)
			continue;

		/* If this is the first char, do the left */
		if (i == 0) {
			l = fnt->vert_ll[g].x * cxt->size;
		}

		/* Handle the others */
		if (cxt->flags & PLX_FCXT_FIXED)
			r += (cxt->gap + cxt->fixed_width) * cxt->size;
		else
			r += (cxt->gap + fnt->vert_ur[g].x) * cxt->size;

		if (fnt->vert_ur[g].y * cxt->size > u)
			u = fnt->vert_ur[g].y * cxt->size;
		if (fnt->vert_ll[g].y * -cxt->size > d)
			d = fnt->vert_ll[g].y * -cxt->size;
	}

	if (outleft)
		*outleft = l;
	if (outup)
		*outup = u;
	if (outright)
		*outright = r;
	if (outdown)
		*outdown = d;
}

void plx_fcxt_setsize(plx_fcxt_t * cxt, float size) {
	assert( cxt != NULL );
	if (cxt == NULL)
		return;

	cxt->size = size;
}

float plx_fcxt_getsize(plx_fcxt_t * cxt) {
	assert( cxt != NULL );
	if (cxt == NULL)
		return 0.0f;
	return cxt->size;
}

void plx_fcxt_setcolor4f(plx_fcxt_t * cxt, float a, float r, float g, float b) {
	assert( cxt != NULL );
	if (cxt == NULL)
		return;

	cxt->color = plx_pack_color(a, r, g, b);
}

void plx_fcxt_setpos_pnt(plx_fcxt_t * cxt, const point_t * pos) {
	assert( cxt != NULL );
	if (cxt == NULL)
		return;

	cxt->pos = *pos;
}

void plx_fcxt_setpos(plx_fcxt_t * cxt, float x, float y, float z) {
	point_t pnt = {x, y, z};
	plx_fcxt_setpos_pnt(cxt, &pnt);
}

void plx_fcxt_getpos(plx_fcxt_t * cxt, point_t * outpos) {
	*outpos = cxt->pos;
}

void plx_fcxt_begin(plx_fcxt_t * cxt) {
	assert( cxt != NULL );
	assert( cxt->fnt != NULL );
	assert( cxt->fnt->txr != NULL );
	if (cxt == NULL || cxt->fnt == NULL || cxt->fnt->txr == NULL)
		return;

	/* Submit the polygon header for the font texture */
	plx_txr_send_hdr(cxt->fnt->txr, cxt->list, 0);
}

void plx_fcxt_end(plx_fcxt_t * cxt) {
	assert( cxt != NULL );
}

float plx_fcxt_draw_ch(plx_fcxt_t * cxt, uint16 ch) {
	plx_vertex_t	vert;
	plx_font_t	* fnt;
	int		i;

	assert( cxt != NULL );
	assert( cxt->fnt != NULL );
	if (cxt == NULL || cxt->fnt == NULL)
		return 0.0f;

	fnt = cxt->fnt;

	/* Do we have the character in question? */
	if (ch >= fnt->map_cnt)
		ch = 32;
	if (fnt->map[ch] == -1)
		ch = 32;
	if (ch == 32) {
		cxt->pos.x += cxt->gap + cxt->size / 2.0f;
		return cxt->gap + cxt->size / 2.0f;
	}

	i = fnt->map[ch];
	assert( i < fnt->glyph_cnt );
	if (i >= fnt->glyph_cnt)
		return 0.0f;

	/* Submit the vertices */
	plx_vert_ifn(&vert, PLX_VERT,
		cxt->pos.x + fnt->vert_ll[i].x * cxt->size,
		cxt->pos.y - fnt->vert_ll[i].y * cxt->size,
		cxt->pos.z,
		cxt->color, fnt->txr_ll[i].x, fnt->txr_ll[i].y);
	plx_prim(&vert, sizeof(vert));

	vert.x += cxt->slant;
	vert.y = cxt->pos.y - fnt->vert_ur[i].y * cxt->size;
	vert.v = fnt->txr_ur[i].y;
	plx_prim(&vert, sizeof(vert));

	vert.x = cxt->pos.x + fnt->vert_ur[i].x * cxt->size;
	vert.y = cxt->pos.y - fnt->vert_ll[i].y * cxt->size;
	vert.u = fnt->txr_ur[i].x;
	vert.v = fnt->txr_ll[i].y;
	plx_prim(&vert, sizeof(vert));

	vert.flags = PLX_VERT_EOS;
	vert.x += cxt->slant;
	vert.y = cxt->pos.y - fnt->vert_ur[i].y * cxt->size;
	vert.v = fnt->txr_ur[i].y;
	plx_prim(&vert, sizeof(vert));

	/* Advance the cursor position */
	float adv;
	if (cxt->flags & PLX_FCXT_FIXED)
		adv = (cxt->gap + cxt->fixed_width) * cxt->size;
	else
		adv = (cxt->gap + fnt->vert_ur[i].x) * cxt->size;
	cxt->pos.x += adv;
	return adv;
}

void plx_fcxt_draw(plx_fcxt_t * cxt, const char * str) {
	float origx = cxt->pos.x;
	float ly = 0.0f;

	while (*str != 0) {
		if (*str == '\n') {
			/* Handle newlines somewhat */
			cxt->pos.x = origx;
			ly = cxt->size;
			cxt->pos.y += ly;
			str++;
		} else
			plx_fcxt_draw_ch(cxt, *str++);
	}
}
