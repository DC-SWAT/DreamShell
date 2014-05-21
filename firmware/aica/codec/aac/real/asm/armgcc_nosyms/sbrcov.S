@ ***** BEGIN LICENSE BLOCK *****  
@ Source last modified: $Id: sbrcov.s,v 1.1 2005/04/08 21:59:46 jrecker Exp $ 
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

@ void CVKernel1(int *XBuf, int *accBuf)
@   see comments in sbrhfgen.c

        .global raac_CVKernel1
raac_CVKernel1:

        stmfd   sp!, {r4-r11, r14}

        ldr     r3, [r0], #4*(1)
        ldr     r4, [r0], #4*(2*64-1)
        ldr     r5, [r0], #4*(1)
        ldr     r6, [r0], #4*(2*64-1)
        rsb     r14, r4, #0

        smull   r7, r8, r5, r3
        smlal   r7, r8, r6, r4
        smull   r9, r10, r3, r6
        smlal   r9, r10, r14, r5
        smull   r11, r12, r3, r3
        smlal   r11, r12, r4, r4

        add     r2, r1, #(4*6)
        stmia   r2, {r7-r12}

        mov     r7, #0
        mov     r8, #0
        mov     r9, #0
        mov     r10, #0
        mov     r11, #0
        mov     r12, #0
        
        mov     r2, #(16*2 + 6)
CV1_Loop_Start:
        mov     r3, r5
        ldr     r5, [r0], #4*(1)
        mov     r4, r6
        ldr     r6, [r0], #4*(2*64-1)
        rsb     r14, r4, #0
        
        smlal   r7, r8, r5, r3
        smlal   r7, r8, r6, r4
        smlal   r9, r10, r3, r6
        smlal   r9, r10, r14, r5
        smlal   r11, r12, r3, r3
        smlal   r11, r12, r4, r4

        subs    r2, r2, #1
        bne     CV1_Loop_Start

        stmia   r1, {r7-r12}

        ldr     r0, [r1, #4*(6)]
        ldr     r2,  [r1, #4*(7)]
        rsb     r3, r3, #0
        adds    r7, r0, r7
        adc     r8, r2,  r8
        smlal   r7, r8, r5, r3
        smlal   r7, r8, r6, r14

        ldr     r0, [r1, #4*(8)]
        ldr     r2,  [r1, #4*(9)]
        adds    r9, r0, r9      
        adc     r10, r2,  r10
        smlal   r9, r10, r3, r6
        smlal   r9, r10, r4, r5
        
        ldr     r0, [r1, #4*(10)]
        ldr     r2,  [r1, #4*(11)]
        adds    r11, r0, r11
        adc     r12, r2, r12
        rsb     r0, r3, #0
        smlal   r11, r12, r3, r0
        rsb     r2,  r4, #0
        smlal   r11, r12, r4, r2
        
        add     r1, r1, #(4*6)
        stmia   r1, {r7-r12}

    ldmfd   sp!, {r4-r11, pc}

@ void CVKernel2(int *XBuf, int *accBuf)
@   see comments in sbrhfgen.c

        .global raac_CVKernel2
raac_CVKernel2:
        stmfd   sp!, {r4-r11, r14}
        
        mov     r7, #0
        mov     r8, #0
        mov     r9, #0
        mov     r10, #0
        
        ldr     r3, [r0], #4*(1)
        ldr     r4, [r0], #4*(2*64-1)
        ldr     r5, [r0], #4*(1)
        ldr     r6, [r0], #4*(2*64-1)

        mov     r2, #(16*2 + 6)
CV2_Loop_Start:
        ldr     r11, [r0], #4*(1)
        ldr     r12, [r0], #4*(2*64-1)
        rsb     r14, r4, #0

        smlal   r7, r8, r11, r3
        smlal   r7, r8, r12, r4
        smlal   r9, r10, r3, r12
        smlal   r9, r10, r14, r11

        mov     r3, r5
        mov     r4, r6
        mov     r5, r11
        mov     r6, r12

        subs    r2, r2, #1
        bne     CV2_Loop_Start

        stmia   r1, {r7-r10}

        ldmfd   sp!, {r4-r11, pc}

        .end
