/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Sum Of Absolute Difference header  -
 *
 *  Copyright(C) 2001-2010 Peter Ross <pross@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: sad.h 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

#ifndef _ENCODER_SAD_H_
#define _ENCODER_SAD_H_

#include "../portab.h"

typedef void (sadInitFunc) (void);
typedef sadInitFunc *sadInitFuncPtr;

extern sadInitFuncPtr sadInit;
sadInitFunc sadInit_altivec;

typedef uint32_t(sad16Func) (const uint8_t * const cur,
							 const uint8_t * const ref,
							 const uint32_t stride,
							 const uint32_t best_sad);
typedef sad16Func *sad16FuncPtr;
extern sad16FuncPtr sad16;
sad16Func sad16_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sad16Func sad16_mmx;
sad16Func sad16_xmm;
sad16Func sad16_3dne;
sad16Func sad16_sse2;
sad16Func sad16_sse3;
#endif

#ifdef ARCH_IS_IA64
sad16Func sad16_ia64;
#endif

#ifdef ARCH_IS_PPC
sad16Func sad16_altivec_c;
#endif

sad16Func mrsad16_c;

typedef uint32_t(sad8Func) (const uint8_t * const cur,
							const uint8_t * const ref,
							const uint32_t stride);
typedef sad8Func *sad8FuncPtr;
extern sad8FuncPtr sad8;
sad8Func sad8_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sad8Func sad8_mmx;
sad8Func sad8_xmm;
sad8Func sad8_3dne;
#endif

#ifdef ARCH_IS_IA64
sad8Func sad8_ia64;
#endif

#ifdef ARCH_IS_PPC
sad8Func sad8_altivec_c;
#endif

typedef uint32_t(sad16biFunc) (const uint8_t * const cur,
							   const uint8_t * const ref1,
							   const uint8_t * const ref2,
							   const uint32_t stride);
typedef sad16biFunc *sad16biFuncPtr;
extern sad16biFuncPtr sad16bi;
sad16biFunc sad16bi_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sad16biFunc sad16bi_mmx;
sad16biFunc sad16bi_xmm;
sad16biFunc sad16bi_3dne;
sad16biFunc sad16bi_3dn;
#endif

#ifdef ARCH_IS_IA64
sad16biFunc sad16bi_ia64;
#endif

#ifdef ARCH_IS_PPC
sad16biFunc sad16bi_altivec_c;
#endif

typedef uint32_t(sad8biFunc) (const uint8_t * const cur,
							   const uint8_t * const ref1,
							   const uint8_t * const ref2,
							   const uint32_t stride);
typedef sad8biFunc *sad8biFuncPtr;
extern sad8biFuncPtr sad8bi;
sad8biFunc sad8bi_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sad8biFunc sad8bi_mmx;
sad8biFunc sad8bi_xmm;
sad8biFunc sad8bi_3dne;
sad8biFunc sad8bi_3dn;
#endif

typedef uint32_t(dev16Func) (const uint8_t * const cur,
							 const uint32_t stride);
typedef dev16Func *dev16FuncPtr;
extern dev16FuncPtr dev16;
dev16Func dev16_c;

typedef uint32_t (sad16vFunc)(	const uint8_t * const cur,
								const uint8_t * const ref,
								const uint32_t stride, int32_t *sad8);
typedef sad16vFunc *sad16vFuncPtr;
extern sad16vFuncPtr sad16v;

sad16vFunc sad16v_c;
sad16vFunc sad32v_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
dev16Func dev16_mmx;
dev16Func dev16_xmm;
dev16Func dev16_3dne;
dev16Func dev16_sse2;
dev16Func dev16_sse3;
sad16vFunc sad16v_xmm;
sad16vFunc sad16v_mmx;
#endif

#ifdef ARCH_IS_IA64
dev16Func dev16_ia64;
#endif

#ifdef ARCH_IS_PPC
dev16Func dev16_altivec_c;
#endif

/* This function assumes blocks use 16bit signed elements */
typedef uint32_t (sse8Func_16bit)(const int16_t * cur,
								  const int16_t * ref,
								  const uint32_t stride);
