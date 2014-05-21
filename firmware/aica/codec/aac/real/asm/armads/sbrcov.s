; ***** BEGIN LICENSE BLOCK *****  
; Source last modified: $Id: sbrcov.s,v 1.1 2005/02/26 01:47:35 jrecker Exp $ 
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

	AREA	|.text|, CODE, READONLY

; commmon
XBUF		RN	r0
ACCBUF		RN  r1
NCT			RN	r2
X0RE		RN 	r3
X0IM		RN 	r4
X1RE		RN 	r5
X1IM		RN 	r6
X0IMNEG		RN	r14

; CVKernel1 - in loop
P01RELO		RN 	r7
P01REHI		RN	r8
P01IMLO		RN  r9
P01IMHI		RN 	r10
P11RELO		RN 	r11
P11REHI		RN	r12

; CVKernel1 - out of loop
P12RELO		RN 	r7
P12REHI		RN	r8
P12IMLO		RN  r9
P12IMHI		RN 	r10
P22RELO		RN 	r11
P22REHI		RN	r12

; void CVKernel1(int *XBuf, int *accBuf)
;   see comments in sbrhfgen.c

raac_CVKernel1		FUNCTION
	EXPORT	raac_CVKernel1
	stmfd	sp!, {r4-r11, r14}

	ldr		X0RE, [XBUF], #4*(1)
	ldr		X0IM, [XBUF], #4*(2*64-1)
	ldr		X1RE, [XBUF], #4*(1)
	ldr		X1IM, [XBUF], #4*(2*64-1)
	rsb		X0IMNEG, X0IM, #0

	smull	P12RELO, P12REHI, X1RE, X0RE
	smlal	P12RELO, P12REHI, X1IM, X0IM
	smull	P12IMLO, P12IMHI, X0RE, X1IM
	smlal	P12IMLO, P12IMHI, X0IMNEG, X1RE
	smull	P22RELO, P22REHI, X0RE, X0RE
	smlal	P22RELO, P22REHI, X0IM, X0IM

	add		NCT, ACCBUF, #(4*6)
	stmia	NCT, {P12RELO-P22REHI}

	mov		P01RELO, #0
	mov		P01REHI, #0
	mov		P01IMLO, #0
	mov		P01IMHI, #0
	mov		P11RELO, #0
	mov		P11REHI, #0
	
	mov		NCT, #(16*2 + 6)
CV1_Loop_Start
	mov		X0RE, X1RE
	ldr		X1RE, [XBUF], #4*(1)
	mov		X0IM, X1IM
	ldr		X1IM, [XBUF], #4*(2*64-1)
	rsb		X0IMNEG, X0IM, #0
	
	smlal	P01RELO, P01REHI, X1RE, X0RE
	smlal	P01RELO, P01REHI, X1IM, X0IM
	smlal	P01IMLO, P01IMHI, X0RE, X1IM
	smlal	P01IMLO, P01IMHI, X0IMNEG, X1RE
	smlal	P11RELO, P11REHI, X0RE, X0RE
	smlal	P11RELO, P11REHI, X0IM, X0IM

	subs	NCT, NCT, #1
	bne		CV1_Loop_Start

	stmia	ACCBUF, {P01RELO-P11REHI}

	ldr		XBUF, [ACCBUF, #4*(6)]	; load P12RELO into temp buf
	ldr		NCT,  [ACCBUF, #4*(7)]	; load P12REHI into temp buf
	rsb		X0RE, X0RE, #0
	adds	P12RELO, XBUF, P01RELO	; P12RELO and P01RELO are same reg
	adc		P12REHI, NCT,  P01REHI	; P12REHI and P01REHI are same reg
	smlal	P12RELO, P12REHI, X1RE, X0RE
	smlal	P12RELO, P12REHI, X1IM, X0IMNEG

	ldr		XBUF, [ACCBUF, #4*(8)]	; load P12IMLO into temp buf
	ldr		NCT,  [ACCBUF, #4*(9)]	; load P12IMHI into temp buf
	adds	P12IMLO, XBUF, P01IMLO	; P12IMLO and P01IMLO are same reg
	adc		P12IMHI, NCT,  P01IMHI	; P12IMHI and P01IMHI are same reg
	smlal	P12IMLO, P12IMHI, X0RE, X1IM
	smlal	P12IMLO, P12IMHI, X0IM, X1RE
	
	ldr		XBUF, [ACCBUF, #4*(10)]	; load P22RELO into temp buf
	ldr		NCT,  [ACCBUF, #4*(11)]	; load P22REHI into temp buf
	adds	P22RELO, XBUF, P11RELO	; P22RELO and P11RELO are same reg
	adc		P22REHI, NCT,  P11REHI	; P22REHI and P11REHI are same reg
	rsb		XBUF, X0RE, #0
	smlal	P22RELO, P22REHI, X0RE, XBUF
	rsb		NCT,  X0IM, #0
	smlal	P22RELO, P22REHI, X0IM, NCT
	
	add		ACCBUF, ACCBUF, #(4*6)
	stmia	ACCBUF, {P12RELO-P22REHI}

	ldmfd	sp!, {r4-r11, pc}
	ENDFUNC

; CVKernel2
P02RELO		RN 	r7
P02REHI		RN	r8
P02IMLO		RN  r9
P02IMHI		RN 	r10
X2RE		RN 	r11
X2IM		RN 	r12

; void CVKernel2(int *XBuf, int *accBuf)
;   see comments in sbrhfgen.c

raac_CVKernel2		FUNCTION
	EXPORT	raac_CVKernel2
	stmfd	sp!, {r4-r11, r14}
	
	mov		P02RELO, #0
	mov		P02REHI, #0
	mov		P02IMLO, #0
	mov		P02IMHI, #0
	
	ldr		X0RE, [XBUF], #4*(1)
	ldr		X0IM, [XBUF], #4*(2*64-1)
	ldr		X1RE, [XBUF], #4*(1)
	ldr		X1IM, [XBUF], #4*(2*64-1)

	mov		NCT, #(16*2 + 6)
CV2_Loop_Start
	ldr		X2RE, [XBUF], #4*(1)
	ldr		X2IM, [XBUF], #4*(2*64-1)
	rsb		X0IMNEG, X0IM, #0

	smlal	P02RELO, P02REHI, X2RE, X0RE
	smlal	P02RELO, P02REHI, X2IM, X0IM
	smlal	P02IMLO, P02IMHI, X0RE, X2IM
	smlal	P02IMLO, P02IMHI, X0IMNEG, X2RE
	
	mov		X0RE, X1RE
	mov		X0IM, X1IM
	mov		X1RE, X2RE
	mov		X1IM, X2IM

	subs	NCT, NCT, #1
	bne		CV2_Loop_Start

	stmia	ACCBUF, {P02RELO-P02IMHI}

	ldmfd	sp!, {r4-r11, pc}
	ENDFUNC

	END
