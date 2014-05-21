@ ***** BEGIN LICENSE BLOCK *****  
@ Source last modified: $Id: sbrqmfsk.s,v 1.1 2005/04/08 21:59:46 jrecker Exp $ 
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

@ void QMFSynthesisConv(int *cPtr, int *delay, int dIdx, short *outbuf, int nChans);
@   see comments in sbrqmf.c

        .global raac_QMFSynthesisConv
raac_QMFSynthesisConv:
        stmfd   sp!, {r4-r11, r14}

        ldr     r9, [r13, #4*9]     @ we saved 9 registers on stack
        mov     r5, r2, lsl #7      @ dOff0 = 128*dIdx
        subs    r6, r5, #1          @ dOff1 = dOff0 - 1
        addlt   r6, r6, #1280       @ if (dOff1 < 0) then dOff1 += 1280
        mov     r4, #64

SRC_Loop_Start:
        ldr     r10, [r0], #4
        ldr     r12, [r0], #4
        ldr     r11, [r1, r5, lsl #2]
        ldr     r14, [r1, r6, lsl #2]
        smull   r7, r8, r10, r11
        subs    r5, r5, #256
        addlt   r5, r5, #1280
        smlal   r7, r8, r12, r14
        subs    r6, r6, #256
        addlt   r6, r6, #1280

        ldr     r10, [r0], #4
        ldr     r12, [r0], #4
        ldr     r11, [r1, r5, lsl #2]
        ldr     r14, [r1, r6, lsl #2]
        smlal   r7, r8, r10, r11
        subs    r5, r5, #256
        addlt   r5, r5, #1280
        smlal   r7, r8, r12, r14
        subs    r6, r6, #256
        addlt   r6, r6, #1280

        ldr     r10, [r0], #4
        ldr     r12, [r0], #4
        ldr     r11, [r1, r5, lsl #2]
        ldr     r14, [r1, r6, lsl #2]
        smlal   r7, r8, r10, r11
        subs    r5, r5, #256
        addlt   r5, r5, #1280
        smlal   r7, r8, r12, r14
        subs    r6, r6, #256
        addlt   r6, r6, #1280

        ldr     r10, [r0], #4
        ldr     r12, [r0], #4
        ldr     r11, [r1, r5, lsl #2]
        ldr     r14, [r1, r6, lsl #2]
        smlal   r7, r8, r10, r11
        subs    r5, r5, #256
        addlt   r5, r5, #1280
        smlal   r7, r8, r12, r14
        subs    r6, r6, #256
        addlt   r6, r6, #1280

        ldr     r10, [r0], #4
        ldr     r12, [r0], #4
        ldr     r11, [r1, r5, lsl #2]
        ldr     r14, [r1, r6, lsl #2]
        smlal   r7, r8, r10, r11
        subs    r5, r5, #256
        addlt   r5, r5, #1280
        smlal   r7, r8, r12, r14
        subs    r6, r6, #256
        addlt   r6, r6, #1280

        add     r5, r5, #1
        sub     r6, r6, #1
        
        add     r8, r8, #0x04
        mov     r8, r8, asr #3    @ FBITS_OUT_QMFS
        mov     r7, r8, asr #31
        cmp     r7, r8, asr #15
        eorne   r8, r7, #0x7f00   @ takes 2 instructions for immediate value of 0x7fffffff
        eorne   r8, r8, #0x00ff
        strh    r8, [r3, #0]
        add     r3, r3, r9, lsl #1
   
        subs    r4, r4, #1
        bne     SRC_Loop_Start

        ldmfd   sp!, {r4-r11, pc}

        .end

