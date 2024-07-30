/* Parallax for KallistiOS ##version##

   prim.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_PRIM
#define __PARALLAX_PRIM

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file This header defines the Parallax library "primitives". These are
  simply inlined wrapper functions which make it simpler and more compact
  to deal with the plx vertex structures. Each function has a suffix
  which determines what kind of color values are used, what kind of
  texture u/v values are used, and whether the vertex will be submitted
  automatically. For colors, "f" means float and "i" means a pre-packed
  32-bit integer. For u/v, "n" means none, and "f" means float. And for
  submission, "n" means none, "d" means DR, and "p" means plx_prim.
 */

#include "color.h"
#include "dr.h"
#include "matrix.h"

/********************************************************* RAW VERTEX ****/

/**
  This simple primitive function will fill a vertex structure for
  you from parameters. It uses floating point numbers for the color
  values and no u/v coordinates. The "vert" parameter may be a DR target.
 */
static inline void plx_vert_fnn(plx_vertex_t * vert, int flags, float x, float y, float z,
	float a, float r, float g, float b)
{
	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = vert->v = 0.0f;
	vert->argb = plx_pack_color(a, r, g, b);
	vert->oargb = 0;
}

/**
  Like plx_vert_fnn, but it takes a pre-packed integer color value.
 */
static inline void plx_vert_inn(plx_vertex_t * vert, int flags, float x, float y, float z,
	uint32 color)
{
	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = vert->v = 0.0f;
	vert->argb = color;
	vert->oargb = 0;
}

/**
  Like plx_vert_fnn, but it takes u/v texture coordinates as well.
 */
static inline void plx_vert_ffn(plx_vertex_t * vert, int flags, float x, float y, float z,
	float a, float r, float g, float b, float u, float v)
{
	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = u;
	vert->v = v;
	vert->argb = plx_pack_color(a, r, g, b);
	vert->oargb = 0;
}

/**
  Like plx_vert_fnn, but it takes u/v texture coordinates and a pre-packed integer
  color value.
 */
static inline void plx_vert_ifn(plx_vertex_t * vert, int flags, float x, float y, float z,
	uint32 color, float u, float v)
{
	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = u;
	vert->v = v;
	vert->argb = color;
	vert->oargb = 0;
}


/********************************************************* DR VERTEX ****/

/**
  Like plx_vert_fnn, but submits the point using DR.
 */
static inline void plx_vert_fnd(plx_dr_state_t * state, int flags, float x, float y, float z,
	float a, float r, float g, float b)
{
	plx_vertex_t * vert = plx_dr_target(state);

	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = vert->v = 0.0f;
	vert->argb = plx_pack_color(a, r, g, b);
	vert->oargb = 0;

	plx_dr_commit(vert);
}

/**
  Like plx_vert_inn, but submits the point using DR.
 */
static inline void plx_vert_ind(plx_dr_state_t * state, int flags, float x, float y, float z,
	uint32 color)
{
	plx_vertex_t * vert = plx_dr_target(state);

	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = vert->v = 0.0f;
	vert->argb = color;
	vert->oargb = 0;

	plx_dr_commit(vert);
}

/**
  Like plx_vert_ffn, but submits the point using DR.
 */
static inline void plx_vert_ffd(plx_dr_state_t * state, int flags, float x, float y, float z,
	float a, float r, float g, float b, float u, float v)
{
	plx_vertex_t * vert = plx_dr_target(state);

	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = u;
	vert->v = v;
	vert->argb = plx_pack_color(a, r, g, b);
	vert->oargb = 0;

	plx_dr_commit(vert);
}

/**
  Like plx_vert_ifn, but submits the point using DR.
 */
static inline void plx_vert_ifd(plx_dr_state_t * state, int flags, float x, float y, float z,
	uint32 color, float u, float v)
{
	plx_vertex_t * vert = plx_dr_target(state);

	vert->flags = flags;
	vert->x = x;
	vert->y = y;
	vert->z = z;
	vert->u = u;
	vert->v = v;
	vert->argb = color;
	vert->oargb = 0;

	plx_dr_commit(vert);
}

/**
  Like plx_vert_ind, but also transforms via the active matrices for 3D
 */
static inline void plx_vert_indm3(plx_dr_state_t * state, int flags, float x, float y, float z,
	uint32 color)
{
       plx_mat_tfip_3d(x, y, z);
       plx_vert_ind(state, flags, x, y, z, color);
}

/**
  Like plx_vert_ifd, but also transforms via the active matrices for 3D
 */
static inline void plx_vert_ifdm3(plx_dr_state_t * state, int flags, float x, float y, float z,
	uint32 color, float u, float v)
{
       plx_mat_tfip_3d(x, y, z);
       if (z <= 0.0f) z = 0.001f;
       plx_vert_ifd(state, flags, x, y, z, color, u, v);
}

/****************************************************** PLX_PRIM VERTEX ****/

/**
  Like plx_vert_fnp, but submits the point using plx_prim.
 */
static inline void plx_vert_fnp(int flags, float x, float y, float z,
	float a, float r, float g, float b)
{
	plx_vertex_t vert;

	vert.flags = flags;
	vert.x = x;
	vert.y = y;
	vert.z = z;
	vert.u = vert.v = 0.0f;
	vert.argb = plx_pack_color(a, r, g, b);
	vert.oargb = 0;

	plx_prim(&vert, sizeof(vert));
}

/**
  Like plx_vert_inn, but submits the point using plx_prim.
 */
static inline void plx_vert_inp(int flags, float x, float y, float z, uint32 color) {
	plx_vertex_t vert;

	vert.flags = flags;
	vert.x = x;
	vert.y = y;
	vert.z = z;
	vert.u = vert.v = 0.0f;
	vert.argb = color;
	vert.oargb = 0;

	plx_prim(&vert, sizeof(vert));
}

/**
  Like plx_vert_indm3, but uses plx_prim.
 */
static inline void plx_vert_inpm3(int flags, float x, float y, float z, uint32 color) {
	plx_mat_tfip_3d(x, y, z);
	plx_vert_inp(flags, x, y, z, color);
}

/**
  Like plx_vert_ffn, but submits the point using plx_prim.
 */
static inline void plx_vert_ffp(int flags, float x, float y, float z,
	float a, float r, float g, float b, float u, float v)
{
	plx_vertex_t vert;

	vert.flags = flags;
	vert.x = x;
	vert.y = y;
	vert.z = z;
	vert.u = u;
	vert.v = v;
	vert.argb = plx_pack_color(a, r, g, b);
	vert.oargb = 0;

	plx_prim(&vert, sizeof(vert));
}

/**
  Like plx_vert_ifn, but submits the point using plx_prim.
 */
static inline void plx_vert_ifp(int flags, float x, float y, float z,
	uint32 color, float u, float v)
{
	plx_vertex_t vert;

	vert.flags = flags;
	vert.x = x;
	vert.y = y;
	vert.z = z;
	vert.u = u;
	vert.v = v;
	vert.argb = color;
	vert.oargb = 0;

	plx_prim(&vert, sizeof(vert));
}

__END_DECLS

#endif	/* __PARALLAX_PRIM */
