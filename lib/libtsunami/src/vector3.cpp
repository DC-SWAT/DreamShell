/*
   Tsunami for KallistiOS ##version##

   vector3.cpp

   Copyright (C) 2004 Megan Potter
*/

#include "vector3.h"
#include "matrix.h"

// Thanks to Iris3D for this algorithm (I'm too lazy to look it up ;)
Vector3 Vector3::operator*(const Matrix & mat) const {
	const matrix_t & m = mat.matrix;

	return Vector3(
		x * m[0][0] + y * m[1][0] + z * m[2][0],
		x * m[0][1] + y * m[1][1] + z * m[2][1],
		x * m[0][2] + y * m[1][2] + z * m[2][2] );
}

Vector3 & Vector3::operator*=(const Matrix & mat) {
	*this = *this * mat;
	return *this;
}
