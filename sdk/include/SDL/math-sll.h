/*
 * $Id: math-sll.c,v 1.15 2002/08/20 18:01:54 andrewm Exp $
 *
 * Changed by CHUI
 *
 * Purpose
 *	A fixed point (31.32 bit) math library.
 *
 * Description
 *	Floating point packs the most accuracy in the available bits, but it
 *	often provides more accuracy than is required.  It is time consuming to
 *	carry the extra precision around, particularly on platforms that don't
 *	have a dedicated floating point processor.
 *
 *	This library is a compromise.  All math is done using the 64 bit signed
 *	"long long" format (sll), and is not intended to be portable, just as
 *	fast as possible.  Since "long long" is a elementary type, it can be
 *	passed around without resorting to the use of pointers.  Since the
 *	format used is fixed point, there is never a need to do time consuming
 *	checks and adjustments to maintain normalized numbers, as is the case
 *	in floating point.
 *
 *	Simply put, this library is limited to handling numbers with a whole
 *	part of up to 2^31 - 1 = 2.147483647e9 in magnitude, and fractional
 *	parts down to 2^-32 = 2.3283064365e-10 in magnitude.  This yields a
 *	decent range and accuracy for many applications.
 *
 * IMPORTANT
 *	No checking for arguments out of range (error).
 *	No checking for divide by zero (error).
 *	No checking for overflow (error).
 *	No checking for underflow (warning).
 *	Chops, doesn't round.
 *
 * Functions
 *	sll dbl2sll(double x)			double -> sll
 *	double slldbl(sll x)			sll -> double
 *
 *	sll slladd(sll x, sll y)		x + y
 *	sll sllsub(sll x, sll y)		x - y
 *	sll sllmul(sll x, sll y)		x * y
 *	sll slldiv(sll x, sll y)		x / y
 *
 *	sll sllinv(sll v)			1 / x
 *	sll sllmul2(sll x)			x * 2
 *	sll sllmul4(sll x)			x * 4
 *	sll sllmul2n(sll x, int n)		x * 2^n, 0 <= n <= 31
 *	sll slldiv2(sll x)			x / 2
 *	sll slldiv4(sll x)			x / 4
 *	sll slldiv2n(sll x, int n)		x / 2^n, 0 <= n <= 31
 *
 *	sll sllcos(sll x)			cos x
 *	sll sllsin(sll x)			sin x
 *	sll slltan(sll x)			tan x
 *	sll sllatan(sll x)			atan x
 *
 *	sll sllexp(sll x)			e^x
 *	sll slllog(sll x)			ln x
 *
 *	sll sllpow(sll x, sll y)		x^y
 *	sll sllsqrt(sll x)			x^(1 / 2)
 *
 */

#ifndef MATHSLL_H
#define MATHSLL_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_FIXED_POINT

#include <math.h>

/*
typedef float sll;
typedef float ull;
*/

typedef double sll;
typedef double ull;

#define int2sll(X)	((sll) (X))
#define sllvalue(X)	(X)
#define sll2int(X)	((int) (X))
#define sll_abs(X)	((sll)fabs((double)X))
#define sllabs(X)	((sll)fabs((double)X))
#define sllint(X)	((sll)floor((double)X))
#define sllfrac(X)	((X) - ((sll)sll2int(X)))
#define sllneg(X)	(-(X))
#define slladd(X,Y)	((X) + (Y))
#define sllsub(X,Y)	((X) - (Y))
#define sllmul(X,Y)	((X) * (Y))
#define slldiv(X,Y)	((X) / (Y))
#define sll2dbl(X)	((double)(X))
#define dbl2sll(X)	((sll)(X))
#define sllinv(X)	(((sll)1.0) / (X))
#define sllmul2(X)	(((sll)2.0) * (X))
#define sllmul4(X)	(((sll)4.0) * (X))
#define sllmul2n(X,N)	(((sll)(1<<N)) * (X))
#define slldiv2(X)	((X) / ((sll)2.0))
#define slldiv4(X)	((X) / ((sll)4.0))
#define slldiv2n(X,N)	((X) / ((sll)(1<<N)))
#define sllcos(X)	((sll)cos((double)(X)))
#define sllsin(X)	((sll)sin((double)(X)))
#define slltan(X)	((sll)tan((double)(X)))
#define sllatan(X)	((sll)atan((double)(X)))
#define sllexp(X)	((sll)exp((double)(X)))
#define slllog(X)	((sll)log((double)(X)))
#define sllpow(X)	((sll)pow((double)(X)))
#define sllsqrt(X)	((sll)sqrt((double)(X)))



