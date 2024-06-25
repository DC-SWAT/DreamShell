/*
   Tsunami for KallistiOS ##version##

   vector3.h

   Copyright (C) 2002, 2003, 2004 Megan Potter

*/

#ifndef __TSUNAMI_VECTOR3_H
#define __TSUNAMI_VECTOR3_H

#include <kos/vector.h>
#include <assert.h>

#ifdef _arch_dreamcast
#	include <dc/fmath.h>
#else
#	include <math.h>
#endif

class Matrix;

/// This is a leaner, meaner version of Vector with no W component.
class Vector3 {
public:
	Vector3(float ix, float iy, float iz)
		: x(ix), y(iy), z(iz) { }
	Vector3() { }

	/// Return one of the vector elements array-style
	float operator[](int i) const {
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		else {
			assert_msg(false, "Point::operator[] Invalid index");
			return 0.0f;
		}
	}

	/// Compare two vectors for equality
	bool operator==(const Vector3 & other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	/// Compare two vectors for inequality
	bool operator!=(const Vector3 & other) const {
		return !(*this == other);
	}

	/// Add two vectors
	Vector3 operator+(const Vector3 & other) const {
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	/// Subtract two vectors
	Vector3 operator-(const Vector3 & other) const {
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	/// Unary minus
	Vector3 operator-() const {
		return Vector3(-x, -y, -z);
	}

	/// Multiply by a scalar
	Vector3 operator*(float s) const {
		return Vector3(x * s, y * s, z * s);
	}

	/// Inline add two vectors
	Vector3 & operator+=(const Vector3 & other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	/// Inline subtract two vectors
	Vector3 & operator-=(const Vector3 & other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	// Inline multiply by a scalar
	Vector3 & operator*=(float s) {
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

	/// Get a C vector_t struct out of it
	operator vector_t() const {
		vector_t v = { x, y, z, 0.0f };
		return v;
	}

	/// Zero this vector out.
	void zero() {
		x = y = z = 0;
	}

	/// Dot product with another vector.
	/// NOTE: Only takes x,y,z into account.
	float dot(const Vector3 & other) const {
		return (x * other.x)
			+ (y * other.y)
			+ (z * other.z);
	}

	/// Cross product with another vector
	/// NOTE: Only takes x,y,z into account.
	Vector3 cross(const Vector3 & other) const {
		return Vector3(
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
	Vector3 & normalizeSelf() {
		float l = rlength();
		x = x * l;
		y = y * l;
		z = z * l;
		return *this;
	}

	/// Normalize this vector and return a new one.
	Vector3 normalize() const {
		float l = rlength();
		return Vector3(
			x * l,
			y * l,
			z * l);
	}

	/// Multiply this vector with a matrix.
	Vector3 operator*(const Matrix & mat) const;

	/// Operator *= to multiply with a matrix.
	Vector3 & operator*=(const Matrix & mat);

public:
	float	x, y, z;
};

#endif	/* __TSUNAMI_VECTOR3_H */
