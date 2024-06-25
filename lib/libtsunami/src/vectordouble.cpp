/*
   Tsunami for KallistiOS ##version##

   vectordouble.cpp

   Copyright (C) 2004 Megan Potter
*/

#include "vectordouble.h"
#include "vector.h"
#include "matrixdouble.h"

VectorDouble::VectorDouble(const Vector & o) {
	x = (double)o.x;
	y = (double)o.y;
	z = (double)o.z;
	w = (double)o.w;
}

// Thanks to Iris3D for this algorithm (I'm too lazy to look it up ;)
VectorDouble VectorDouble::operator*(const MatrixDouble & mat) const {
	const MatrixDouble::matrix_t & m = mat.matrix;

	return VectorDouble(
		x * m[0][0] + y * m[1][0] + z * m[2][0] + w * m[3][0],
		x * m[0][1] + y * m[1][1] + z * m[2][1] + w * m[3][1],
		x * m[0][2] + y * m[1][2] + z * m[2][2] + w * m[3][2],
		x * m[0][3] + y * m[1][3] + z * m[2][3] + w * m[3][3] );
}

VectorDouble & VectorDouble::operator*=(const MatrixDouble & mat) {
	*this = *this * mat;
	return *this;
}

Vector VectorDouble::truncate() const {
	return Vector(
		(float)x,
		(float)y,
		(float)z,
		(float)w);
}
