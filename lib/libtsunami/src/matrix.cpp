/*
   Tsunami for KallistiOS ##version##

   matrix.cpp

   Copyright (C) 2001, 2002, 2003 Megan Potter
   Copyright (C) 2002 Benoit Miller and Paul Boese
*/

#include "matrix.h"
#include <string.h>

/* Several pieces of this file were pulled from libparallax, which was
   in turn pulled from KGL. */

Matrix::Matrix() {
}


Matrix::Matrix(const Matrix & other) {
	memcpy(&matrix, &other.matrix, sizeof(matrix));
}

Matrix::Matrix(const matrix_t & other) {
	memcpy(&matrix, &other, sizeof(matrix_t));
}

Matrix & Matrix::operator=(const Matrix & other) {
	memcpy(&matrix, &other.matrix, sizeof(matrix));
	return *this;
}



void Matrix::identity() {
	memset(&matrix, 0, sizeof(matrix));
	matrix[0][0] = 1.0f;
	matrix[1][1] = 1.0f;
	matrix[2][2] = 1.0f;
	matrix[3][3] = 1.0f;
}

#define DEG2RAD (F_PI / 180.0f)
void Matrix::rotate(float angle, const Vector & axis) {
	float vx = axis.x, vy = axis.y, vz = axis.z;
	float rcos = fcos(angle * DEG2RAD);
	float rsin = fsin(angle * DEG2RAD);
	float invrcos = (1.0f - rcos);
	float mag = fsqrt(vx*vx + vy*vy + vz*vz);
	float xx, yy, zz, xy, yz, zx;

	if (mag < 1.0e-6) {
		// Rotation vector is too small to be significant
		return;
	}

	// Normalize the rotation vector
	vx /= mag;
	vy /= mag;
	vz /= mag;

	xx = vx * vx;
	yy = vy * vy;
	zz = vz * vz;
	xy = (vx * vy * invrcos);
	yz = (vy * vz * invrcos);
	zx = (vz * vx * invrcos);

	// Generate the rotation matrix
	Matrix mr; mr.identity();

	// Hehe
	mr.matrix[0][0] = xx + rcos * (1.0f - xx);
	mr.matrix[2][1] = yz - vx * rsin;
	mr.matrix[1][2] = yz + vx * rsin;

	mr.matrix[1][1] = yy + rcos * (1.0f - yy);
	mr.matrix[2][0] = zx + vy * rsin;
	mr.matrix[0][2] = zx - vy * rsin;

	mr.matrix[2][2] = zz + rcos * (1.0f - zz);
	mr.matrix[1][0] = xy - vz * rsin;
	mr.matrix[0][1] = xy + vz * rsin;

	// Multiply it onto us
	*this = *this * mr;
}

void Matrix::scale(const Vector & scale) {
	Matrix ms; ms.identity();

	ms.matrix[0][0] = scale.x;
	ms.matrix[1][1] = scale.y;
	ms.matrix[2][2] = scale.z;

	*this = *this * ms;
}

void Matrix::translate(const Vector & delta) {
	Matrix mt; mt.identity();

	mt.matrix[3][0] = delta.x;
	mt.matrix[3][1] = delta.y;
	mt.matrix[3][2] = delta.z;

	*this = *this * mt;
}

bool Matrix::operator==(const Matrix & other) const {
	int x, y;
	for (y=0; y<4; y++)
		for (x=0; x<4; x++)
			if (matrix[x][y] != other.matrix[x][y])
				return false;
	return true;
}

Matrix Matrix::operator+(const Matrix & other) const {
	Matrix nm;
	int x, y;

	for (y=0; y<4; y++)
		for (x=0; x<4; x++)
			nm.matrix[x][y] = matrix[x][y] + other.matrix[x][y];
	return nm;
}

Matrix Matrix::operator-(const Matrix & other) const {
	Matrix nm;
	int x, y;

	for (y=0; y<4; y++)
		for (x=0; x<4; x++)
			nm.matrix[x][y] = matrix[x][y] + other.matrix[x][y];
	return nm;
}

Matrix Matrix::operator-() const {
	Matrix nm;
	int x, y;

	for (y=0; y<4; y++)
		for (x=0; x<4; x++)
			nm.matrix[x][y] = -matrix[x][y];
	return nm;
}

// Thanks to Iris3D for this algorithm (I'm too lazy to look it up ;)
Matrix Matrix::operator*(const Matrix & other) const {
	Matrix nm;

	for (int i=0; i<4; i++) {
		for (int j=0; j<4; j++) {
			nm.matrix[i][j] = 0;
			for (int k=0; k<4; k++)
				nm.matrix[i][j] += matrix[i][k] * other.matrix[k][j];
		}
	}

	return nm;
}

// Pulled from Parallax
void Matrix::lookAt(const Vector & eye, const Vector & center, const Vector & upi) {
	Vector forward = (center - eye).normalize();

	// Side = forward x up
	Vector side = (forward.cross(upi)).normalize();

	// Recompute up as: up = side x forward
	Vector up = side.cross(forward);

	// Put the initial transformation in a tmp matrix
	Matrix mult; mult.identity();

	mult.matrix[0][0] = side.x;
	mult.matrix[1][0] = side.y;
	mult.matrix[2][0] = side.z;

	mult.matrix[0][1] = up.x;
	mult.matrix[1][1] = up.y;
	mult.matrix[2][1] = up.z;

	mult.matrix[0][2] = -forward.x;
	mult.matrix[1][2] = -forward.y;
	mult.matrix[2][2] = -forward.z;

	// Translate us
	translate(-eye);

	// And finish recomputing
	*this = (*this) * mult;
}
