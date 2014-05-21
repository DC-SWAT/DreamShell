@ ***** BEGIN LICENSE BLOCK *****  
@ Source last modified: $Id: sbrqmfak.s,v 1.1 2005/04/08 21:59:46 jrecker Exp $ 
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

@void QMFAnalysisConv(int *cTab, int *delay, int dIdx, int *uBuf)
@   see comments in sbrqmf.c

        .global raac_QMFAnalysisConv
raac_QMFAnalysisConv:
        stmfd   sp!, {r4-r11, r14}

        mov     r6, r2, lsl #5      @ dOff0 = 32*dIdx
        add     r6, r6, #31         @ dOff0 = 32*dIdx + 31
        add     r4, r0, #4*(164)    @ cPtr1 = cPtr0 + 164
        
        @ special first pass (flip sign for cTab[384], cTab[512])
        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smull   r7, r8, r11, r12
        smull   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        rsb     r11, r11, #0
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        rsb     r11, r11, #0
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        str     r10, [r3, #4*32]
        str     r8, [r3], #4
        sub     r6, r6, #1
        mov     r5, #31

SRC_Loop_Start:
        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smull   r7, r8, r11, r12
        smull   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r0], #4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r0], #4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        ldr     r11, [r4], #-4
        ldr     r14, [r4], #-4
        ldr     r12, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        ldr     r2, [r1, r6, lsl #2]
        subs    r6, r6, #32
        addlt   r6, r6, #320
        smlal   r7, r8, r11, r12
        smlal   r9, r10, r14, r2

        str     r10, [r3, #4*32]
        str     r8, [r3], #4
        sub     r6, r6, #1
        
        subs    r5, r5, #1
        bne     SRC_Loop_Start

        ldmfd   sp!, {r4-r11, pc}

        .end