#define sllrotr(X,N)	((X) / ((sll)(1<<N)))
#define sllrotl(X,N)	((X) * ((sll)(1<<N)))

#define SLL_CONST_0		((sll)0.0)
#define SLL_CONST_1		((sll)1.0)
#define SLL_CONST_2		((sll)2.0)
#define SLL_CONST_3		((sll)3.0)
#define SLL_CONST_4		((sll)4.0)
#define SLL_CONST_10		((sll)10.0)
#define SLL_CONST_15		((sll)15.0)
#define SLL_CONST_1_2		((sll)0.5)
#define SLL_CONST_1_3		((sll)0.33333333333333333333333333333333)
#define SLL_CONST_1_4		((sll)0.25)
#define SLL_CONST_1_5		((sll)0.2)
#define SLL_CONST_1_6		((sll)0.16666666666666666666666666666667)
#define SLL_CONST_1_7		((sll)0.14285714285714285714285714285714)
#define SLL_CONST_1_8		((sll)0.125)
#define SLL_CONST_1_9		((sll)0.11111111111111111111111111111111)
#define SLL_CONST_1_10		((sll)0.10)
#define SLL_CONST_1_11		((sll)0.090909090909090909090909090909091)
#define SLL_CONST_1_12		((sll)0.083333333333333333333333333333333)
#define SLL_CONST_1_20		((sll)0.05)
#define SLL_CONST_1_30		((sll)0.033333333333333333333333333333333)
#define SLL_CONST_1_42		((sll)0.023809523809523809523809523809524)
#define SLL_CONST_1_56		((sll)0.017857142857142857142857142857143)
#define SLL_CONST_1_72		((sll)0.013888888888888888888888888888889)
#define SLL_CONST_1_90		((sll)0.011111111111111111111111111111111)
#define SLL_CONST_1_110		((sll)0.0090909090909090909090909090909091)
#define SLL_CONST_1_132		((sll)0.0075757575757575757575757575757576)
#define SLL_CONST_1_156		((sll)0.0064102564102564102564102564102564)
#define SLL_CONST_E		((sll)2.7182818284590452354)
#define SLL_CONST_1_E		((sll)0.36787944117144232159014926384893)
#define SLL_CONST_1_SQRTE	((sll)0.13533528323661269189004515421424)
#define SLL_CONST_LOG10_E	((sll)0.43429448190325182765747371064254)
#define SLL_CONST_PI		((sll)3.1415926535897932384626433832795)
#define SLL_CONST_2PI		((sll)6.283185307179586476925286766559)
#define SLL_CONST_4PI		((sll)12.566370614359172953850573533118)
#define SLL_CONST_PI_2		((sll)1.5707963267948966192313216916398)
#define SLL_CONST_PI_4		((sll)0.78539816339744830961566084581988)
#define SLL_CONST_PI_8		((sll)0.3926990816987242)
#define SLL_CONST_PI_12         ((sll)0.26179938779914943653855361527329)
#define SLL_CONST_1_PI		((sll)0.31830988618379067153776752674503)
#define SLL_CONST_2_PI		((sll)3.1415926535897932384626433832795)
#define SLL_CONST_ATAN_1	((sll)0.785398)


#else


/* Data types */
typedef signed long long sll;
typedef unsigned long long ull;

/* Macros */
#define int2sll(X)	(((sll) (X)) << 32)
// #define sllvalue(X)	(sll2dbl(X))
#define sllvalue(X)	(X)
#define sll2int(X)	((int) ((X) >> 32))
#define sll_abs(X)	((X) & 0xefffffffffffffffLL)
#define sllabs(A)	(((A)<SLL_CONST_0)? -(A):(A))
#define sllint(X)	((X) & 0xffffffff00000000LL)
#define sllfrac(X)	((X) & 0x00000000ffffffffLL)
#define sllneg(X)	(-(X))
#define _slladd(X,Y)	((X) + (Y))
#define _sllsub(X,Y)	((X) - (Y))

#define sllrotl(X,N)	((X)<<N)
#define sllrotr(X,N)	((X)>>N)

