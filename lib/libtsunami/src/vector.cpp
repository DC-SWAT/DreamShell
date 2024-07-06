/*
   Tsunami for KallistiOS ##version##

   vector.cpp

   Copyright (C) 2004 Megan Potter
*/

#include "vector.h"
#include "vector3.h"
#include "matrix.h"

// Thanks to Iris3D for this algorithm (I'm too lazy to look it up ;)
Vector Vector::operator*(const Matrix & mat) const {
	const matrix_t & m = mat.matrix;

	return Vector(
		x * m[0][0] + y * m[1][0] + z * m[2][0] + w * m[3][0],
		x * m[0][1] + y * m[1][1] + z * m[2][1] + w * m[3][1],
		x * m[0][2] + y * m[1][2] + z * m[2][2] + w * m[3][2],
		x * m[0][3] + y * m[1][3] + z * m[2][3] + w * m[3][3] );
}

Vector & Vector::operator*=(const Matrix & mat) {
	*this = *this * mat;
	return *this;
}

Vector & Vector::operator=(const Vector3 & o) {
	x = o.x;
	y = o.y;
	z = o.z;
	w = 0.0f;
	return *this;
}
