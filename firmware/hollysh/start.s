!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// This code is used to unlock the HOLLY G1 bus,
!// hardware YUV converter and full PVR performance
!//
!// Copyright 2025 by megavolt85
!// Copyright 2025-2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .globl	_sg_bios_start
_sg_bios_start:
    mov		#-1, r6
    shll16	r6
    swap.w	r6, r7
    shll8	r6
    shlr2	r7
    ldc		r6, gbr
    shlr2	r7
    mov.l	@(36, gbr), r0
    xor		r0, r7
    and		#0xFF, r0
    mulu.w	r7, r0
    sts		macl, r0
    tst		#0xFF, r0
    bf		.jump_to_exception_hnd
    mov.l	r0, @(16, gbr)
    mov.l	r0, @(28, gbr)
    shar	r6
    ldc		r6, gbr
    mov.w	r0, @(4, gbr)
    mov		#-93, r0
    shll8	r0
    or		#2, r0
    shll16	r0
    or		#8, r0
    mov.l	r0, @(0, gbr)
    mov		#1, r0
    shll8	r0
    or		#0x11, r0
    mov		r0, r1
    shll16	r0
    or		r1, r0
    mov.l	r0, @(8, gbr)
    add		#0x6F, r0
    shll8	r0
    or		#0x60, r0
    shll8	r0
    or		#0xD8, r0
    mov.l	r0, @(12, gbr)
    mov		#-64, r0
    shll8	r0
    or		#0x12, r0
    shll8	r0
    or		#0x12, r0
    shll8	r0
    or		#0x14, r0
    mov.l	r0, @(20, gbr)
    mov		#-91, r0
    shll8	r0
    or		#0x10, r0
    mov.w	r0, @(28, gbr)
    add		#0x4E, r0
    mov.w	r0, @(36, gbr)
    mov		#0x14, r0
    shll8	r0
    or		#1, r0
    shll8	r0
    or		#0x90, r0
    add		r0, r6
    ldc		r6, gbr
    mov		#-1, r0
    mov.b	r0, @(0, gbr)
    mov		#-84, r6
    shll16	r6
    add		#1, r6
    shll8	r6
    mov		#7, r0
    shll8	r0
    or		#0xC0, r0
    shll8	r0
    or		#3, r0
    shll8	r0
    or		#0xFF, r0
    mov.l	r0, @-r6
    mov		#0xA0, r0
    shll8	r0
    or		#0x5F, r0
    shll8	r0
    or		#0x74, r0
    shll8	r0
    or		#0xE4, r0
    mov.l	r0, @-r6
    mov		#-119, r0
    shll8	r0
    or		#0x13, r0
    shll8	r0
    or		#0x8B, r0
    shll8	r0
    or		#0xFA, r0
    mov.l	r0, @-r6
    mov		#0x40, r0
    shll8	r0
    or		#0x10, r0
    shll8	r0
    or		#0x76, r0
    shll8	r0
    or		#4, r0
    mov.l	r0, @-r6
    mov		#0x26, r0
    shll8	r0
    or		#0x12, r0
    shll8	r0
    or		#0x61, r0
    shll8	r0
    or		#0x56, r0
    mov.l	r0, @-r6
    mov		#0x90, r0
    shll8	r0
    or		#8, r0
    shll8	r0
    or		#0x21, r0
    shll8	r0
    or		#2, r0
    mov.l	r0, @-r6
    mov		#0xD1, r0
    shll8	r0
    or		#4, r0
    shll8	r0
    or		#0x90, r0
    shll8	r0
    or		#0x0A, r0
    mov.l	r0, @-r6
    mova	.exception_hnd, r0
    mov		r0, r5
    nop
    jmp		@r6
    add		#0x1C, r6

    .long	0x270000FF

    .org	0x100

.exception_hnd:
    mov.l	r0, @-r15
    stc.l	gbr, @-r15
    mov		#0xFF, r0
    shll16	r0
    shll8	r0
    ldc		r0, gbr
    mov.l	@(36, gbr), r0
    ldc.l	@r15+, gbr
    mov.l	@r15+, r0
    rte
    nop
    nop