/* Constants (converted from double) */
#define SLL_CONST_0		0x0000000000000000LL
#define SLL_CONST_1		0x0000000100000000LL
#define SLL_CONST_2		0x0000000200000000LL
#define SLL_CONST_3		0x0000000300000000LL
#define SLL_CONST_4		0x0000000400000000LL
#define SLL_CONST_10	0x0000000a00000000LL
#define SLL_CONST_15	0x0000000f00000000LL
#define SLL_CONST_1_2	0x0000000080000000LL
#define SLL_CONST_1_3	0x0000000055555555LL
#define SLL_CONST_1_4	0x0000000040000000LL
#define SLL_CONST_1_5	0x0000000033333333LL
#define SLL_CONST_1_6	0x000000002aaaaaaaLL
#define SLL_CONST_1_7	0x0000000024924924LL
#define SLL_CONST_1_8	0x0000000020000000LL
#define SLL_CONST_1_9	0x000000001c71c71cLL
#define SLL_CONST_1_10	0x0000000019999999LL
#define SLL_CONST_1_11	0x000000001745d174LL
#define SLL_CONST_1_12	0x0000000015555555LL
#define SLL_CONST_1_20	0x000000000cccccccLL
#define SLL_CONST_1_30	0x0000000008888888LL
#define SLL_CONST_1_42	0x0000000006186186LL
#define SLL_CONST_1_56	0x0000000004924924LL
#define SLL_CONST_1_72	0x00000000038e38e3LL
#define SLL_CONST_1_90	0x0000000002d82d82LL
#define SLL_CONST_1_110	0x000000000253c825LL
#define SLL_CONST_1_132	0x0000000001f07c1fLL
#define SLL_CONST_1_156	0x0000000001a41a41LL
#define SLL_CONST_E		0x00000002b7e15162LL
#define SLL_CONST_1_E	0x000000005e2d58d8LL
#define SLL_CONST_SQRTE	0x00000001a61298e1LL
#define SLL_CONST_1_SQRTE	0x000000009b4597e3LL
#define SLL_CONST_LOG2_E	0x0000000171547652LL
#define SLL_CONST_LOG10_E	0x000000006f2dec54LL
#define SLL_CONST_LN2	0x00000000b17217f7LL
#define SLL_CONST_LN10	0x000000024d763776LL
#define SLL_CONST_PI	0x00000003243f6a88LL
#define SLL_CONST_2PI	0x00000006487ED510LL
#define SLL_CONST_4PI	0x0000000C90FDAA22LL
#define SLL_CONST_PI_2	0x00000001921fb544LL
#define SLL_CONST_PI_4	0x00000000c90fdaa2LL
#define SLL_CONST_PI_8  0x000000006487ED51LL
#define SLL_CONST_PI_12 0x00000000430548E0LL
#define SLL_CONST_1_PI	0x00000000517cc1b7LL
#define SLL_CONST_2_PI	0x00000000a2f9836eLL
#define SLL_CONST_2_SQRTPI	0x0000000120dd7504LL
#define SLL_CONST_SQRT2	0x000000016a09e667LL
#define SLL_CONST_1_SQRT2	0x00000000b504f333LL
#define SLL_CONST_ATAN_1	0x00000000C90FD7E4LL

static __inline__ double sll2dbl(sll s)
{

	union {
		double d;
		unsigned u[2];
		ull _ull;
		sll _sll;
	} in, retval;
	register unsigned exp;
	register unsigned flag;

	if (s == 0)
		return 0.0;

	/* Move into memory as args might be passed in regs */
	in._sll = s;

	/* Handle the negative flag */
	if (in._sll < 1) {
		flag = 0x80000000;
		in._ull = sllneg(in._sll);
	} else
		flag = 0x00000000;

	/* Normalize */
	for (exp = 1053; in._ull && (in.u[1] & 0x80000000) == 0; exp--) {
		in._ull <<= 1;
	}
	in._ull <<= 1;
	exp++;
	in._ull >>= 12;
	retval._ull = in._ull;
	retval.u[1] |= flag | (exp << 20);

#if defined(__arm__)

	/* ARM architecture has a big-endian double */
	exp = retval.u[0];
	retval.u[0] = retval.u[1];
	retval.u[1] = exp;

#endif /* defined(__arm__) */

	return retval.d;
}

static __inline__ sll slladd(sll x, sll y)
{
	return (x + y);
}

static __inline__ sll sllsub(sll x, sll y)
{
	return (x - y);
}

