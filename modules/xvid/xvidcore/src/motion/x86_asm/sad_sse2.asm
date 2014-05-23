;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - SSE2 optimized SAD operators -
; *
; *  Copyright(C) 2003-2010 Pascal Massimino <skal@planet-d.net>
; *               2008-2010 Michael Militzer <michael@xvid.org>
; *
; *
; *  This program is free software; you can redistribute it and/or modify it
; *  under the terms of the GNU General Public License as published by
; *  the Free Software Foundation; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: sad_sse2.asm,v 1.21 2010-11-28 15:18:21 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
zero    times 4   dd 0

ALIGN SECTION_ALIGN
ones    times 8   dw 1

ALIGN SECTION_ALIGN
round32 times 4   dd 32

;=============================================================================
; Coeffs for MSE_H calculation
;=============================================================================

ALIGN SECTION_ALIGN
iMask_Coeff:
  dw     0, 29788, 32767, 20479, 13653, 8192, 6425, 5372,
  dw 27306, 27306, 23405, 17246, 12603, 5650, 5461, 5958,
  dw 23405, 25205, 20479, 13653,  8192, 5749, 4749, 5851,
  dw 23405, 19275, 14894, 11299,  6425, 3766, 4096, 5285,
  dw 18204, 14894,  8856,  5851,  4819, 3006, 3181, 4255,
  dw 13653,  9362,  5958,  5120,  4045, 3151, 2900, 3562,
  dw  6687,  5120,  4201,  3766,  3181, 2708, 2730, 3244,
  dw  4551,  3562,  3449,  3344,  2926, 3277, 3181, 3310

ALIGN SECTION_ALIGN
Inv_iMask_Coeff:
  dd    0,   155,   128,   328,   737,  2048,  3329,  4763,
  dd  184,   184,   251,   462,   865,  4306,  4608,  3872,
  dd  251,   216,   328,   737,  2048,  4159,  6094,  4014,
  dd  251,   370,   620,  1076,  3329,  9688,  8192,  4920,
  dd  415,   620,  1752,  4014,  5919, 15207, 13579,  7589,
  dd  737,  1568,  3872,  5243,  8398, 13844, 16345, 10834,
  dd 3073,  5243,  7787,  9688, 13579, 18741, 18433, 13057,
  dd 6636, 10834, 11552, 12294, 16056, 12800, 13579, 12545

ALIGN SECTION_ALIGN
iCSF_Coeff:
  dw 26353, 38331, 42164, 26353, 17568, 10541, 8268, 6912,
  dw 35137, 35137, 30117, 22192, 16217,  7270, 7027, 7666,
  dw 30117, 32434, 26353, 17568, 10541,  7397, 6111, 7529,
  dw 30117, 24803, 19166, 14539,  8268,  4846, 5271, 6801,
  dw 23425, 19166, 11396,  7529,  6201,  3868, 4094, 5476,
  dw 17568, 12047,  7666,  6588,  5205,  4054, 3731, 4583,
  dw  8605,  6588,  5406,  4846,  4094,  3485, 3514, 4175,
  dw  5856,  4583,  4438,  4302,  3765,  4216, 4094, 4259

ALIGN SECTION_ALIGN
iCSF_Round:
  dw 1, 1, 1, 1, 2, 3, 4, 5,
  dw 1, 1, 1, 1, 2, 5, 5, 4,
  dw 1, 1, 1, 2, 3, 4, 5, 4,
  dw 1, 1, 2, 2, 4, 7, 6, 5,
  dw 1, 2, 3, 4, 5, 8, 8, 6,
  dw 2, 3, 4, 5, 6, 8, 9, 7,
  dw 4, 5, 6, 7, 8, 9, 9, 8,
  dw 6, 7, 7, 8, 9, 8, 8, 8


;=============================================================================
; Code
;=============================================================================

TEXT

cglobal  sad16_sse2
cglobal  dev16_sse2

cglobal  sad16_sse3
cglobal  dev16_sse3

cglobal  sseh8_16bit_sse2
cglobal  coeff8_energy_sse2
cglobal  blocksum8_sse2

