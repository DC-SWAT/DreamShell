!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// Dreamcast BIOS syscall vectors stub
!//
!// Copyright 2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .globl _vectors_init
    .text

_vectors_init:
    mov.l   SYS_FUNC_ADDR, r1
    mov.l   SYS_VECTOR_ADDR, r2
    mov.l   r1, @r2

    mov.l   ROMFONT_FUNC_ADDR, r1
    mov.l   ROMFONT_VECTOR_ADDR, r2
    mov.l   r1, @r2

    mov.l   FLASH_FUNC_ADDR, r1
    mov.l   FLASH_VECTOR_ADDR, r2
    mov.l   r1, @r2

    mov.l   GDC_BC_FUNC_ADDR, r1
    mov.l   GDC_BC_VECTOR_ADDR, r2
    mov.l   r1, @r2

    mov.l   GDC_C0_FUNC_ADDR, r1
    mov.l   GDC_C0_VECTOR_ADDR, r2
    mov.l   r1, @r2

    mov.l   MENU_FUNC_ADDR, r1
    mov.l   MENU_VECTOR_ADDR, r2
    mov.l   r1, @r2

    rts
    nop

.vector_sys:
    mov     #4, r1
    cmp/hs  r1, r7
    bt      .vector_sys_bad
    mov     r7, r1
    mova    .vector_sys_table, r0
    shll2   r1
    mov.l   @(r0, r1), r0
    jmp     @r0
    nop
.vector_sys_bad:
    rts
    mov     #-1, r0

.vector_sys_init:
    rts
    mov     #0, r0

.vector_sys_unknown:
    rts
    mov     #0, r0

.vector_sys_icon:
    rts
    mov     #-1, r0

.vector_sys_id:
    mov.l   SYS_ID_BUF_ADDR, r0
    rts
    nop

.vector_error:
    rts
    mov     #-1, r0

.vector_menu:
    rts
    mov     #0, r0

.vector_gdc_bc:
    mov     #-1, r1
    cmp/eq  r6, r1
    bt/s    .vector_gdc_bc_ok
    mov     #0, r6
    mov     #-1, r6
.vector_gdc_bc_ok:
    rts
    mov     r6, r0

.vector_gdc_c0:
    mov     #-1, r6
    rts
    mov     r6, r0

.romfontfunc:
    mov     #0, r0
    cmp/eq  r0, r1
    bt      .romfont_get_addr
    rts
    nop
.romfont_get_addr:
    mov.l   ROMFONT_ROM_OFFSET, r0
    mov.l   BIOS_ROM_BASE, r1
    add     r1, r0
    rts
    nop

    .align 4

BIOS_ROM_BASE:
    .long   _sg_bios_start

ROMFONT_ROM_OFFSET:
    .long   0x00100020

SYS_VECTOR_ADDR:
    .long   0xAC0000B0
SYS_FUNC_ADDR:
    .long   .vector_sys

    .align 4
.vector_sys_table:
    .long   .vector_sys_init
    .long   .vector_sys_unknown
    .long   .vector_sys_icon
    .long   .vector_sys_id

    .balign 8
.sys_id_buf:
    .long   0, 0
SYS_ID_BUF_ADDR:
    .long   .sys_id_buf

ROMFONT_VECTOR_ADDR:
    .long   0xAC0000B4
ROMFONT_FUNC_ADDR:
    .long   .romfontfunc

FLASH_VECTOR_ADDR:
    .long   0xAC0000B8
FLASH_FUNC_ADDR:
    .long   .vector_error

GDC_BC_VECTOR_ADDR:
    .long   0xAC0000BC
GDC_BC_FUNC_ADDR:
    .long   .vector_gdc_bc

GDC_C0_VECTOR_ADDR:
    .long   0xAC0000C0
GDC_C0_FUNC_ADDR:
    .long   .vector_gdc_c0

MENU_VECTOR_ADDR:
    .long   0xAC0000E0
MENU_FUNC_ADDR:
    .long   .vector_menu
