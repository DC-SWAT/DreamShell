/*
   Tsunami for KallistiOS ##version##

   matrix.h

   Copyright (C) 2003 Megan Potter

*/

#ifndef __TSUNAMI_MATRIX_H
#define __TSUNAMI_MATRIX_H

#include <kos/vector.h>
#include "vector.h"

#ifdef _arch_dreamcast
#	include <dc/fmath.h>
#else
#	error Architecture not supported yet
#endif

/// A C++ friendly wrapper for the matrix_t struct
class Matrix {
public:
	Matrix();
	Matrix(const Matrix & other);
	Matrix(const matrix_t & other);

	Matrix & operator=(const Matrix & other);

	/// Set us to the identity matrix
	void identity();

	/// Do an arbitrary rotation on the matrix
	void rotate(float angle, const Vector & axis);

	/// Do a scale operation on the matrix
	void scale(const Vector & scale);

	/// Do a translation operation on the matrix
	void translate(const Vector & delta);

	/// Do a look-at operation on the matrix (i.e. camera)
	void lookAt(const Vector & pos, const Vector & lookAt, const Vector & up);

	/// Compare two matrices for equality
	bool operator==(const Matrix & other) const;

	/// Compare two matrices for inequality
	bool operator!=(const Matrix & other) const {
		return !(*this == other);
	}

	/// Add two matrices
	Matrix operator+(const Matrix & other) const;

	/// Subtract two matrices
	Matrix operator-(const Matrix & other) const;

	/// Unary minus
	Matrix operator-() const;

	/// Inline add two matrices
	Matrix & operator+=(const Matrix & other) {
		*this = *this + other;
		return *this;
	}

	/// Inline subtract two matrices
	Matrix & operator-=(const Matrix & other) {
		*this = *this - other;
		return *this;
	}

	/// Matrix multiply (aka mat_apply)
	Matrix operator*(const Matrix & other) const;

public:
	matrix_t	matrix;
};

#endif	/* __TSUNAMI_MATRIX_H */
