/* Parallax for KallistiOS ##version##

   matrix.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_MATRIX
#define __PARALLAX_MATRIX

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Matrix primitives. Half of these are just wrappers for the DC arch
  matrix routines. The main purpose of this is to try to keep client code
  less architecture specific so we can potentially port later more easily.

  The second half are routines adapted from KGL, and are intended for 3D
  usage primarily, though they can also be used for 2D if you want to do
  that (rotate, translate, etc). Like KGL, these use degrees.
 */

#include <dc/matrix.h>
#include <dc/matrix3d.h>

/* dc/matrix.h -- basic matrix register operations. */
#define plx_mat_store		mat_store
#define plx_mat_load		mat_load
#define plx_mat_identity	mat_identity
#define plx_mat_apply		mat_apply
#define plx_mat_transform	mat_transform
#define plx_mat_tfip_3d		mat_trans_single
#define plx_mat_tfip_3dw	mat_trans_single4
#define plx_mat_tfip_2d		mat_trans_single3

/* dc/matrix3d.h -- some of these are still useful in 2D. All of
   the following work on the matrix registers. */
#define plx_mat_rotate_x	mat_rotate_x
#define plx_mat_rotate_y	mat_rotate_y
#define plx_mat_rotate_z	mat_rotate_z
#define plx_mat_rotate		mat_rotate
#define plx_mat_translate	mat_translate
#define plx_mat_scale		mat_scale

/* The 3D matrix operations, somewhat simplified from KGL. All of these use
   the matrix regs, but do not primarily keep their values in them. To get
   the values out into the matrix regs (and usable) you'll want to set
   everything up and then call plx_mat3d_apply(). */

/** Call before doing anything else, or after switching video
    modes to setup some basic parameters. */
void plx_mat3d_init();

/** Set which matrix we are working on */
void plx_mat3d_mode(int mode);

/* Constants for plx_mat3d_mode and plx_mat3d_apply */
#define	PLX_MAT_PROJECTION	0	/** Projection (frustum, screenview) matrix */
#define PLX_MAT_MODELVIEW	1	/** Modelview (rotate, scale) matrix */
#define PLX_MAT_SCREENVIEW	2	/** Internal screen view matrix */
#define PLX_MAT_SCRATCH		3	/** Temp matrix for user usage */
#define PLX_MAT_WORLDVIEW	4	/** Optional camera/worldview matrix */
#define PLX_MAT_COUNT		5

/** Load an identity matrix */
void plx_mat3d_identity();

/** Load a raw matrix */
void plx_mat3d_load(matrix_t * src);

/** Save a raw matrix */
void plx_mat3d_store(matrix_t * src);

/** Setup viewport parameters */
void plx_mat3d_viewport(int x1, int y1, int width, int height);

/** Setup a perspective matrix */
void plx_mat3d_perspective(float angle, float aspect, float znear, float zfar);

/** Setup a frustum matrix */
void plx_mat3d_frustum(float left, float right, float bottom, float top, float znear, float zfar);

/** Push a matrix on the stack */
void plx_mat3d_push();

/** Pop a matrix from the stack and reload it */
void plx_mat3d_pop();

/** Reload a matrix from the top of the stack, but don't pop it */
void plx_mat3d_peek();

/** Rotation */
void plx_mat3d_rotate(float angle, float x, float y, float z);

/** Scaling */
void plx_mat3d_scale(float x, float y, float z);

/** Translation */
void plx_mat3d_translate(float x, float y, float z);

/** Do a camera "look at" */
void plx_mat3d_lookat(const point_t * eye, const point_t * center, const vector_t * up);

/** Apply a matrix from one of the matrix modes to the matrix regs */
void plx_mat3d_apply(int mode);

/** Manually apply a matrix */
void plx_mat3d_apply_mat(matrix_t * src);

/** Apply all the matrices for a normal 3D scene */
void plx_mat3d_apply_all();

__END_DECLS

#endif	/* __PARALLAX_MATRIX */
