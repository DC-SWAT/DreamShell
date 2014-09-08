/* Parallax for KallistiOS ##version##

   mat3d.c
   (c)2001-2002 Dan Potter
   (c)2002 Benoit Miller and Paul Boese
   (c)2013-2014 SWAT
*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dc/fmath.h>
#include <dc/matrix.h>
#include <dc/video.h>
#include <plx/matrix.h>
#include "utils.h"

/*
  Most of this file was pulled from KGL's gltrans.c. Why did we do that
  instead of just suggesting linking with KGL to get them? Because:

  1) There are things KGL does that aren't really necessary or germane
     for non-KGL usage, and you're trying to avoid initializing all
     of KGL, right? :)
  2) KGL is a work in progress and is attempting over time to become
     more and more GL compliant. On the other hand, we just want simple
     and working 3D matrix functions.
 */


/* Degrees<->Radians conversion */
#define DEG2RAD (F_PI / 180.0f)
#define RAD2DEG (180.0f / F_PI)

/* Matrix stacks */
#define MAT_MV_STACK_CNT 32
#define MAT_P_STACK_CNT 2
static matrix_t mat_mv_stack[MAT_MV_STACK_CNT] __attribute__((aligned(32)));
static int mat_mv_stack_top;
static matrix_t mat_p_stack[MAT_P_STACK_CNT] __attribute__((aligned(32)));
static int mat_p_stack_top;

/* Active mode matrices */
static int matrix_mode;
static matrix_t trans_mats[PLX_MAT_COUNT] __attribute__((aligned(32)));

/* Screenview parameters */
static vector_t vp, vp_size, vp_scale, vp_offset;
static float vp_z_fudge;
static float depthrange_near, depthrange_far;

/* Set which matrix we are working on */
void plx_mat3d_mode(int mode) {
	matrix_mode = mode;
}

/* Load the identitiy matrix */
void plx_mat3d_identity() {
	mat_identity();
	mat_store(trans_mats + matrix_mode);
}

static matrix_t ml __attribute__((aligned(32)));

/* Load an arbitrary matrix */
void plx_mat3d_load(matrix_t * m) {
	memcpy_sh4(trans_mats + matrix_mode, m, sizeof(matrix_t));
}

/* Save the matrix (whoa!) */
void plx_mat3d_store(matrix_t * m) {
	memcpy_sh4(m, trans_mats + matrix_mode, sizeof(matrix_t));
}

/* Set the depth range */
void plx_mat3d_depthrange(float n, float f) {
	/* clamp the values... */
	if (n < 0.0f) n = 0.0f;
	if (n > 1.0f) n = 1.0f;
	if (f < 0.0f) f = 0.0f;
	if (f > 1.0f) f = 1.0f;

	depthrange_near = n;
	depthrange_far = f;

	/* Adjust the viewport scale and offset for Z */
	vp_scale.z = ((f - n) / 2.0f);
	vp_offset.z = (n + f) / 2.0f;
}

/* Set the viewport */
void plx_mat3d_viewport(int x1, int y1, int width, int height) {
	vp.x = x1;
	vp.y = y1;
	vp_size.x = width;
	vp_size.y = height;

	/* Calculate the viewport scale and offset */
	vp_scale.x = (float)width / 2.0f;
	vp_offset.x = vp_scale.x + (float)x1;
	vp_scale.y = (float)height / 2.0f;
	vp_offset.y = vp_scale.y + (float)y1;
	vp_scale.z = (depthrange_far -depthrange_near) / 2.0f;
	vp_offset.z = (depthrange_near + depthrange_far) / 2.0f;

	/* FIXME: Why does the depth value need some nudging?
         * This makes polys with Z=0 work.
	 */
	vp_offset.z += 0.0001f;
}

/* Setup perspective */
void plx_mat3d_perspective(float angle, float aspect, 
	float znear, float zfar)
{
	float xmin, xmax, ymin, ymax;

	ymax = znear * ftan(angle * F_PI / 360.0f);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	plx_mat3d_frustum(xmin, xmax, ymin, ymax, znear, zfar);
}

static matrix_t mf __attribute__((aligned(32))) = {
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, -1.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f }
};

void plx_mat3d_frustum(float left, float right,
	float bottom, float top,
	float znear, float zfar)
{
	float x, y, a, b, c, d;

	assert(znear > 0.0f);
	x = (2.0f * znear) / (right - left);
	y = (2.0f * znear) / (top - bottom);
	a = (right + left) / (right - left);
	b = (top + bottom) / (top - bottom);
	c = -(zfar + znear) / (zfar - znear);
	d = -(2.0f * zfar * znear) / (zfar - znear);

	mf[0][0] = x;
	mf[2][0] = a;
	mf[1][1] = y;
	mf[2][1] = b;
	mf[2][2] = c;
	mf[3][2] = d;

	mat_load(trans_mats + matrix_mode);
	mat_apply(&mf);
	mat_store(trans_mats + matrix_mode);
}

