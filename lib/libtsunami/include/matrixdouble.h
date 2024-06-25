/*
   Tsunami for KallistiOS ##version##

   matrixdouble.h

   Copyright (C) 2003, 2004 Megan Potter

*/

#ifndef __TSUNAMI_MATRIXDOUBLE_H
#define __TSUNAMI_MATRIXDOUBLE_H

#include "vectordouble.h"

class Matrix;

/// A C++ friendly wrapper for the matrix_t struct
class MatrixDouble {
public:
	MatrixDouble();
	MatrixDouble(const MatrixDouble & other);

	MatrixDouble(const Matrix & o);

	MatrixDouble & operator=(const MatrixDouble & other);

	/// Get a Matrix from us
	Matrix truncate() const;

	/// Set us to the identity matrix
	void identity();

	/// Do an arbitrary rotation on the matrix
	void rotate(double angle, const VectorDouble & axis);

	/// Do a scale operation on the matrix
	void scale(const VectorDouble & scale);

	/// Do a translation operation on the matrix
	void translate(const VectorDouble & delta);

	/// Do a look-at operation on the matrix (i.e. camera)
	void lookAt(const VectorDouble & pos, const VectorDouble & lookAt, const VectorDouble & up);

	/// Compare two matrices for equality
	bool operator==(const MatrixDouble & other) const;

	/// Compare two matrices for inequality
	bool operator!=(const MatrixDouble & other) const {
		return !(*this == other);
	}

	/// Add two matrices
	MatrixDouble operator+(const MatrixDouble & other) const;

	/// Subtract two matrices
	MatrixDouble operator-(const MatrixDouble & other) const;

	/// Unary minus
	MatrixDouble operator-() const;

	/// Inline add two matrices
	MatrixDouble & operator+=(const MatrixDouble & other) {
		*this = *this + other;
		return *this;
	}

	/// Inline subtract two matrices
	MatrixDouble & operator-=(const MatrixDouble & other) {
		*this = *this - other;
		return *this;
	}

	/// Matrix multiply (aka mat_apply)
	MatrixDouble operator*(const MatrixDouble & other) const;

public:
	typedef double matrix_t[4][4];
	matrix_t	matrix;
};

#endif	/* __TSUNAMI_MATRIXDOUBLE_H */
