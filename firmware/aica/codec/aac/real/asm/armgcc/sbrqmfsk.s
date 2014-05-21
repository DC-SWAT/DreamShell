@ ***** BEGIN LICENSE BLOCK *****  
@ Source last modified: $Id: sbrqmfsk.s,v 1.1 2005/02/26 01:47:35 jrecker Exp $ 
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

CPTR    .req    r0
DELAY   .req    r1
DIDX    .req    r2
OUTBUF  .req    r3
K       .req    r4
DOFF0   .req    r5
DOFF1   .req    r6
SUMLO   .req    r7
SUMHI   .req    r8
NCHANS  .req    r9
COEF0   .req    r10
DVAL0   .req    r11
COEF1   .req    r12
DVAL1   .req    r14

        .set    FBITS_OUT_QMFS, (14 - (1+2+3+2+1) - (2+3+2) + 6 - 1)
        .set    RND_VAL,    (1 << (FBITS_OUT_QMFS - 1))

@ void QMFSynthesisConv(int *cPtr, int *delay, int dIdx, short *outbuf, int nChans);
@   see comments in sbrqmf.c

        .global raac_QMFSynthesisConv
raac_QMFSynthesisConv:
        stmfd   sp!, {r4-r11, r14}

        ldr     NCHANS,  [r13, #4*9]    @ we saved 9 registers on stack
        mov     DOFF0, DIDX, lsl #7 @ dOff0 = 128*dIdx
        subs    DOFF1, DOFF0, #1    @ dOff1 = dOff0 - 1
        addlt   DOFF1, DOFF1, #1280 @ if (dOff1 < 0) then dOff1 += 1280
        mov K, #64

SRC_Loop_Start:
        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smull   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        addlt   DOFF1, DOFF1, #1280

        ldr     COEF0, [CPTR], #4
        ldr     COEF1, [CPTR], #4
        ldr     DVAL0, [DELAY, DOFF0, lsl #2]
        ldr     DVAL1, [DELAY, DOFF1, lsl #2]
        smlal   SUMLO, SUMHI, COEF0, DVAL0
        subs    DOFF0, DOFF0, #256
        addlt   DOFF0, DOFF0, #1280
        smlal   SUMLO, SUMHI, COEF1, DVAL1
        subs    DOFF1, DOFF1, #256
        addlt   DOFF1, DOFF1, #1280

        add     DOFF0, DOFF0, #1
        sub     DOFF1, DOFF1, #1
        
        add     SUMHI, SUMHI, #RND_VAL
        mov     SUMHI, SUMHI, asr #FBITS_OUT_QMFS
        mov     SUMLO, SUMHI, asr #31
        cmp     SUMLO, SUMHI, asr #15
        eorne   SUMHI, SUMLO, #0x7f00   @ takes 2 instructions for immediate value of 0x7fffffff
        eorne   SUMHI, SUMHI, #0x00ff
        strh    SUMHI, [OUTBUF, #0]
        add     OUTBUF, OUTBUF, NCHANS, lsl #1
   
        subs    K, K, #1
        bne     SRC_Loop_Start

        ldmfd   sp!, {r4-r11, pc}

        .end