/*
 * Let a = A * 2^32 + a_hi * 2^0 + a_lo * 2^(-32)
 * Let b = B * 2^32 + b_hi * 2^0 + b_lo * 2^(-32)
 *
 * Where:
 *   *_hi is the integer part
 *   *_lo the fractional part
 *   A and B are the sign (0 for positive, -1 for negative).
 *
 * a * b = (A * 2^32 + a_hi * 2^0 + a_lo * 2^-32)
 *       * (B * 2^32 + b_hi * 2^0 + b_lo * 2^-32)
 *
 * Expanding the terms, we get:
 *
 *	 = A * B * 2^64 + A * b_h * 2^32 + A * b_l * 2^0
 *	 + a_h * B * 2^32 + a_h * b_h * 2^0 + a_h * b_l * 2^-32
 *	 + a_l * B * 2^0 + a_l * b_h * 2^-32 + a_l * b_l * 2^-64
 *
 * Grouping by powers of 2, we get:
 *
 *	 = A * B * 2^64
 *	 Meaningless overflow from sign extension - ignore
 *
 *	 + (A * b_h + a_h * B) * 2^32
 *	 Overflow which we can't handle - ignore
 *
 *	 + (A * b_l + a_h * b_h + a_l * B) * 2^0
 *	 We only need the low 32 bits of this term, as the rest is overflow
 *
 *	 + (a_h * b_l + a_l * b_h) * 2^-32
 *	 We need all 64 bits of this term
 *
 *	 +  a_l * b_l * 2^-64
 *	 We only need the high 32 bits of this term, as the rest is underflow
 *
 * Note that:
 *   a > 0 && b > 0: A =  0, B =  0 and the third term is a_h * b_h
 *   a < 0 && b > 0: A = -1, B =  0 and the third term is a_h * b_h - b_l
 *   a > 0 && b < 0: A =  0, B = -1 and the third term is a_h * b_h - a_l
 *   a < 0 && b < 0: A = -1, B = -1 and the third term is a_h * b_h - a_l - b_l
 */
#if defined(__arm__)
static __inline__ sll sllmul(sll left, sll right)
{
	/*
	 * From gcc/config/arm/arm.h:
	 *   In a pair of registers containing a DI or DF value the 'Q'
	 *   operand returns the register number of the register containing
	 *   the least significant part of the value.  The 'R' operand returns
	 *   the register number of the register containing the most
	 *   significant part of the value.
	 */
	sll retval;

	__asm__ (
		"@ sllmul\n\t"
		"umull	%R0, %Q0, %Q1, %Q2\n\t"
		"mul	%R0, %R1, %R2\n\t"
		"umlal	%Q0, %R0, %Q1, %R2\n\t"
		"umlal	%Q0, %R0, %Q2, %R1\n\t"
	        "tst	%R1, #0x80000000\n\t"
	        "subne	%R0, %R0, %Q2\n\t"
	        "tst	%R2, #0x80000000\n\t"
	        "subne	%R0, %R0, %Q1\n\t"
		: "=&r" (retval)
		: "%r" (left), "r" (right)
		: "cc"
	);

	return retval;
}
#elif defined(__i386__)
static __inline__ sll sllmul(sll left, sll right)
{
	register sll retval;
	__asm__(
		"# sllmul\n\t"
		"	movl	%1, %%eax\n\t"
		"	mull 	%3\n\t"
		"	movl	%%edx, %%ebx\n\t"
		"\n\t"
		"	movl	%2, %%eax\n\t"
		"	mull 	%4\n\t"
		"	movl	%%eax, %%ecx\n\t"
		"\n\t"
		"	movl	%1, %%eax\n\t"
		"	mull	%4\n\t"
		"	addl	%%eax, %%ebx\n\t"
		"	adcl	%%edx, %%ecx\n\t"
		"\n\t"
		"	movl	%2, %%eax\n\t"
		"	mull	%3\n\t"
		"	addl	%%ebx, %%eax\n\t"
		"	adcl	%%ecx, %%edx\n\t"
		"\n\t"
		"	btl	$31, %2\n\t"
		"	jnc	1f\n\t"
		"	subl	%3, %%edx\n\t"
		"1:	btl	$31, %4\n\t"
		"	jnc	1f\n\t"
		"	subl	%1, %%edx\n\t"
		"1:\n\t"
		: "=&A" (retval)
		: "m" (left), "m" (((unsigned *) &left)[1]),
		  "m" (right), "m" (((unsigned *) &right)[1])
		: "ebx", "ecx", "cc"
	);
	return retval;
}
#else
/* Plain C version: not optimal but portable. */
#warning Fixed Point no optimal
static __inline__ sll sllmul(sll a, sll b)
{
	unsigned int a_lo, b_lo;
	signed int a_hi, b_hi;
	sll x;

	a_lo = a;
	a_hi = (ull) a >> 32;
	b_lo = b;
	b_hi = (ull) b >> 32;

	x = ((ull) (a_hi * b_hi) << 32)
	  + (((ull) a_lo * b_lo) >> 32)
	  + (sll) a_lo * b_hi
	  + (sll) b_lo * a_hi;

	return x;
}
#endif