;-----------------------------------------------------------------------------
; uint32_t sad16_sse2 (const uint8_t * const cur, <- assumed aligned!
;                      const uint8_t * const ref,
;	                   const uint32_t stride,
;                      const uint32_t /*ignored*/);
;-----------------------------------------------------------------------------


%macro SAD_16x16_SSE2 1
  %1  xmm0, [TMP1]
  %1  xmm1, [TMP1+TMP0]
  lea TMP1,[TMP1+2*TMP0]
  movdqa  xmm2, [_EAX]
  movdqa  xmm3, [_EAX+TMP0]
  lea _EAX,[_EAX+2*TMP0]
  psadbw  xmm0, xmm2
  paddusw xmm4,xmm0
  psadbw  xmm1, xmm3
  paddusw xmm4,xmm1
%endmacro

%macro SAD16_SSE2_SSE3 1
  mov _EAX, prm1 ; cur (assumed aligned)
  mov TMP1, prm2 ; ref
  mov TMP0, prm3 ; stride

  pxor xmm4, xmm4 ; accum

  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1

  pshufd  xmm5, xmm4, 00000010b
  paddusw xmm4, xmm5
  pextrw  eax, xmm4, 0

  ret
%endmacro

ALIGN SECTION_ALIGN
sad16_sse2:
  SAD16_SSE2_SSE3 movdqu
ENDFUNC


ALIGN SECTION_ALIGN
sad16_sse3:
  SAD16_SSE2_SSE3 lddqu
ENDFUNC


;-----------------------------------------------------------------------------
; uint32_t dev16_sse2(const uint8_t * const cur, const uint32_t stride);
;-----------------------------------------------------------------------------

%macro MEAN_16x16_SSE2 1  ; _EAX: src, TMP0:stride, mm7: zero or mean => mm6: result
  %1 xmm0, [_EAX]
  %1 xmm1, [_EAX+TMP0]
  lea _EAX, [_EAX+2*TMP0]    ; + 2*stride
  psadbw xmm0, xmm5
  paddusw xmm4, xmm0
  psadbw xmm1, xmm5
  paddusw xmm4, xmm1
%endmacro


%macro MEAN16_SSE2_SSE3 1
  mov _EAX, prm1   ; src
  mov TMP0, prm2   ; stride

  pxor xmm4, xmm4     ; accum
  pxor xmm5, xmm5     ; zero

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  mov _EAX, prm1       ; src again

  pshufd   xmm5, xmm4, 10b
  paddusw  xmm5, xmm4
  pxor     xmm4, xmm4     ; zero accum
  psrlw    xmm5, 8        ; => Mean
  pshuflw  xmm5, xmm5, 0  ; replicate Mean
  packuswb xmm5, xmm5
  pshufd   xmm5, xmm5, 00000000b

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  pshufd   xmm5, xmm4, 10b
  paddusw  xmm5, xmm4
  pextrw eax, xmm5, 0

  ret
%endmacro

ALIGN SECTION_ALIGN
dev16_sse2:
  MEAN16_SSE2_SSE3 movdqu
ENDFUNC

ALIGN SECTION_ALIGN
dev16_sse3:
  MEAN16_SSE2_SSE3 lddqu
ENDFUNC

;-----------------------------------------------------------------------------
; uint32_t coeff8_energy_sse2(const int16_t * dct);
;-----------------------------------------------------------------------------

%macro DCT_ENERGY_SSE2 4

  movdqa  %1, [%3 + %4]
  movdqa  %2, [%3 + %4 + 16]

  psllw %1, 4
  psllw %2, 4

  pmulhw  %1, [iMask_Coeff + %4]
  pmulhw  %2, [iMask_Coeff + %4 + 16]

  pmaddwd %1, %1
  pmaddwd %2, %2

  paddd   %1, %2
  psrld   %1, 3

%endmacro

ALIGN SECTION_ALIGN
coeff8_energy_sse2:

  mov TMP0, prm1  ; DCT_A

  DCT_ENERGY_SSE2 xmm0, xmm1, TMP0,  0
  DCT_ENERGY_SSE2 xmm1, xmm2, TMP0, 32
  
  DCT_ENERGY_SSE2 xmm2, xmm3, TMP0, 64
  DCT_ENERGY_SSE2 xmm3, xmm4, TMP0, 96
  
  paddd xmm0, xmm1
  paddd xmm2, xmm3
  
  paddd xmm0, xmm2 ; A B C D

  ; convolute
  pshufd xmm1, xmm0, 238 
  paddd xmm0, xmm1
  
  pshufd xmm2, xmm0, 85 
  paddd xmm0, xmm2
  
  movd eax, xmm0

  ret
