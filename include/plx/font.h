/* Parallax for KallistiOS ##version##

   font.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_FONT
#define __PARALLAX_FONT

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Texture-based font routines. These are equivalent to DCPLIB's
  fntTxf routines, but are written entirely in C.
 */

#include <kos/vector.h>
#include "texture.h"

/**
  A font struct. This will maintain all the information we need to actually
  draw using this font. Note however that this does not include things
  like style states (italics, etc). All members are considered private
  and should not be tampered with.
 */
typedef struct plx_font {
	plx_texture_t	* txr;		/**< Our font texture */
	int		glyph_cnt;	/**< The number of glyphs we have loaded */
	int		map_cnt;	/**< Size of our font map in entries */
	short		* map;		/**< Mapping from 16-bit character to glyph index */
	point_t		* txr_ll;	/**< Lower-left texture coordinates */
	point_t		* txr_ur;	/**< Upper-right texture coordinates */
	point_t		* vert_ll;	/**< Lower-left vertex coordinates */
	point_t		* vert_ur;	/**< Upper-right vertex coordinates */
} plx_font_t;


/**
  Font drawing context struct. This struct will keep track of everything you
  need to actually draw text using a plx_font_t. This includes current output
  position, font attributes (oblique, etc), etc. All members are considered
  private and should not be tampered with.
 */
typedef struct plx_fcxt {
	plx_font_t	* fnt;		/**< Our current font */
	int		list;		/**< Which PLX list will we use? */
	float		slant;		/**< For oblique text output */
	float		size;		/**< Pixel size */
	float		gap;		/**< Extra pixels between each char */
	float		fixed_width;	/**< Width of each character in points in fixed mode */
	uint32		flags;		/**< Font attributes */
	uint32		color;		/**< Color value */
	point_t		pos;		/**< Output position */
} plx_fcxt_t;

#define PLX_FCXT_FIXED	0x0001		/**< Fixed width flag */

/**
  Load a font from the VFS. Fonts are currently in the standard TXF format,
  though this may eventually change to add more capabilities (textured fonts,
  large/wide character sets, etc).
 */
plx_font_t * plx_font_load(const char * fn);

/**
  Remove a previously loaded font from memory.
 */
void plx_font_destroy(plx_font_t * fnt);

/**
  Create a font context from the given font struct with some reasonable
  default values. The list given in the list parameter will be used for
  low-level interactions.
 */
plx_fcxt_t * plx_fcxt_create(plx_font_t * fnt, int list);

/**
  Destroy a previously created font context. Note that this will NOT affect
  the font the context points to.
 */
void plx_fcxt_destroy(plx_fcxt_t * cxt);

/**
  Given a font context, return the metrics of the individual named character.
 */
void plx_fcxt_char_metrics(plx_fcxt_t * cxt, uint16 ch,
	float * outleft, float * outup, float * outright, float * outdown);

/**
  Given a font context, return the metrics of the given string.
 */
void plx_fcxt_str_metrics(plx_fcxt_t * cxt, const char * str,
	float * outleft, float * outup, float * outright, float *outdown);

/**
  Set the slant value for the given context.
 */
void plx_fcxt_setslant(plx_fcxt_t * cxt, float slant);

/**
  Get the slant value for the given context.
 */
float plx_fcxt_getslant(plx_fcxt_t * cxt);

/**
  Set the size value for the given context.
 */
void plx_fcxt_setsize(plx_fcxt_t * cxt, float size);

/**
  Get the size value for the given context.
 */
float plx_fcxt_getsize(plx_fcxt_t * cxt);

/**
  Set the color value for the given context as a set of floating
  point numbers.
 */
void plx_fcxt_setcolor4f(plx_fcxt_t * cxt, float a, float r, float g, float b);

/**
  Get the color value for the given context as a set of floating
  point numbers.
 */
void plx_fcxt_getcolor4f(plx_fcxt_t * cxt, float * outa, float * outr,
	float * outg, float * outb);

/**
  Set the color value for the given context as a packed integer.
 */
void plx_fcxt_setcolor1u(plx_fcxt_t * cxt, uint32 color);

/**
  Get the color value for the given context as a packed integer.
 */
uint32 plx_fcxt_getcolor1u(plx_fcxt_t * cxt);

/**
  Set the output cursor position for the given context using a point_t.
 */
void plx_fcxt_setpos_pnt(plx_fcxt_t * cxt, const point_t * pos);

/**
  Set the output cursor position for the given context using 3 coords.
 */
void plx_fcxt_setpos(plx_fcxt_t * cxt, float x, float y, float z);

/**
  Get the output cursor position for the given context.
 */
void plx_fcxt_getpos(plx_fcxt_t * cxt, point_t * outpos);

/**
  Set the offset cursor position for the given context.
 */
void plx_fcxt_setoffs(plx_fcxt_t * cxt, const point_t * offs);

/**
  Get the offset cursor position for the given context.
 */
void plx_fcxt_getoffs(plx_fcxt_t * cxt, point_t * outoffs);

/**
  Add the given values to the given context's offset position.
 */
void plx_fcxt_addoffs(plx_fcxt_t * cxt, const point_t * offset);

/**
  Begin a drawing operation with the given context. This must be
  done before any font operations where something else might have
  submitted a polygon header since the last font operation.
 */
void plx_fcxt_begin(plx_fcxt_t * cxt);

/**
  Draw a single character with the given context and parameters.
  Returns the cursor advancement amount.
 */
float plx_fcxt_draw_ch(plx_fcxt_t * cxt, uint16 ch);

/**
  Draw a string with the given context and parameters.
 */
void plx_fcxt_draw(plx_fcxt_t * cxt, const char * str);

/**
  Finish a drawing operation with the given context. Called after
  you are finished with a drawing operation. This currently doesn't
  do much, but it might later, so call it!
 */
void plx_fcxt_end(plx_fcxt_t * cxt);

__END_DECLS

#endif	/* __PARALLAX_FONT */