static __inline__ sll sllinv(sll v)
{
	int sgn = 0;
	sll u;
	ull s = -1; //0xFFFFFFFFFFFFFFFF; //-1;

	/* Use positive numbers, or the approximation won't work */
	if (v < SLL_CONST_0) {
		v = sllneg(v);
		sgn = 1;
	}

	/* An approximation - must be larger than the actual value */
	for (u = v; u; ((ull)u) >>= 1)
		s >>= 1;

	/* Newton's Method */
	u = sllmul(s, _sllsub(SLL_CONST_2, sllmul(v, s)));
	u = sllmul(u, _sllsub(SLL_CONST_2, sllmul(v, u)));
	u = sllmul(u, _sllsub(SLL_CONST_2, sllmul(v, u)));
	u = sllmul(u, _sllsub(SLL_CONST_2, sllmul(v, u)));
	u = sllmul(u, _sllsub(SLL_CONST_2, sllmul(v, u)));
	u = sllmul(u, _sllsub(SLL_CONST_2, sllmul(v, u)));

	return ((sgn) ? sllneg(u): u);
}

static __inline__ sll slldiv(sll left, sll right)
{
	return sllmul(left, sllinv(right));
}

static __inline__ sll sllmul2(sll x)
{
	return x << 1;
}

static __inline__ sll sllmul4(sll x)
{
	return x << 2;
}

static __inline__ sll sllmul2n(sll x, int n)
{
	sll y;

#if defined(__arm__)
	/* 
	 * On ARM we need to do explicit assembly since the compiler
	 * doesn't know the range of n is limited and decides to call
	 * a library function instead.
	 */
	__asm__ (
		"@ sllmul2n\n\t"
		"mov	%R0, %R1, lsl %2\n\t"
		"orr	%R0, %R0, %Q1, lsr %3\n\t"
		"mov	%Q0, %Q1, lsl %2\n\t"
		: "=r" (y)
		: "r" (x), "rM" (n), "rM" (32 - n)
	);
#else
	y = x << n;
#endif

	return y;
}

static __inline__ sll slldiv2(sll x)
{
	return x >> 1;
}

static __inline__ sll slldiv4(sll x)
{
	return x >> 2;
}

static __inline__ sll slldiv2n(sll x, int n)
{
	sll y;

#if defined(__arm__)
	/* 
	 * On ARM we need to do explicit assembly since the compiler
	 * doesn't know the range of n is limited and decides to call
	 * a library function instead.
	 */
	__asm__ (
		"@ slldiv2n\n\t"
		"mov	%Q0, %Q1, lsr %2\n\t"
		"orr	%Q0, %Q0, %R1, lsl %3\n\t"
		"mov	%R0, %R1, asr %2\n\t"
		: "=r" (y)
		: "r" (x), "rM" (n), "rM" (32 - n)
	);
#else
	y = x >> n;
#endif

	return y;
}

/*
 * Unpack the IEEE floating point double format and put it in fixed point
 * sll format.
 */
static __inline__ sll dbl2sll(double dbl)
{
	union {
		double d;
		unsigned u[2];
		ull _ull;
		sll _sll;
	} in, retval;
	register unsigned exp;

	/* Move into memory as args might be passed in regs */
	in.d = dbl;

#if defined(__arm__)

	/* ARM architecture has a big-endian double */
	exp = in.u[0];
	in.u[0] = in.u[1];
	in.u[1] = exp;

#endif /* defined(__arm__) */

	/* Leading 1 is assumed by IEEE */
	retval.u[1] = 0x40000000;

	/* Unpack the mantissa into the unsigned long */
	retval.u[1] |= (in.u[1] << 10) & 0x3ffffc00;
	retval.u[1] |= (in.u[0] >> 22) & 0x000003ff;
	retval.u[0] = in.u[0] << 10;

	/* Extract the exponent and align the decimals */
	exp = (in.u[1] >> 20) & 0x7ff;
	if (exp)
		retval._ull >>= 1053 - exp;
	else
		return 0L;

	/* Negate if negative flag set */
	if (in.u[1] & 0x80000000)
		retval._sll = -retval._sll;

	return retval._sll;
}

static __inline__ sll float2sll(float f)
{
	return dbl2sll((double)f);
}

static __inline__ float sll2float(sll s)
{
	return ((float)sll2dbl(s));
}

/*
 * Calculate cos x where -pi/4 <= x <= pi/4
 *
 * Description:
 *	cos x = 1 - x^2 / 2! + x^4 / 4! - ... + x^(2N) / (2N)!
 *	Note that (pi/4)^12 / 12! < 2^-32 which is the smallest possible number.
 */
