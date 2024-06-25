/*
   Tsunami for KallistiOS ##version##

   vector.h

   Copyright (C) 2002, 2003, 2004 Megan Potter

*/

#ifndef __TSUNAMI_VECTOR_H
#define __TSUNAMI_VECTOR_H

#include <kos/vector.h>
#include <assert.h>

// We can only do the hardware transforms if this is a DC.
#ifdef _arch_dreamcast
#	include <dc/fmath.h>
#else
#	include <math.h>
#endif

class Matrix;
class Vector3;

/// A C++ friendly wrapper for the point_t / vector_t struct. Note that
/// the 'w' value is not actually a fourth dimension but rather a scaling
/// factor for a homogeneous coordinate. i.e. the real vector/point expressed
/// by <x,y,z,w> is actually <x/w,y/w,z/w>.
class Vector {
public:
	Vector(float ix, float iy, float iz, float iw = 1.0f)
		: x(ix), y(iy), z(iz), w(iw) { }
	Vector() { }

	/// Return one of the vector elements array-style
	float operator[](int i) const {
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
			return 0.0f;
		}
	}

	/// Copy a Vector3 into a Vector
	Vector & operator=(const Vector3 & other);

	/// Compare two vectors for equality
	bool operator==(const Vector & other) const {
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	/// Compare two vectors for inequality
	bool operator!=(const Vector & other) const {
		return !(*this == other);
	}

	/// Add two vectors
	Vector operator+(const Vector & other) const {
		return Vector(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	/// Subtract two vectors
	Vector operator-(const Vector & other) const {
		return Vector(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	/// Unary minus
	Vector operator-() const {
		return Vector(-x, -y, -z);
	}

	/// Multiply by a scalar
	Vector operator*(float s) const {
		return Vector(x * s, y * s, z * s, w * s);
	}

	/// Inline add two vectors
	Vector & operator+=(const Vector & other) {
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

	/// Inline subtract two vectors
	Vector & operator-=(const Vector & other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}

	// Inline multiply by a scalar
	Vector & operator*=(float s) {
		x *= s;
		y *= s;
		z *= s;
		w *= s;
		return *this;
	}

	/// Get a C vector_t struct out of it
	operator vector_t() const {
		vector_t v = { x, y, z, w };
		return v;
	}

	/// Zero this vector out.
	void zero() {
		x = y = z = w = 0;
	}

	/// Dot product with another vector.
	/// NOTE: Only takes x,y,z into account.
	float dot(const Vector & other) const {
		return (x * other.x)
			+ (y * other.y)
			+ (z * other.z);
	}

	/// Cross product with another vector
	/// NOTE: Only takes x,y,z into account.
	Vector cross(const Vector & other) const {
		return Vector(
			y * other.z - z*other.y,
			z * other.x - x*other.z,
			x * other.y - y*other.x);
	}

	/// Get the length/magnitude of the vector
	float length() const {
#ifdef _arch_dreamcast
		return fsqrt(x*x+y*y+z*z);
#else
		return (float)sqrt(x*x+y*y+z*z);
#endif
	}

	/// Returns 1.0/length()
	float rlength() const {
#ifdef _arch_dreamcast
		return frsqrt(x*x+y*y+z*z);
#else
		return 1.0f/length();
#endif
	}

	/// Normalize this vector in place.
	Vector & normalizeSelf() {
		float l = rlength();
		x = x * l;
		y = y * l;
		z = z * l;
		w = w * l;
		return *this;
	}

	/// Normalize this vector and return a new one.
	Vector normalize() const {
		float l = rlength();
		return Vector(
			x * l,
			y * l,
			z * l,
			w * l);
	}

	/// Multiply this vector with a matrix.
	Vector operator*(const Matrix & mat) const;

	/// Operator *= to multiply with a matrix.
	Vector & operator*=(const Matrix & mat);

public:
	float	x, y, z, w;
};

#endif	/* __TSUNAMI_VECTOR_H */