typedef sse8Func_16bit *sse8Func_16bitPtr;
extern sse8Func_16bitPtr sse8_16bit;

sse8Func_16bit sse8_16bit_c;
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sse8Func_16bit sse8_16bit_mmx;
#endif

#ifdef ARCH_IS_PPC
sse8Func_16bit sse8_16bit_altivec_c;
#endif

/* This function assumes blocks use 8bit *un*signed elements */
typedef uint32_t (sse8Func_8bit)(const uint8_t * cur,
								 const uint8_t * ref,
								 const uint32_t stride);
typedef sse8Func_8bit *sse8Func_8bitPtr;
extern sse8Func_8bitPtr sse8_8bit;

sse8Func_8bit sse8_8bit_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sse8Func_8bit sse8_8bit_mmx;
#endif

typedef uint32_t (sseh8Func_16bit)(const int16_t * cur,
								   const int16_t * ref,
								   uint16_t mask);
typedef sseh8Func_16bit *sseh8Func_16bitPtr;
extern sseh8Func_16bitPtr sseh8_16bit;

sseh8Func_16bit sseh8_16bit_c;
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
sseh8Func_16bit sseh8_16bit_sse2;
#endif

typedef uint32_t (coeff8_energyFunc)(const int16_t * cur);
typedef coeff8_energyFunc *coeff8_energyFunc_Ptr;
extern coeff8_energyFunc_Ptr coeff8_energy;

coeff8_energyFunc coeff8_energy_c;
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
coeff8_energyFunc coeff8_energy_sse2;
#endif

typedef uint32_t (blocksum8Func)(const uint8_t * cur, int stride, 
								 uint16_t sums[4], uint32_t squares[4]);
typedef blocksum8Func *blocksum8Func_Ptr;
extern blocksum8Func_Ptr blocksum8;

blocksum8Func blocksum8_c;
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
blocksum8Func blocksum8_sse2;
#endif

/* Coeffs for MSE_H calculation */

static const int16_t Inv_iMask_Coeff[64] = {    
       0,   155,   128,   328,   737,  2048,  3329,  4763,
     184,   184,   251,   462,   865,  4306,  4608,  3872,
     251,   216,   328,   737,  2048,  4159,  6094,  4014,
     251,   370,   620,  1076,  3329,  9688,  8192,  4920,
     415,   620,  1752,  4014,  5919, 15207, 13579,  7589,
     737,  1568,  3872,  5243,  8398, 13844, 16345, 10834,
    3073,  5243,  7787,  9688, 13579, 18741, 18433, 13057,
    6636, 10834, 11552, 12294, 16056, 12800, 13579, 12545
};

static const uint16_t iCSF_Coeff[64] = { 
	26353, 38331, 42164, 26353, 17568, 10541, 8268, 6912,
	35137, 35137, 30117, 22192, 16217,  7270, 7027, 7666,
	30117, 32434, 26353, 17568, 10541,  7397, 6111, 7529,
	30117, 24803, 19166, 14539,  8268,  4846, 5271, 6801,
	23425, 19166, 11396,  7529,  6201,  3868, 4094, 5476,
	17568, 12047,  7666,  6588,  5205,  4054, 3731, 4583,
	 8605,  6588,  5406,  4846,  4094,  3485, 3514, 4175,
	 5856,  4583,  4438,  4302,  3765,  4216, 4094, 4259
};

static const uint16_t iCSF_Round[64] = {  
	1, 1, 1, 1, 2, 3, 4, 5,
	1, 1, 1, 1, 2, 5, 5, 4,
	1, 1, 1, 2, 3, 4, 5, 4,
	1, 1, 2, 2, 4, 7, 6, 5,
	1, 2, 3, 4, 5, 8, 8, 6,
	2, 3, 4, 5, 6, 8, 9, 7,
	4, 5, 6, 7, 8, 9, 9, 8,
	6, 7, 7, 8, 9, 8, 8, 8
};

#endif							/* _ENCODER_SAD_H_ */