static __inline__ sll _sllcos(sll x)
{
	sll retval, x2;
	x2 = sllmul(x, x);
	/*
	 * cos x = t0 + t1 + t2 + t3 + t4 + t5 + t6
	 *
	 * f0 =  0! =  1
	 * f1 =  2! =  2 *  1 * f0 =   2 * f0
	 * f2 =  4! =  4 *  3 * f1 =  12 x f1
	 * f3 =  6! =  6 *  5 * f2 =  30 * f2
	 * f4 =  8! =  8 *  7 * f3 =  56 * f3
	 * f5 = 10! = 10 *  9 * f4 =  90 * f4
	 * f6 = 12! = 12 * 11 * f5 = 132 * f5
	 *
	 * t0 = 1
	 * t1 = -t0 * x2 /   2 = -t0 * x2 * SLL_CONST_1_2
	 * t2 = -t1 * x2 /  12 = -t1 * x2 * SLL_CONST_1_12
	 * t3 = -t2 * x2 /  30 = -t2 * x2 * SLL_CONST_1_30
	 * t4 = -t3 * x2 /  56 = -t3 * x2 * SLL_CONST_1_56
	 * t5 = -t4 * x2 /  90 = -t4 * x2 * SLL_CONST_1_90
	 * t6 = -t5 * x2 / 132 = -t5 * x2 * SLL_CONST_1_132
	 */
	retval = _sllsub(SLL_CONST_1, sllmul(x2, SLL_CONST_1_132));
	retval = _sllsub(SLL_CONST_1, sllmul(sllmul(x2, retval), SLL_CONST_1_90));
	retval = _sllsub(SLL_CONST_1, sllmul(sllmul(x2, retval), SLL_CONST_1_56));
	retval = _sllsub(SLL_CONST_1, sllmul(sllmul(x2, retval), SLL_CONST_1_30));
	retval = _sllsub(SLL_CONST_1, sllmul(sllmul(x2, retval), SLL_CONST_1_12));
	retval = _sllsub(SLL_CONST_1, slldiv2(sllmul(x2, retval)));
	return retval;
}

/*
 * Calculate sin x where -pi/4 <= x <= pi/4
 *
 * Description:
 *	sin x = x - x^3 / 3! + x^5 / 5! - ... + x^(2N+1) / (2N+1)!
 *	Note that (pi/4)^13 / 13! < 2^-32 which is the smallest possible number.
 */
static __inline__ sll _sllsin(sll x)
{
	sll retval, x2;
	x2 = sllmul(x, x);
	/*
	 * sin x = t0 + t1 + t2 + t3 + t4 + t5 + t6
	 *
	 * f0 =  0! =  1
	 * f1 =  3! =  3 *  2 * f0 =   6 * f0
	 * f2 =  5! =  5 *  4 * f1 =  20 x f1
	 * f3 =  7! =  7 *  6 * f2 =  42 * f2
	 * f4 =  9! =  9 *  8 * f3 =  72 * f3
	 * f5 = 11! = 11 * 10 * f4 = 110 * f4
	 * f6 = 13! = 13 * 12 * f5 = 156 * f5
	 *
	 * t0 = 1
	 * t1 = -t0 * x2 /   6 = -t0 * x2 * SLL_CONST_1_6
	 * t2 = -t1 * x2 /  20 = -t1 * x2 * SLL_CONST_1_20
	 * t3 = -t2 * x2 /  42 = -t2 * x2 * SLL_CONST_1_42
	 * t4 = -t3 * x2 /  72 = -t3 * x2 * SLL_CONST_1_72
	 * t5 = -t4 * x2 / 110 = -t4 * x2 * SLL_CONST_1_110
	 * t6 = -t5 * x2 / 156 = -t5 * x2 * SLL_CONST_1_156
	 */
	retval = _sllsub(x, sllmul(x2, SLL_CONST_1_156));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), SLL_CONST_1_110));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), SLL_CONST_1_72));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), SLL_CONST_1_42));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), SLL_CONST_1_20));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), SLL_CONST_1_6));
	return retval;
}

static __inline__ sll sllcos(sll x)
{
	int i;
	sll retval;

	/* Calculate cos (x - i * pi/2), where -pi/4 <= x - i * pi/2 <= pi/4  */
	i = sll2int(_slladd(sllmul(x, SLL_CONST_2_PI), SLL_CONST_1_2));
	x = _sllsub(x, sllmul(int2sll(i), SLL_CONST_PI_2));

	switch (i & 3) {
		default:
		case 0:
			retval = _sllcos(x);
			break;
		case 1:
			retval = sllneg(_sllsin(x));
			break;
		case 2:
			retval = sllneg(_sllcos(x));
			break;
		case 3:
			retval = _sllsin(x);
			break;
	}
	return retval;
}