ENDFUNC

;-----------------------------------------------------------------------------------
; uint32_t mseh8_16bit_sse2(const int16_t * cur, const int16_t * ref, uint16_t mask)
;-----------------------------------------------------------------------------------

%macro SSEH_SSE2 4
  movdqa xmm0, [%1 + %3]
  movdqa xmm1, [%2 + %3]
  
  movdqa xmm2, [%1 + %3 + 16]
  movdqa xmm3, [%2 + %3 + 16]
  
  
  movdqa xmm4, xmm7 ; MASK
  movdqa xmm5, xmm7

  psubsw xmm0, xmm1 ; A - B
  psubsw xmm2, xmm3 


  ; ABS
  pxor xmm1, xmm1
  pxor xmm3, xmm3

  pcmpgtw xmm1, xmm0
  pcmpgtw xmm3, xmm2
  
  pxor xmm0, xmm1     ; change sign if negative
  pxor xmm2, xmm3     ;

  psubw xmm0, xmm1    ; ABS (A - B)
  psubw xmm2, xmm3    ; ABS (A - B)


  movdqa xmm1, xmm7 ; MASK
  movdqa xmm3, xmm7
  
  pmaddwd xmm4, [Inv_iMask_Coeff + 2*(%3)]
  pmaddwd xmm5, [Inv_iMask_Coeff + 2*(%3) + 16]

  pmaddwd xmm1, [Inv_iMask_Coeff + 2*(%3) + 32]
  pmaddwd xmm3, [Inv_iMask_Coeff + 2*(%3) + 48]

  psllw xmm0, 4
  psllw xmm2, 4
  
  paddd xmm4, [round32]
  paddd xmm5, [round32]

  paddd xmm1, [round32]
  paddd xmm3, [round32]

  psrad xmm4, 7
  psrad xmm5, 7

  psrad xmm1, 7
  psrad xmm3, 7

  packssdw xmm4, xmm5 ; Thresh
  packssdw xmm1, xmm3 ; Thresh

  
  psubusw xmm0, xmm4 ; Decimate by masking effect
  psubusw xmm2, xmm1
    
  paddusw xmm0, [iCSF_Round + %3]
  paddusw xmm2, [iCSF_Round + %3 + 16]
    
  pmulhuw xmm0, [iCSF_Coeff + %3]
  pmulhuw xmm2, [iCSF_Coeff + %3 + 16]
    
  pmaddwd xmm0, xmm0
  pmaddwd xmm2, xmm2

  paddd xmm0, xmm2
%endmacro


ALIGN SECTION_ALIGN
sseh8_16bit_sse2:

  PUSH_XMM6_XMM7

  mov TMP0, prm1  ; DCT_A
  mov TMP1, prm2  ; DCT_B
  mov _EAX, prm3  ; MASK
  
  movd xmm7, eax
  pshufd xmm7, xmm7, 0
  
  SSEH_SSE2 TMP0, TMP1,   0, xmm7
  movdqa xmm6, xmm0
  SSEH_SSE2 TMP0, TMP1,  32, xmm7
  paddd xmm6, xmm0
  SSEH_SSE2 TMP0, TMP1,  64, xmm7
  paddd xmm6, xmm0
  SSEH_SSE2 TMP0, TMP1,  96, xmm7
  paddd xmm6, xmm0

  ; convolute
  pshufd xmm1, xmm6, 238 
  paddd xmm6, xmm1
  
  pshufd xmm2, xmm6, 85
  paddd xmm6, xmm2


  movd eax, xmm6
  
  POP_XMM6_XMM7
  ret
ENDFUNC

;--------------------------------------------------------------------------------------------
; uint32_t blocksum8_c(const int8_t * cur, int stride, uint16_t sums[4], uint32_t squares[4])
;--------------------------------------------------------------------------------------------

