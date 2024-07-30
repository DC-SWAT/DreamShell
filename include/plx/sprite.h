/* Parallax for KallistiOS ##version##

   sprite.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_SPRITE
#define __PARALLAX_SPRITE

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Here we define the basic sprite functions. These, like the primitive
  functions, are all inlined for speed (and DR usability). A sprite is
  basically just a quad. There are two axes of variation here, like in
  the primitives:

  - Whether you use DR or plx_prim for submission
  - Whether you use the matrix math for special effects

  Each function asks for the width and height of the quad, which allows you
  to either pass in the texture's width and height for a raw sprite, or
  scale the values for easy scaling without any matrix math. Full matrix
  math will allow for 3D usage, rotations, etc.

  All sprite quads are assumed to be textured (though of course you can submit
  quads with u/v values without using a texture, in which case it will be
  ignored by the hardware). It is also assumed that you have already submitted
  the polygon header for the given texture.

  Like the primitives, each function has a suffix that describes its
  function. The first letter is f or i, for float or integer color values.
  The second value is n for no matrix or m for matrix. The third is whether
  it uses DR (d) or plx_prim (p).

  If you need anything more complex (like custom u/v coords) then do it
  manually using the plx_vert_* functions.
 */

#include "prim.h"
#include "texture.h"
#include "matrix.h"

/********************************************************* DR VERSIONS ***/

/**
  Submit a quad using the given coordinates, color, and UV values via
  DR. The coordinates are at the center point.
 */
static inline void plx_spr_fnd(plx_dr_state_t * state,
	float wi, float hi,
	float x, float y, float z,
	float a, float r, float g, float b)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	plx_vert_ffd(state, PLX_VERT,
		x - w, y + h, z,
		a, r, g, b,
		0.0f, 1.0f);
	plx_vert_ffd(state, PLX_VERT,
		x - w, y - h, z,
		a, r, g, b,
		0.0f, 0.0f);
	plx_vert_ffd(state, PLX_VERT,
		x + w, y + h, z,
		a, r, g, b,
		1.0f, 1.0f);
	plx_vert_ffd(state, PLX_VERT_EOS,
		x + w, y - h, z,
		a, r, g, b,
		1.0f, 0.0f);
}

/**
  Like plx_spr_fnd, but with integer color.
 */
static inline void plx_spr_ind(plx_dr_state_t * state,
	float wi, float hi,
	float x, float y, float z,
	uint32 color)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	plx_vert_ifd(state, PLX_VERT,
		x - w, y + h, z,
		color,
		0.0f, 1.0f);
	plx_vert_ifd(state, PLX_VERT,
		x - w, y - h, z,
		color,
		0.0f, 0.0f);
	plx_vert_ifd(state, PLX_VERT,
		x + w, y + h, z,
		color,
		1.0f, 1.0f);
	plx_vert_ifd(state, PLX_VERT_EOS,
		x + w, y - h, z,
		color,
		1.0f, 0.0f);
}

/**
  Like plx_spr_fnd, but using matrix math.
 */
static inline void plx_spr_fmd(plx_dr_state_t * state,
	float wi, float hi,
	float xi, float yi, float zi,
	float a, float r, float g, float b)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	float x, y, z;

	x = xi-w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffd(state, PLX_VERT,
		x, y, z,
		a, r, g, b,
		0.0f, 1.0f);

	x = xi-w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffd(state, PLX_VERT,
		x, y, z,
		a, r, g, b,
		0.0f, 0.0f);

	x = xi+w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffd(state, PLX_VERT,
		x, y, z,
		a, r, g, b,
		1.0f, 1.0f);

	x = xi+w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffd(state, PLX_VERT_EOS,
		x, y, z,
		a, r, g, b,
		1.0f, 0.0f);
}

/**
  Like plx_spr_fmd, but using integer colors.
 */
static inline void plx_spr_imd(plx_dr_state_t * state,
	float wi, float hi,
	float xi, float yi, float zi,
	uint32 color)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	float x, y, z;

	x = xi-w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifd(state, PLX_VERT,
		x, y, z,
		color,
		0.0f, 1.0f);

	x = xi-w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifd(state, PLX_VERT,
		x, y, z,
		color,
		0.0f, 0.0f);

	x = xi+w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifd(state, PLX_VERT,
		x, y, z,
		color,
		1.0f, 1.0f);

	x = xi+w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifd(state, PLX_VERT_EOS,
		x, y, z,
		color,
		1.0f, 0.0f);
}

/**************************************************** PVR_PRIM VERSIONS ***/

/**
  Like plx_spr_fnd, but using pvr_prim.
 */
static inline void plx_spr_fnp(
	float wi, float hi,
	float x, float y, float z,
	float a, float r, float g, float b)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	plx_vert_ffp(PLX_VERT,
		x - w, y + h, z,
		a, r, g, b,
		0.0f, 1.0f);
	plx_vert_ffp(PLX_VERT,
		x - w, y - h, z,
		a, r, g, b,
		0.0f, 0.0f);
	plx_vert_ffp(PLX_VERT,
		x + w, y + h, z,
		a, r, g, b,
		1.0f, 1.0f);
	plx_vert_ffp(PLX_VERT_EOS,
		x + w, y - h, z,
		a, r, g, b,
		1.0f, 0.0f);
}

/**
  Like plx_spr_ind, but using pvr_prim.
 */
static inline void plx_spr_inp(
	float wi, float hi,
	float x, float y, float z,
	uint32 color)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	plx_vert_ifp(PLX_VERT,
		x - w, y + h, z,
		color,
		0.0f, 1.0f);
	plx_vert_ifp(PLX_VERT,
		x - w, y - h, z,
		color,
		0.0f, 0.0f);
	plx_vert_ifp(PLX_VERT,
		x + w, y + h, z,
		color,
		1.0f, 1.0f);
	plx_vert_ifp(PLX_VERT_EOS,
		x + w, y - h, z,
		color,
		1.0f, 0.0f);
}

/**
  Like plx_spr_fmd, but using pvr_prim.
 */
static inline void plx_spr_fmp(
	float wi, float hi,
	float xi, float yi, float zi,
	float a, float r, float g, float b)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	float x, y, z;

	x = xi-w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffp(PLX_VERT,
		x, y, z,
		a, r, g, b,
		0.0f, 1.0f);

	x = xi-w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffp(PLX_VERT,
		x, y, z,
		a, r, g, b,
		0.0f, 0.0f);

	x = xi+w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffp(PLX_VERT,
		x, y, z,
		a, r, g, b,
		1.0f, 1.0f);

	x = xi+w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ffp(PLX_VERT_EOS,
		x, y, z,
		a, r, g, b,
		1.0f, 0.0f);
}

/**
  Like plx_spr_imd, but using pvr_prim.
 */
static inline void plx_spr_imp(
	float wi, float hi,
	float xi, float yi, float zi,
	uint32 color)
{
	float w = wi / 2.0f;
	float h = hi / 2.0f;
	float x, y, z;

	x = xi-w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifp(PLX_VERT,
		x, y, z,
		color,
		0.0f, 1.0f);

	x = xi-w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifp(PLX_VERT,
		x, y, z,
		color,
		0.0f, 0.0f);

	x = xi+w; y = yi+h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifp(PLX_VERT,
		x, y, z,
		color,
		1.0f, 1.0f);

	x = xi+w; y = yi-h; z = zi;
	plx_mat_tfip_2d(x, y, z);
	plx_vert_ifp(PLX_VERT_EOS,
		x, y, z,
		color,
		1.0f, 0.0f);
}

__END_DECLS

#endif	/* __PARALLAX_SPRITE */
