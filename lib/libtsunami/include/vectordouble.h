/*
   Tsunami for KallistiOS ##version##

   vectordouble.h

   Copyright (C) 2002, 2003, 2004 Megan Potter

*/

#ifndef __TSUNAMI_VECTORDOUBLE_H
#define __TSUNAMI_VECTORDOUBLE_H

#include <assert.h>
#include <math.h>

class MatrixDouble;
class Vector;

/// A C++ friendly wrapper for the point_t / vector_t struct
class VectorDouble {
public:
	VectorDouble(double ix, double iy, double iz, double iw = 0.0)
		: x(ix), y(iy), z(iz), w(iw) { }
	VectorDouble() { }

	VectorDouble(const Vector & o);

	/// Return one of the vector elements array-style
	double operator[](int i) const {
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		else if (i == 3)
			return w;
		else {
			assert_msg(false, "Point::operator[] Invalid index");
			return 0.0;
		}
	}

	/// Copy a VectorDouble into a Vector
	Vector truncate() const;

	/// Compare two vectors for equality
	bool operator==(const VectorDouble & other) const {
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	/// Compare two vectors for inequality
	bool operator!=(const VectorDouble & other) const {
		return !(*this == other);
	}

	/// Add two vectors
	VectorDouble operator+(const VectorDouble & other) const {
		return VectorDouble(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	/// Subtract two vectors
	VectorDouble operator-(const VectorDouble & other) const {
		return VectorDouble(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	/// Unary minus
	VectorDouble operator-() const {
		return VectorDouble(-x, -y, -z);
	}

	/// Multiply by a scalar
	VectorDouble operator*(double s) const {
		return VectorDouble(x * s, y * s, z * s, w * s);
	}

	/// Inline add two vectors
	VectorDouble & operator+=(const VectorDouble & other) {
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	/// Inline subtract two vectors
	VectorDouble & operator-=(const VectorDouble & other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}

	// Inline multiply by a scalar
	VectorDouble & operator*=(double s) {
		x *= s;
		y *= s;
		z *= s;
		w *= s;
		return *this;
	}

	/// Zero this vector out.
	void zero() {
		x = y = z = w = 0;
	}

	/// Dot product with another vector.
	/// NOTE: Only takes x,y,z into account.
	double dot(const VectorDouble & other) const {
		return (x * other.x)
			+ (y * other.y)
			+ (z * other.z);
	}

	/// Cross product with another vector
	/// NOTE: Only takes x,y,z into account.
	VectorDouble cross(const VectorDouble & other) const {
		return VectorDouble(
			y * other.z - z*other.y,
			z * other.x - x*other.z,
			x * other.y - y*other.x);
	}

	/// Get the length/magnitude of the vector
	double length() const {
		return sqrt(x*x+y*y+z*z+w*w);
	}

	/// Returns 1.0/length()
	double rlength() const {
		return 1.0/length();
	}

	/// Normalize this vector in place.
	VectorDouble & normalizeSelf() {
		double l = rlength();
		x = x * l;
		y = y * l;
		z = z * l;
		w = w * l;
		return *this;
	}

	/// Normalize this vector and return a new one.
	VectorDouble normalize() const {
		double l = rlength();
		return VectorDouble(
			x * l,
			y * l,
			z * l,
			w * l);
	}

	/// Multiply this vector with a matrix.
	VectorDouble operator*(const MatrixDouble & mat) const;

	/// Operator *= to multiply with a matrix.
	VectorDouble & operator*=(const MatrixDouble & mat);

public:
	double	x, y, z, w;
};

#endif	/* __TSUNAMI_VECTORDOUBLE_H */