%macro BLOCKSUM_SSE2 3
  movq xmm0, [%1       ] ; 0 0 B A
  movq xmm2, [%1 +   %2] ; 0 0 B A
  movq xmm1, [%1 + 2*%2]
  movq xmm3, [%1 +   %3]

  punpckldq xmm0, xmm2 ; B B A A
  punpckldq xmm1, xmm3 ; B B A A

  movdqa xmm2, xmm0
  movdqa xmm3, xmm1
    
  psadbw xmm0, xmm7 ; 000b000a
  psadbw xmm1, xmm7

  movdqa xmm4, xmm2
  movdqa xmm5, xmm3
 
  punpcklbw xmm2, xmm7 ; aaaaaaaa
  punpcklbw xmm3, xmm7

  punpckhbw xmm4, xmm7 ; bbbbbbbb
  punpckhbw xmm5, xmm7
  
  pmaddwd xmm2, xmm2 ; a*a+a*a a*a+a*a a*a+a*a a*a+a*a
  pmaddwd xmm3, xmm3
  
  pmaddwd xmm4, xmm4 ; b*b+b*b b*b+b*b b*b+b*b b*b+b*b
  pmaddwd xmm5, xmm5

  paddd xmm2, xmm3
  paddd xmm4, xmm5
  
  movdqa xmm3, xmm2
  punpckldq xmm2, xmm4 ; BABA
  punpckhdq xmm3, xmm4 ; BABA
  
  paddd xmm2, xmm3
  
  lea %1, [%1 + 4*%2]

  movdqa xmm4, xmm2
  punpckhqdq xmm4, xmm7 ; 
  
  paddd xmm2, xmm4
  
  ;
  movq xmm3, [%1       ] ; 0 0 D C
  movq xmm5, [%1 +   %2] ; 0 0 D C
  movq xmm4, [%1 + 2*%2]
  movq xmm6, [%1 +   %3]

  punpckldq xmm3, xmm5 ; D D C C
  punpckldq xmm4, xmm6 ; D D C C

  movdqa xmm5, xmm3
  movdqa xmm6, xmm4
    
  psadbw xmm3, xmm7 ; 000d000c
  psadbw xmm4, xmm7
  
  packssdw xmm0, xmm3 ; 0d0c0b0a
  packssdw xmm1, xmm4 ;

  paddusw  xmm0, xmm1
  packssdw xmm0, xmm7 ; 0000dcba
  
  
  movdqa xmm3, xmm5
  movdqa xmm4, xmm6
 
  punpcklbw xmm3, xmm7
  punpcklbw xmm4, xmm7

  punpckhbw xmm5, xmm7
  punpckhbw xmm6, xmm7
  
  pmaddwd xmm3, xmm3 ; C*C+C*C 
  pmaddwd xmm4, xmm4
  
  pmaddwd xmm5, xmm5 ; D*D+D*D
  pmaddwd xmm6, xmm6

  paddd xmm3, xmm4
  paddd xmm5, xmm6
  
  movdqa xmm1, xmm3
  punpckldq xmm3, xmm5 ; DCDC
  punpckhdq xmm1, xmm5 ; DCDC

  paddd xmm3, xmm1
  
  movdqa xmm4, xmm3
  punpckhqdq xmm4, xmm7 ; 
  
  paddd xmm3, xmm4
  punpcklqdq xmm2, xmm3
%endmacro


ALIGN SECTION_ALIGN
blocksum8_sse2:

  PUSH_XMM6_XMM7

  mov TMP0, prm1  ; cur
  mov TMP1, prm2  ; stride
  mov _EAX, prm3  ; sums

  push _EBP
  lea _EBP, [TMP1 + 2*TMP1]
  
  pxor xmm7, xmm7

  BLOCKSUM_SSE2 TMP0, TMP1, _EBP

  pop _EBP
  mov TMP0, prm4  ; squares
  
  movq [_EAX], xmm0   ; sums of the 4x4 sub-blocks
  movdqa [TMP0], xmm2 ; squares of the 4x4 sub-blocks
  
  pmaddwd xmm0, [ones]
  packssdw xmm0, xmm7
  
  pmaddwd xmm0, [ones]
  movd eax, xmm0
  
  POP_XMM6_XMM7
  ret
ENDFUNC

NON_EXEC_STACK
