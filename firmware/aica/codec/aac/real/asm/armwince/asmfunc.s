; ***** BEGIN LICENSE BLOCK *****  
; Source last modified: $Id: asmfunc.s,v 1.1 2005/02/26 01:47:36 jrecker Exp $ 
;   
; Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.  
;       
; The contents of this file, and the files included with this file, 
; are subject to the current version of the RealNetworks Public 
; Source License (the "RPSL") available at 
; http://www.helixcommunity.org/content/rpsl unless you have licensed 
; the file under the current version of the RealNetworks Community 
; Source License (the "RCSL") available at 
; http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
; will apply. You may also obtain the license terms directly from 
; RealNetworks.  You may not use this file except in compliance with 
; the RPSL or, if you have a valid RCSL with RealNetworks applicable 
; to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
; the rights, obligations and limitations governing use of the 
; contents of the file. 
;   
; This file is part of the Helix DNA Technology. RealNetworks is the 
; developer and/or licensor of the Original Code and owns the  
; copyrights in the portions it created. 
;   
; This file, and the files included with this file, is distributed 
; and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
; KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
; ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
; ENJOYMENT OR NON-INFRINGEMENT. 
;  
; Technology Compatibility Kit Test Suite(s) Location:  
;    http://www.helixcommunity.org/content/tck  
;  
; Contributor(s):  
;   
; ***** END LICENSE BLOCK *****  

;/* assembly.s
; * 
; * the assembly routines are written in this separate .s file because EVC++ won't inline...
; *
; * Notes on APCS (ARM procedure call standard):
; *  - first four arguments are in r0-r3
; *  - additional argumets are pushed onto stack in reverse order
; *  - return value goes in r0
; *  - routines can overwrite r0-r3, r12 without saving
; *  - other registers must be preserved with stm/ldm commands
; * 
; * Equivalent register names:
; *
; * r0  r1  r2  r3  r4  r5  r6  r7  r8  r9  r10 r11 r12 r13 r14 r15
; * ---------------------------------------------------------------
; * a1  a2  a3  a4  v1  v2  v3  v4  v5  v6  sl  fp  ip  sp  lr  pc
; */

	AREA	|.text|, CODE, READONLY

; int MULSHIFT32(int x, int y);

	EXPORT	raac_MULSHIFT32
	
raac_MULSHIFT32

	smull	r2, r0, r1, r0
	mov		pc, lr

; __int64 MADD64(unsigned int sumLo, int sumHi, int a, int b);

	EXPORT raac_MADD64

raac_MADD64

   	smlal	r0, r1, r2, r3 
	mov		pc, lr

	END
