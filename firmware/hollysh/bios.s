!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!// Main initialization and loader loading
!//
!// Copyright 2026 by SWAT
!// Derived from JinGasa BIOS by T_chan
!//
!///////////////////////////////////////////////////////////////////////////////
    .globl .start
    .globl _bios_init
    .text

.start:
.bios_to_p2:
    mov.l BIOS_INIT_ADDR, r0
    mov.l RAM_ADDRESS_MASK, r1
    and r1, r0
    mov.l RAM_AREA_P2_MASK, r1
    or r1, r0
	jmp	@r0
	nop
    nop

_bios_init:
    mov.l sh4_init_addr, r0
    jsr @r0
    nop

    mov.l holly_init_addr, r0
    jsr @r0
    nop

    mov.l VECTORS_INIT_ADDR, r0
    jsr @r0
    nop

.loader_copy_start:
    mov.l LOADER_ROM_OFFSET, r0
    mov.l BIOS_LOAD_ADDR, r1
    add r1, r0
    mov.l RAM_AREA_P2_MASK, r1
    or r1, r0
    mov.l LOADER_SIZE, r3
    mov.l LOADER_BIN_ADDR, r2
    mov.l RAM_AREA_P2_MASK, r1
    or r1, r2
    tst r3, r3
    bt .loader_copy_done

loader_copy_loop:
    mov.l @r0+, r1
    mov.l r1, @r2
    add #4, r2
    dt r3
    bf/s loader_copy_loop
    nop

.loader_copy_done:
    mov.l LOADER_BIN_ADDR, r0
    mov.l RAM_AREA_P2_MASK, r1
    or r1, r0
    jmp	@r0
    nop

    .align 4

BIOS_LOAD_ADDR:
    .long        _sg_bios_start

RAM_ADDRESS_MASK:
    .long        0x1FFFFFFF
RAM_AREA_P2_MASK:
    .long        0xA0000000
BIOS_INIT_ADDR:
    .long        _bios_init
sh4_init_addr:
    .long        _sh4_init
holly_init_addr:
    .long        _holly_init
VECTORS_INIT_ADDR:
    .long        _vectors_init

    .include     "loader_cfg.s"