.jump_to_exception_hnd:
    bra		.exception_hnd
    nop
    nop
    nop

    .org	0x120

    mov.l	.reg_sr_val, r0
    ldc		r0, sr
    mov.l	.stack_val, r15
    mov.l	r7, @-r15
    stc.l	sr, @-r15
    mov.l	.reg_sr_val2, r0
    ldc		r0, sr
    mova	.reg_array2, r0
    mov.l	@r0+, r14
    mov.l	@r0+, r1
    mov.l	@r0+, r2
    mov.l	@r0+, r3
    mov.l	@r0+, r4
    mov.l	@r0+, r5
    mov.l	@r0+, r6
    mov.l	@r0+, r7
    mov		r14, r0
    ldc.l	@r15+, sr
    mova	.reg_array, r0
    mov.l	@r0+, r1
    mov.l	@r0+, r1
    mov.l	@r0+, r2
    mov.l	@r0+, r3
    mov.l	@r0+, r4
    mov.l	@r0+, r5
    mov.l	@r0+, r6
    mov.l	@r0+, r7
    mov.l	@r0+, r8
    mov.l	@r0+, r9
    mov.l	@r0+, r10
    mov.l	@r0+, r11
    mov.l	@r0+, r12
    mov.l	@r0+, r13
    mov.l	@r0+, r14
    mova	.reg_array3, r0
    lds.l	@r0+, mach
    lds.l	@r0+, macl
    lds.l	@r0+, pr
    ldc.l	@r0+, gbr
    ldc.l	@r0+, vbr
    nop
    nop
    nop
    mov.l	.reg_fpscr_val, r0
    lds		r0, fpscr
    sts.l	fpscr, @-r15
    mov.l	.reg_fpscr_val2, r0
    lds		r0, fpscr
    mova	.fpu_val2, r0
    fmov	@r0+, fr0
    fmov	@r0+, fr2
    fmov	@r0+, fr4
    fmov	@r0+, fr6
    fmov	@r0+, fr8
    fmov	@r0+, fr10
    fmov	@r0+, fr12
    fmov	@r0+, fr14
    lds.l	@r15+, fpscr
    mova	.fpu_val, r0
    fmov	@r0+, fr0
    fmov	@r0+, fr2
    fmov	@r0+, fr4
    fmov	@r0+, fr6
    fmov	@r0+, fr8
    fmov	@r0+, fr10
    fmov	@r0+, fr12
    fmov	@r0+, fr14
    mova	.fpul_val, r0
    lds.l	@r0+, fpul
    ldc.l	@r0+, ssr
    ldc.l	@r0+, ssr
    ldc.l	@r0+, spc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    bra		.bios_entry_point
    nop

    .org	0x200

.reg_array:	
    .long	0xDEADDEAD, 0x11111111, 0x22222222, 0x33333333, 0x44444444
    .long	0x55555555, 0x66666666, 0x77777777, 0x88888888, 0x99999999
    .long	0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE
.stack_val:
    .long	0x8C010000
.reg_array2:
    .long	0xB0B0B0B0, 0xB1B1B1B1, 0xB2B2B2B2, 0xB3B3B3B3, 0xB4B4B4B4
    .long	0xB5B5B5B5, 0xB6B6B6B6, 0xB7B7B7B7
.reg_array3:
    .long	0xAC00AC00, 0xAC11AC11
    .long	0x8C000000, 0xFF000000, 0x8C000000

.reg_sr_val:
    .long	0x700000F0
.reg_sr_val2:
    .long	0x500000F0
    .long	0xAC000000
.fpu_val:
    .long	0x0070F0F0, 0x0071F1F1, 0x0072F2F2, 0x0073F3F3, 0x0074F4F4
    .long	0x0075F5F5, 0x0076F6F6, 0x0077F7F7, 0x0078F8F8, 0x0079F9F9
    .long	0x007AFAFA, 0x007BFBFB, 0x007CFCFC, 0x007DFDFD, 0x007EFEFE
    .long	0x007FFFFF
.fpu_val2:
    .long	0x0070B0B0, 0x0071B1B1, 0x0072B2B2, 0x0073B3B3, 0x0074B4B4
    .long	0x0075B5B5, 0x0076B6B6, 0x0077B7B7, 0x0078B8B8, 0x0079B9B9
    .long	0x007ABABA, 0x007BBBBB, 0x007CBCBC, 0x007DBDBD, 0x007EBEBE
    .long	0x007FBFBF

.fpul_val:
    .long	0xFFFFFFFF
.reg_fpscr_val:
    .long	0x00040001
    .long	0x700000F0
    .long	0xFFFFFFFF
.reg_fpscr_val2:
    .long	0x00240001
    .long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .long	0xCF000000
    .long	0xAD53CECE
    .long	0x0000ADAD

    .org 0x400

    .include     "bios_cfg.s"

    .org 0x420

.bios_entry_point:
    nop
    mov.l	.bios_init_addr, r0
    jmp		@r0
    nop

.bios_init_addr:
    .long	.start