static __inline__ sll sllsin(sll x)
{
	int i;
	sll retval;

	/* Calculate sin (x - n * pi/2), where -pi/4 <= x - i * pi/2 <= pi/4 */
	i = sll2int(_slladd(sllmul(x, SLL_CONST_2_PI), SLL_CONST_1_2));
	x = _sllsub(x, sllmul(int2sll(i), SLL_CONST_PI_2));

	switch (i & 3) {
		default:
		case 0:
			retval = _sllsin(x);
			break;
		case 1:
			retval = _sllcos(x);
			break;
		case 2:
			retval = sllneg(_sllsin(x));
			break;
		case 3:
			retval = sllneg(_sllcos(x));
			break;
	}
	return retval;
}

static __inline__ sll slltan(sll x)
{
	int i;
	sll retval;

	i = sll2int(_slladd(sllmul(x, SLL_CONST_2_PI), SLL_CONST_1_2));
	x = _sllsub(x, sllmul(int2sll(i), SLL_CONST_PI_2));
	switch (i & 3) {
		default:
		case 0:
		case 2:
			retval = slldiv(_sllsin(x), _sllcos(x));
			break;
		case 1:
		case 3:
			retval = sllneg(slldiv(_sllcos(x), _sllsin(x)));
			break;
	}
	return retval;
}

/*
 * atan x = SUM[n=0,) (-1)^n * x^(2n + 1)/(2n + 1), |x| < 1
 *
 * Two term approximation
 *	a = x - x^3/3
 * Gives us
 *	atan x = a + ??
 * Let ?? = arctan ?
 *	atan x = a + arctan ?
 * Rearrange
 *	atan x - a = arctan ?
 * Apply tan to both sides
 *	tan (atan x - a) = tan arctan ?
 *	tan (atan x - a) = ?
 * Applying the standard formula
 *	tan (u - v) = (tan u - tan v) / (1 + tan u * tan v)
 * Gives us
 *	tan (atan x - a) = (tan atan x - tan a) / (1 + tan arctan x * tan a)
 * Let t = tan a
 *	tan (atan x - a) = (x - t) / (1 + x * t)
 * So finally
 *	arctan x = a + arctan ((tan x - t) / (1 + x * t))
 * And the typical worst case is x = 1.0 which converges in 3 iterations.
 */
static __inline__ sll _sllatan(sll x)
{
	sll a, t, retval;

	/* First iteration */
	a = sllmul(x, _sllsub(SLL_CONST_1, sllmul(x, sllmul(x, SLL_CONST_1_3))));
	retval = a;

	/* Second iteration */
	t = slldiv(_sllsin(a), _sllcos(a));
	x = slldiv(_sllsub(x, t), _slladd(SLL_CONST_1, sllmul(t, x)));
	a = sllmul(x, _sllsub(SLL_CONST_1, sllmul(x, sllmul(x, SLL_CONST_1_3))));
	retval = _slladd(retval, a);

	/* Third  iteration */
	t = slldiv(_sllsin(a), _sllcos(a));
	x = slldiv(_sllsub(x, t), _slladd(SLL_CONST_1, sllmul(t, x)));
	a = sllmul(x, _sllsub(SLL_CONST_1, sllmul(x, sllmul(x, SLL_CONST_1_3))));
	return _slladd(retval, a);
}

static __inline__ sll sllatan(sll x)
{
	sll retval;

	if (x < -sllneg(SLL_CONST_1))
		retval = sllneg(SLL_CONST_PI_2);
	else if (x > SLL_CONST_1)
		retval = SLL_CONST_PI_2;
	else
		return _sllatan(x);
	return _sllsub(retval, _sllatan(sllinv(x)));
}

/*
 * Calculate e^x where -0.5 <= x <= 0.5
 *
 * Description:
 *	e^x = x^0 / 0! + x^1 / 1! + ... + x^N / N!
 *	Note that 0.5^11 / 11! < 2^-32 which is the smallest possible number.
 */
static __inline__ sll _sllexp(sll x)
{
	sll retval;
	retval = _slladd(SLL_CONST_1, sllmul(0, sllmul(x, SLL_CONST_1_11)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_11)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_10)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_9)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, slldiv2n(x, 3)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_7)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_6)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_5)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, slldiv4(x)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, sllmul(x, SLL_CONST_1_3)));
	retval = _slladd(SLL_CONST_1, sllmul(retval, slldiv2(x)));
	return retval;
}

/*
 * Calculate e^x where x is arbitrary
 */
static __inline__ sll sllexp(sll x)
{
	int i;
	sll e, retval;

	e = SLL_CONST_E;

	/* -0.5 <= x <= 0.5  */
	i = sll2int(_slladd(x, SLL_CONST_1_2));
	retval = _sllexp(_sllsub(x, int2sll(i)));

	/* i >= 0 */
	if (i < 0) {
		i = -i;
		e = SLL_CONST_1_E;
	}

	/* Scale the result */
	for (;i; i >>= 1) {
		if (i & 1)
			retval = sllmul(retval, e);
		e = sllmul(e, e);
	}
	return retval;
}

