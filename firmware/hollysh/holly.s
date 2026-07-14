!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// Initialization of the HOLLY ASICs for various systems
!//
!// Copyright 2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .globl _holly_init
    .text

_holly_init:

    mov.l FB_R_CTRL_DISABLE, r1
    mov.l FB_R_CTRL_ADDR, r2
    mov.l r1, @r2

    mov.l ELAN_MAGIC_ADDR, r2
    mov.l @r2, r0
    mov.l ELAN_MAGIC_VALUE, r1
    cmp/eq r0, r1
    bt .holly_init_naomi2

    bra .holly_init_clxa
    nop

.holly_init_naomi2:
    mov.l .elan_tbl_ptr, r1
    mov.l .elan_tbl_end_ptr, r2
.elan_loop:
    mov.l @r1+, r3
    mov.l @r1+, r4
    mov.l r4, @r3
    cmp/eq r1, r2
    bf .elan_loop

    mov.l .clxb_tbl_ptr, r1
    mov.l .clxb_tbl_end_ptr, r2
.clxb_loop:
    mov.l @r1+, r3
    mov.l @r1+, r4
    mov.l r4, @r3
    cmp/eq r1, r2
    bf .clxb_loop

    mov.l .clxc_tbl_ptr, r1
    mov.l .clxc_tbl_end_ptr, r2
.clxc_loop:
    mov.l @r1+, r3
    mov.l @r1+, r4
    mov.l r4, @r3
    cmp/eq r1, r2
    bf .clxc_loop

.holly_init_clxa:
    mov.l .clxa_tbl_ptr, r1
    mov.l .clxa_tbl_end_ptr, r2
.clxa_loop:
    mov.l @r1+, r3
    mov.l @r1+, r4
    mov.l r4, @r3
    cmp/eq r1, r2
    bf .clxa_loop

    mov.l holly_init_primary_addr, r3
    jmp @r3
    nop

    .align 4
.elan_tbl_ptr:
    .long .elan_tbl
.elan_tbl_end_ptr:
    .long .elan_tbl_end
.clxb_tbl_ptr:
    .long .clxb_tbl
.clxb_tbl_end_ptr:
    .long .clxb_tbl_end
.clxc_tbl_ptr:
    .long .clxc_tbl
.clxc_tbl_end_ptr:
    .long .clxc_tbl_end
.clxa_tbl_ptr:
    .long .clxa_tbl
.clxa_tbl_end_ptr:
    .long .clxa_tbl_end
holly_init_primary_addr:
    .long .holly_init_primary

.holly_init_primary:
    xor r0, r0
    mov.l VRAM_BASE_ADDR, r2
    mov.l FB_CLEAR_DWORDS, r3
.fb_clear_loop:
    mov.l r0, @r2
    dt r3
    bf/s .fb_clear_loop
    add #4, r2

    rts
    nop

    .align 4

FB_R_CTRL_ADDR:
    .long        0xa05f8044
FB_R_CTRL_DISABLE:
    .long        0x00800004

ELAN_MAGIC_ADDR:
    .long        0xA8800000
ELAN_MAGIC_VALUE:
    .long        0xE1AD0000

VRAM_BASE_ADDR:
    .long        0xA5000000
FB_CLEAR_DWORDS:
    .long        0x00025800

    .include "clxa_tbl.s"
    .include "elan_tbl.s"
    .include "clxb_tbl.s"
    .include "clxc_tbl.s"
