! DreamShell memtest OCRAM helpers
! Copyright (C) 2026 SWAT

    .text
    .align 2
    .globl _memtest_ocram_set
    .globl _memtest_call_on_ocram

_memtest_ocram_set:
    mov.l    ccr_addr, r6
    mov.l    @r6, r0
    mov.l    ora_bit, r1
    not      r1, r2
    and      r2, r0
    tst      r4, r4
    bt       .ora_ready
    or       r1, r0
.ora_ready:
    mov.l    oci_bit, r1
    or       r1, r0
    mov      r0, r4
    mov.l    block_bit, r0
    stc      sr, r7
    or       r7, r0
    ldc      r0, sr
    mova     .ccr_p2, r0
    mov.l    p2_mask, r1
    or       r1, r0
    jmp      @r0
    nop

    .align 2
.ccr_p2:
    mov.l    loc_tags, r0
    mov      #2, r1
    shll8    r1
    mov      #0, r2
.ccr_flush:
    mov.l    r2, @r0
    dt       r1
    add      #32, r0
    bf       .ccr_flush
    mov.l    r4, @r6
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    ldc      r7, sr
    rts
    nop

_memtest_call_on_ocram:
    mov.l    r8, @-r15
    mov.l    r9, @-r15
    mov.l    r10, @-r15
    mov.l    r11, @-r15
    mov.l    r12, @-r15
    mov.l    r13, @-r15
    mov.l    r14, @-r15
    sts.l    pr, @-r15
    mov      r15, r8
    mov.l    ocram_sp, r15
    mov      r4, r2
    jsr      @r2
    mov      r5, r4
    mov      r8, r15
    lds.l    @r15+, pr
    mov.l    @r15+, r14
    mov.l    @r15+, r13
    mov.l    @r15+, r12
    mov.l    @r15+, r11
    mov.l    @r15+, r10
    mov.l    @r15+, r9
    rts
    mov.l    @r15+, r8

    .align 2
ccr_addr:
    .long    0xFF00001C
ora_bit:
    .long    0x00000020
oci_bit:
    .long    0x00000008
block_bit:
    .long    0x10000000
p2_mask:
    .long    0xA0000000
loc_tags:
    .long    0xF4000000
ocram_sp:
    .long    0x7C002FF0
