@ ***** BEGIN LICENSE BLOCK *****  
@ Source last modified: $Id: sbrqmfak.s,v 1.1 2005/02/26 01:47:35 jrecker Exp $ 
@ 
@ Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.  
@ 
@ The contents of this file, and the files included with this file, 
@ are subject to the current version of the RealNetworks Public 
@ Source License (the "RPSL") available at 
@ http://www.helixcommunity.org/content/rpsl unless you have licensed 
@ the file under the current version of the RealNetworks Community 
@ Source License (the "RCSL") available at 
@ http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
@ will apply. You may also obtain the license terms directly from 
@ RealNetworks.  You may not use this file except in compliance with 
@ the RPSL or, if you have a valid RCSL with RealNetworks applicable 
@ to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
@ the rights, obligations and limitations governing use of the 
@ contents of the file. 
@ 
@ This file is part of the Helix DNA Technology. RealNetworks is the 
@ developer and/or licensor of the Original Code and owns the  
@ copyrights in the portions it created. 
@   
@ This file, and the files included with this file, is distributed 
@ and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
@ KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
@ ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
@ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
@ ENJOYMENT OR NON-INFRINGEMENT. 
@  
@ Technology Compatibility Kit Test Suite(s) Location:  
@    http://www.helixcommunity.org/content/tck  
@  
@ Contributor(s):  
@   
@ ***** END LICENSE BLOCK *****  

        .text
        .code 32
        .align

CPTR0   .req    r0
DELAY   .req    r1
DIDX    .req    r2
UBUF    .req    r3
CPTR1   .req    r4
K       .req    r5
DOFF    .req    r6
ULO0    .req    r7
UHI0    .req    r8
ULO1    .req    r9
UHI1    .req    r10
COEF0   .req    r11
DVAL0   .req    r12
COEF1   .req    r14
DVAL1   .req    r2   @ overlay with DIDX

@void QMFAnalysisConv(int *cTab, int *delay, int dIdx, int *uBuf)
@   see comments in sbrqmf.c

        .global raac_QMFAnalysisConv
raac_QMFAnalysisConv:
        stmfd   sp!, {r4-r11, r14}

        mov     DOFF, DIDX, lsl #5      @ dOff0 = 32*dIdx
        add     DOFF, DOFF, #31         @ dOff0 = 32*dIdx + 31
        add     CPTR1, CPTR0, #4*(164)  @ cPtr1 = cPtr0 + 164
        
        @ special first pass (flip sign for cTab[384], cTab[512])
        ldr     COEF0, [CPTR0], #4
        ldr     COEF1, [CPTR0], #4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smull   ULO0, UHI0, COEF0, DVAL0
        smull   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR0], #4
        ldr     COEF1, [CPTR0], #4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR0], #4
        ldr     COEF1, [CPTR1], #-4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR1], #-4
        ldr     COEF1, [CPTR1], #-4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        rsb     COEF0, COEF0, #0
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR1], #-4
        ldr     COEF1, [CPTR1], #-4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        rsb     COEF0, COEF0, #0
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        str     UHI1, [UBUF, #4*32]
        str     UHI0, [UBUF], #4
        sub     DOFF, DOFF, #1
        mov     K, #31

SRC_Loop_Start:
        ldr     COEF0, [CPTR0], #4
        ldr     COEF1, [CPTR0], #4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smull   ULO0, UHI0, COEF0, DVAL0
        smull   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR0], #4
        ldr     COEF1, [CPTR0], #4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR0], #4
        ldr     COEF1, [CPTR1], #-4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR1], #-4
        ldr     COEF1, [CPTR1], #-4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        ldr     COEF0, [CPTR1], #-4
        ldr     COEF1, [CPTR1], #-4
        ldr     DVAL0, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        ldr     DVAL1, [DELAY, DOFF, lsl #2]
        subs    DOFF, DOFF, #32
        addlt   DOFF, DOFF, #320
        smlal   ULO0, UHI0, COEF0, DVAL0
        smlal   ULO1, UHI1, COEF1, DVAL1

        str     UHI1, [UBUF, #4*32]
        str     UHI0, [UBUF], #4
        sub     DOFF, DOFF, #1
        
        subs    K, K, #1
        bne     SRC_Loop_Start

        ldmfd   sp!, {r4-r11, pc}

        .end