static matrix_t mo __attribute__((aligned(32))) = {
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

void plx_mat3d_ortho(float left, float right, float bottom, float top, float znear, float zfar) {

	float x, y, z;
	float tx, ty, tz;

	x = 2.0f / (right - left);
	y = 2.0f / (top - bottom);
	z = -2.0f / (zfar - znear);
	tx = -(right + left) / (right - left);
	ty = -(top + bottom) / (top - bottom);
	tz = -(zfar + znear) / (zfar - znear);

	mo[0][0] = x;
	mo[1][1] = y;
	mo[2][2] = z;
	mo[3][0] = tx;
	mo[3][1] = ty;
	mo[3][2] = tz;

	mat_load(trans_mats + matrix_mode);
	mat_apply(&mo);
	mat_store(trans_mats + matrix_mode);
}

void plx_mat3d_push() {
	switch (matrix_mode)
	{
	case PLX_MAT_MODELVIEW:
		assert_msg(mat_mv_stack_top < MAT_MV_STACK_CNT, "Modelview stack overflow.");
		if (mat_mv_stack_top >= MAT_MV_STACK_CNT) return;

		memcpy_sh4(mat_mv_stack + mat_mv_stack_top,
			trans_mats + matrix_mode,
			sizeof(matrix_t));
		mat_mv_stack_top++;
		break;
	case PLX_MAT_PROJECTION:
		assert_msg(mat_p_stack_top < MAT_P_STACK_CNT, "Projection stack overflow.");
		if (mat_p_stack_top >= MAT_P_STACK_CNT) return;

		memcpy_sh4(mat_p_stack + mat_p_stack_top,
			trans_mats + matrix_mode,
			sizeof(matrix_t));
		mat_p_stack_top++;
		break;
	default:
		assert_msg( 0, "Invalid matrix type" );
	}
}

void plx_mat3d_pop() {
	switch(matrix_mode)
	{
	case PLX_MAT_MODELVIEW:
		assert_msg(mat_mv_stack_top > 0, "Modelview stack underflow.");
		if (mat_mv_stack_top <= 0) return;

		mat_mv_stack_top--;
		memcpy_sh4(trans_mats + matrix_mode,
			mat_mv_stack + mat_mv_stack_top,
			sizeof(matrix_t));
		break;
	case PLX_MAT_PROJECTION:
		assert_msg(mat_p_stack_top > 0, "Projection stack underflow.");
		if (mat_p_stack_top <= 0) return;

		mat_p_stack_top--;
		memcpy_sh4(trans_mats + matrix_mode,
			mat_p_stack + mat_p_stack_top,
			sizeof(matrix_t));
		break;
	default:
		assert_msg( 0, "Invalid matrix type" );
	}
}

void plx_mat3d_peek() {
	switch(matrix_mode)
	{
	case PLX_MAT_MODELVIEW:
		assert_msg(mat_mv_stack_top > 0, "Modelview stack underflow.");
		if (mat_mv_stack_top <= 0) return;

		memcpy_sh4(trans_mats + matrix_mode,
			mat_mv_stack + mat_mv_stack_top - 1,
			sizeof(matrix_t));
		break;
	case PLX_MAT_PROJECTION:
		assert_msg(mat_p_stack_top > 0, "Projection stack underflow.");
		if (mat_p_stack_top <= 0) return;

		memcpy_sh4(trans_mats + matrix_mode,
			mat_p_stack + mat_p_stack_top - 1,
			sizeof(matrix_t));
		break;
	default:
		assert_msg( 0, "Invalid matrix type" );
	}
}

static matrix_t mr __attribute__((aligned(32))) = {
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

void plx_mat3d_rotate(float angle, float x, float y, float z) {
	float rcos = fcos(angle * DEG2RAD);
	float rsin = fsin(angle * DEG2RAD);
	float invrcos = (1.0f - rcos);
	float mag = fsqrt(x*x + y*y + z*z);
	float xx, yy, zz, xy, yz, zx;

	if (mag < 1.0e-6) {
		/* Rotation vector is too small to be significant */
		return;
	}

	/* Normalize the rotation vector */
	x /= mag;
	y /= mag;
	z /= mag;

	xx = x * x;
	yy = y * y;
	zz = z * z;
	xy = (x * y * invrcos);
	yz = (y * z * invrcos);
	zx = (z * x * invrcos);

	/* Generate the rotation matrix */
	mr[0][0] = xx + rcos * (1.0f - xx);
	mr[2][1] = yz - x * rsin;
	mr[1][2] = yz + x * rsin;

	mr[1][1] = yy + rcos * (1.0f - yy);
	mr[2][0] = zx + y * rsin;
	mr[0][2] = zx - y * rsin;

	mr[2][2] = zz + rcos * (1.0f - zz);
	mr[1][0] = xy - z * rsin;
	mr[0][1] = xy + z * rsin;

	mat_load(trans_mats + matrix_mode);
	mat_apply(&mr);
	mat_store(trans_mats + matrix_mode);
}

static matrix_t ms __attribute__((aligned(32))) = {
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

void plx_mat3d_scale(float x, float y, float z) {
        ms[0][0] = x;
	ms[1][1] = y;
	ms[2][2] = z;

	mat_load(trans_mats + matrix_mode);
	mat_apply(&ms);
	mat_store(trans_mats + matrix_mode);
}

static matrix_t mt __attribute__((aligned(32))) = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

void plx_mat3d_translate(float x, float y, float z) {
        mt[3][0] = x;
	mt[3][1] = y;
	mt[3][2] = z;

	mat_load(trans_mats + matrix_mode);
	mat_apply(&mt);
	mat_store(trans_mats + matrix_mode);
}

static void normalize(vector_t * p) {
	float r;

	r = fsqrt( p->x*p->x + p->y*p->y + p->z*p->z );
	if (r == 0.0) return;

	p->x /= r;
	p->y /= r;
	p->z /= r;
}

static void cross(const vector_t * v1, const vector_t * v2, vector_t * r) {
	r->x = v1->y*v2->z - v1->z*v2->y;
	r->y = v1->z*v2->x - v1->x*v2->z;
	r->z = v1->x*v2->y - v1->y*v2->x;
}

static matrix_t ml __attribute__((aligned(32))) = {
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};

void plx_mat3d_lookat(const point_t * eye, const point_t * center, const vector_t * upi) {
	point_t forward, side, up;

	forward.x = center->x - eye->x;
	forward.y = center->y - eye->y;
	forward.z = center->z - eye->z;

	up.x = upi->x;
	up.y = upi->y;
	up.z = upi->z;

	normalize(&forward);

	/* Side = forward x up */
	cross(&forward, &up, &side);
	normalize(&side);

	/* Recompute up as: up = side x forward */
	cross(&side, &forward, &up);

	ml[0][0] = side.x;
	ml[1][0] = side.y;
	ml[2][0] = side.z;

	ml[0][1] = up.x;
	ml[1][1] = up.y;
	ml[2][1] = up.z;

	ml[0][2] = -forward.x;
	ml[1][2] = -forward.y;
	ml[2][2] = -forward.z;

	mat_load(trans_mats + matrix_mode);
	mat_apply(&ml);
	mat_translate(-eye->x, -eye->y, -eye->z);
	mat_store(trans_mats + matrix_mode);
}

static matrix_t msv __attribute__((aligned(32))) = {
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};

/* Apply a matrix */
void plx_mat3d_apply(int mode) {
	if (mode != PLX_MAT_SCREENVIEW) {
		mat_apply(trans_mats + mode);
	} else {
		msv[0][0] = vp_scale.x;
		msv[1][1] = -vp_scale.y;
		msv[3][0] = vp_offset.x;
		msv[3][1] = vp_size.y - vp_offset.y;
		mat_apply(&msv);
	}
}

static matrix_t mam __attribute__((aligned(32))) = {
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};

void plx_mat3d_apply_mat(matrix_t * mat) {
	memcpy_sh4(&mam, mat, sizeof(matrix_t));
	mat_load(trans_mats + matrix_mode);
	mat_apply(&mam);
	mat_store(trans_mats + matrix_mode);
}

void plx_mat3d_apply_all() {
	mat_identity();

	msv[0][0] = vp_scale.x;
	msv[1][1] = -vp_scale.y;
	msv[3][0] = vp_offset.x;
	msv[3][1] = vp_size.y - vp_offset.y;
	mat_apply(&msv);

	mat_apply(trans_mats + PLX_MAT_PROJECTION);
	mat_apply(trans_mats + PLX_MAT_WORLDVIEW);
	mat_apply(trans_mats + PLX_MAT_MODELVIEW);
}


/* Init */
void plx_mat3d_init() {
	int i;

	/* Setup all the matrices */
	mat_identity();
	for (i=0; i<PLX_MAT_COUNT; i++)
		mat_store(trans_mats + i);
	matrix_mode = PLX_MAT_PROJECTION;
	mat_mv_stack_top = 0;
	mat_p_stack_top = 0;

	/* Setup screen w&h */
	plx_mat3d_depthrange(0.0f, 1.0f);
	plx_mat3d_viewport(0, 0, vid_mode->width, vid_mode->height);
	vp_z_fudge = 0.0f;
}