/*
 * Calculate natural logarithm using Netwton-Raphson method
 */
static __inline__ sll slllog(sll x)
{
	sll x1, ln = 0;

	/* Scale: e^(-1/2) <= x <= e^(1/2) */
	while (x < SLL_CONST_1_SQRTE) {
		ln = _sllsub(ln, SLL_CONST_1);
		x = sllmul(x, SLL_CONST_E);
	}
	while (x > SLL_CONST_SQRTE) {
		ln = _slladd(ln, SLL_CONST_1);
		x = sllmul(x, SLL_CONST_1_E);
	}

	/* First iteration */
	x1 = sllmul(_sllsub(x, SLL_CONST_1), slldiv2(_sllsub(x, SLL_CONST_3)));
	ln = _sllsub(ln, x1);
	x = sllmul(x, _sllexp(x1));

	/* Second iteration */
	x1 = sllmul(_sllsub(x, SLL_CONST_1), slldiv2(_sllsub(x, SLL_CONST_3)));
	ln = _sllsub(ln, x1);
	x = sllmul(x, _sllexp(x1));

	/* Third iteration */
	x1 = sllmul(_sllsub(x, SLL_CONST_1), slldiv2(_sllsub(x, SLL_CONST_3)));
	ln = _sllsub(ln, x1);

	return ln;
}

/*
 * ln x^y = y * log x
 * e^(ln x^y) = e^(y * log x)
 * x^y = e^(y * ln x)
 */
static __inline__ sll sllpow(sll x, sll y)
{
	if (y == SLL_CONST_0)
		return SLL_CONST_1;
	return sllexp(sllmul(y, slllog(x)));
}

/*
 * Consider a parabola centered on the y-axis
 * 	y = a * x^2 + b
 * Has zeros (y = 0)  at
 *	a * x^2 + b = 0
 *	a * x^2 = -b
 *	x^2 = -b / a
 *	x = +- (-b / a)^(1 / 2)
 * Letting a = 1 and b = -X
 *	y = x^2 - X
 *	x = +- X^(1 / 2)
 * Which is convenient since we want to find the square root of X, and we can
 * use Newton's Method to find the zeros of any f(x)
 *	xn = x - f(x) / f'(x)
 * Applied Newton's Method to our parabola
 *	f(x) = x^2 - X
 *	xn = x - (x^2 - X) / (2 * x)
 *	xn = x - (x - X / x) / 2
 * To make this converge quickly, we scale X so that
 *	X = 4^N * z
 * Taking the roots of both sides
 *	X^(1 / 2) = (4^n * z)^(1 / 2)
 *	X^(1 / 2) = 2^n * z^(1 / 2)
 * Let N = 2^n
 *	x^(1 / 2) = N * z^(1 / 2)
 * We want this to converge to the positive root, so we must start at a point
 *	0 < start <= x^(1 / 2)
 * or
 *	x^(1/2) <= start <= infinity
 * since
 *	(1/2)^(1/2) = 0.707
 *	2^(1/2) = 1.414
 * A good choice is 1 which lies in the middle, and takes 4 iterations to
 * converge from either extreme.
 */
static __inline__ sll sllsqrt(sll x)
{
	sll n, xn;
       
	/* Start with a scaling factor of 1 */
	n = SLL_CONST_1;

	/* Quick solutions for the simple cases */
	if (x <= SLL_CONST_0 || x == SLL_CONST_1)
		return x;

	/* Scale x so that 0.5 <= x < 2 */
	while (x >= SLL_CONST_2) {
		x = slldiv4(x);
		n = sllmul2(n);
	}
	while (x < SLL_CONST_1_2) {
		x = sllmul4(x);
		n = slldiv2(n);
	}

	/* Simple solution if x = 4^n */
	if (x == SLL_CONST_1)
		return n;

	/* The starting point */
	xn = SLL_CONST_1;

	/* Four iterations will be enough */
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));

	/* Scale the result */
	return sllmul(n, xn);
}


#endif

static inline sll sllatan2(sll x, sll y)
{
	register sll ret;
	register sll d=slldiv(y,x); 

	if (d <= SLL_CONST_1 && d >= sllneg(SLL_CONST_1))
		ret=sllmul(SLL_CONST_ATAN_1, d);
	else
		ret=sllatan(d);

	if (x > SLL_CONST_0)
    		ret= slladd(ret, SLL_CONST_PI);

	return ret;
}


#ifdef __cplusplus
}
#endif

#endif /* MATHSLL_H */
