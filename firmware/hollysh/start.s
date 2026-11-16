!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// Minimal G1 unlock. Turns SDRAM on, writes 0x3FF to 0x005F74E4 and reads
!// 0x000..0x3FF once, in order, from a short reader in RAM. Then jumps to the
!// BIOS entry.
!//
!// Copyright 2025 by megavolt85
!// Copyright 2025-2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .globl	_sg_bios_start

    .macro mov8 reg, byte
        .if \byte > 127
            mov #\byte - 256, \reg
        .else
            mov #\byte, \reg
        .endif
    .endm

    .macro ld32 reg, b3, b2, b1, b0
        mov8 r0, \b3
        shll8 r0
        or #\b2, r0
        shll8 r0
        or #\b1, r0
        shll8 r0
        or #\b0, r0
        .ifnc \reg,r0
            mov r0, \reg
        .endif
    .endm

    .macro st32 b3, b2, b1, b0
        ld32 r0, \b3, \b2, \b1, \b0
        mov.l r0, @r6
        add #4, r6
    .endm

_sg_bios_start:
    ld32 r1, 0xFF, 0x00, 0x00, 0x10
    mov #0, r0
    mov.l r0, @r1                       ! MMUCR
    ld32 r1, 0xFF, 0x00, 0x00, 0x1C
    mov.l r0, @r1                       ! CCR

    mov #-1, r1
    shll16 r1
    shll8 r1
    shar r1
    ldc r1, gbr

    mov #0, r0
    mov.w r0, @(4, gbr)                 ! BCR2

    ld32 r0, 0xA3, 0x02, 0x00, 0x08
    mov.l r0, @(0, gbr)                 ! BCR1

    ld32 r0, 0x01, 0x11, 0x01, 0x11
    mov.l r0, @(8, gbr)                 ! WCR1

    ld32 r0, 0x01, 0x80, 0x60, 0xD8
    mov.l r0, @(12, gbr)                ! WCR2

    ld32 r0, 0xC0, 0x12, 0x12, 0x14
    mov.l r0, @(20, gbr)                ! MCR, temporary, _sh4_init sets the size

    ld32 r0, 0x00, 0x00, 0xA5, 0x10
    mov.w r0, @(28, gbr)                ! RTCSR

    ld32 r0, 0x00, 0x00, 0xA5, 0x5E
    mov.w r0, @(36, gbr)                ! RTCOR

    ld32 r0, 0xFF, 0x94, 0x01, 0x90
    mov #-1, r2
    mov.b r2, @r0                       ! SDMR3

    ld32 r6, 0xAC, 0x00, 0x00, 0x40
    mov r6, r7
    st32 0x64, 0x56, 0x21, 0x02
    st32 0x8B, 0xFC, 0x35, 0x22
    st32 0x43, 0x2B, 0x00, 0x09
    st32 0x00, 0x00, 0x00, 0x09
    mov r7, r6

    ld32 r1, 0xA0, 0x5F, 0x74, 0xE4
    mova .g1_end, r0
    mov r0, r2
    mova .bios_entry_point, r0
    mov r0, r3
    mova .g1_read, r0
    mov r0, r5
    ld32 r0, 0x00, 0x00, 0x03, 0xFF
    nop
    .align 4
    jmp @r6
    nop
    .byte 0, 0, 0, 0

.g1_read:

    .org 0x3F6

g1_sum:
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .word 0x0000

    .org 0x400

.g1_end:
    .include "bios_cfg.s"

    .org 0x420

.bios_entry_point:
    nop
    mov.l .bios_init_addr, r0
    jmp @r0
    nop

.bios_init_addr:
    .long .start

    .org 0x440

    .globl _hollysh_reset
_hollysh_reset:
    mov.l .reset_fn, r0
    jmp @r0
    nop
    nop
.reset_fn:
    .long _bios_reset

    .org 0x460

    .globl _hollysh_test
_hollysh_test:
    mov.l .test_fn, r0
    jmp @r0
    nop
    nop
.test_fn:
    .long _bios_test

    .org 0x480

    .globl _hollysh_game
_hollysh_game:
    mov.l .game_fn, r0
    jmp @r0
    nop
    nop
.game_fn:
    .long _bios_game
