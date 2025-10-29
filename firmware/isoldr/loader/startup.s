!   This file is part of DreamShell ISO Loader
!   Copyright (C) 2009-2016, 2025 SWAT <http://www.dc-swat.ru>
!   Based on Sylverant PSO Patcher code by Lawrence Sebald
!
!   This program is free software: you can redistribute it and/or modify
!   it under the terms of the GNU General Public License version 3 as
!   published by the Free Software Foundation.
!
!   This program is distributed in the hope that it will be useful,
!   but WITHOUT ANY WARRANTY; without even the implied warranty of
!   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!   GNU General Public License for more details.
!
!   You should have received a copy of the GNU General Public License
!   along with this program. If not, see <http://www.gnu.org/licenses/>.
!
    .globl      start
    .globl      _loader_addr
    .globl      _loader_size
    .globl      _bios_patch_base
    .globl      _bios_patch_handler
    .globl      _bios_patch_end
    .globl      _boot_stub
    .globl      _boot_stub_len
    .globl      _boot_stack
    .globl      _boot_sr
    .globl      _boot_vbr

    .text
start:
    ! First, make sure to run in the P2 area
    mov.l       setup_cache_addr, r0
    mov.l       p2_mask, r1
    or          r1, r0
    jmp         @r0
    nop
setup_cache:
    ! Now that we are in P2, it's safe to enable the cache
    mov.l       ccr_addr, r0
    mov.l       ccr_data, r1
    mov.l       r1, @r0
    ! After changing CCR, eight instructions must be executed before it's safe
    ! to enter a cached area such as P1
    mov.l       initaddr, r0        ! 1
    mov         #0, r1              ! 2
    nop                             ! 3
    nop                             ! 4
    nop                             ! 5
    nop                             ! 6
    nop                             ! 7
    nop                             ! 8
    jmp         @r0                 ! go
    mov         r1, r0
init:
    ! Clear BSS section
    mov.l       bss_start_addr, r0
    mov.l       bss_end_addr, r2
    sub         r0, r2
    shlr2       r2
    mov         #0, r1
.loop:
    dt          r2
    mov.l       r1, @r0
    bf/s        .loop
    add         #4, r0
    mov         #0, r2
    mov         #0, r1
    mov.l       mainaddr, r0
    mov.l       stack_ptr, r15
    jmp         @r0
    mov         #0, r0

    .align      4
mainaddr:
    .long       _main
initaddr:
    .long       init
bss_start_addr:
    .long       __bss_start
bss_end_addr:
    .long       _end
setup_cache_addr:
    .long       setup_cache
p2_mask:
    .long       0xa0000000
stack_ptr:
    .long       0x8d000000
_loader_addr:
    .long       start
_loader_size:
    .long       _end - start
_boot_stub_len:
    .long       _boot_stub_len - _boot_stub

    .align 2
bios_patch_null:
    nop
    rts
    nop

_bios_patch_base:
    mov.l   _bios_patch_handler, r0
    jmp     @r0

    .align  4
_bios_patch_handler:
    .long   bios_patch_null
_bios_patch_end:

	.align 2
    ! This MUST always be run from a non-cacheable area of memory, since it
    ! messes with the CCR register.
_boot_stub:
    ! Set up some registers we will need to deal with later...
    mov         r4, r14
    mov.l       newr15, r0
    mov.l       @r0, r15
    ldc         r14, spc
    mov.l       newgbr, r0
    mov.l       ccr_addr, r3
    ldc         r0, gbr
    mov.l       newvbr, r1
    mov.l       @r1, r1
    mov.l       ccr_data, r2
    mov.l       newsr, r4
    mov.l       @r4, r4
    ldc         r1, vbr
    mov.l       newfpscr, r5
    ldc         r4, sr
    lds         r5, fpscr
    ldc         r4, ssr
    mov.l       r2, @r3             ! Set the CCR to clear the caches and such.
    ! Now, we need to execute a few instructions before we can go back to any
    ! cached areas, like P1. Of course, we actually are not going to jump to P1
    ! at the end of this anyway, so it really does not matter all that much...
    mov         #0, r0              ! 1
    mov         #0, r1              ! 2
    mov         #0, r2              ! 3
    mov         #0, r3              ! 4
    mov         #0, r4              ! 5
    mov         #0, r6              ! 7
    mov         #0, r7              ! 8 (safe to go back to P1 now)
    mov.l       startaddr, r12
    mov         #0, r8
    mov         #0, r9
    lds         r12, pr
    mov         #0, r10
    mov         #0, r11
    mov         #0, r12
    mov         #0, r13
    clrmac
    clrt
    jmp         @r14                ! Go!
    mov         #0, r14
    ! We should not ever get back here.

    .align 4
ccr_data:
    .long       0x00000909
ccr_addr:
    .long       0xff00001c
_boot_stack:
    .long       0x8c00f400          ! r15
newr15:
    .long       _boot_stack
newgbr:
    .long       0x8c000000          ! gbr
_boot_vbr:
    .long       0x8c00f400          ! vbr
newvbr:
    .long       _boot_vbr
_boot_sr:
    .long       0x700000f0          ! sr
newsr:
    .long       _boot_sr
newfpscr:
    .long       0x00040001          ! fpscr
startaddr:
    .long       start
p2mask:
    .long       0xa0000000
