!   This file is part of DreamShell ISO Loader
!   Copyright (C) 2014-2020 SWAT <http://www.dc-swat.ru>
!   Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
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

    .section .text

!    .global _cache_sub_handler
!    .global _cache_sub_handler_base
!    .global _cache_sub_handler_end
    .global _interrupt_sub_handler
    .global _interrupt_sub_handler_base
    .global _interrupt_sub_handler_end
    .global _my_exception_finish
    .global _r15
    .global _vbr
    .global _exception_os_type
    .global _interrupt_stack

#ifdef USE_GDB
    .global _ubc_handler_lowlevel
    .global _ubc_wait
    .global _general_sub_handler
    .global _general_sub_handler_base
    .global _general_sub_handler_end
    .global _dbr
    .global _dbr_set
    .global _sgr
_dbr:
    stc     DBR, r0
    rts
    nop

_dbr_set:
    ldc     r4, DBR
    rts
    nop

_sgr:
    stc     SGR, r0
    rts
    nop
#endif

_r15:
    mov     r15, r0
    rts
    nop
	
_vbr:
    stc     VBR, r0
    rts
    nop

!
! HANDLE GENERAL EXCEPTIONS
!

#ifdef USE_GDB
_ubc_handler_lowlevel:
ubc_exception:
!BEGIN REGISTER SAVE
!UBC via DBR
    mov.l   r0, @-r15
    mov     #1, r0
    mov.l   r0, @-r15
#endif
general_exception_xt:
    mov.l   r1, @-r15
    mov.l   r2, @-r15
    mov.l   r3, @-r15
    mov.l   r4, @-r15
    mov.l   r5, @-r15
    mov.l   r6, @-r15
    mov.l   r7, @-r15
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    mov.l   r12, @-r15
    mov.l   r13, @-r15
    mov.l   r14, @-r15
!FLOATING POINT
#if 0//defined(__SH_FPU_ANY__)
    sts.l   fpul, @-r15
    sts.l   fpscr, @-r15
    mov     #0, r2          ! Set known FP flags
    lds     r2, fpscr
    fmov.s  fr15, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr11, @-r15
    fmov.s  fr10, @-r15
    fmov.s  fr9, @-r15
    fmov.s  fr8, @-r15
    fmov.s  fr7, @-r15
    fmov.s  fr6, @-r15
    fmov.s  fr5, @-r15
    fmov.s  fr4, @-r15
    fmov.s  fr3, @-r15
    fmov.s  fr2, @-r15
    fmov.s  fr1, @-r15
    fmov.s  fr0, @-r15
    frchg
    fmov.s  fr15, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr11, @-r15
    fmov.s  fr10, @-r15
    fmov.s  fr9, @-r15
    fmov.s  fr8, @-r15
    fmov.s  fr7, @-r15
    fmov.s  fr6, @-r15
    fmov.s  fr5, @-r15
    fmov.s  fr4, @-r15
    fmov.s  fr3, @-r15
    fmov.s  fr2, @-r15
    fmov.s  fr1, @-r15
    fmov.s  fr0, @-r15
    frchg
#endif
!CONTROL
    stc.l   r0_bank, @-r15
    stc.l   r1_bank, @-r15
    stc.l   r2_bank, @-r15
    stc.l   r3_bank, @-r15
    stc.l   r4_bank, @-r15
    stc.l   r5_bank, @-r15
    stc.l   r6_bank, @-r15
    stc.l   r7_bank, @-r15
    stc.l   dbr, @-r15
    stc.l   sr, @-r15
    stc.l   gbr, @-r15
    stc.l   vbr, @-r15
!SAVED CONTROL (exception)
    stc.l   ssr, @-r15
!    stc.l   sgr, @-r15
    stc.l   spc, @-r15
!SYSTEM
    sts.l   macl, @-r15
    sts.l   mach, @-r15
    sts.l   pr, @-r15
!END REGISTER SAVE

    ! Arguments are passed on r4
    mov     r15, r4

    mov.l   hdl_except, r2
    jsr     @r2
    nop

    ! The return value is put in r0

!BEGIN REGISTER RESTORE
!SYSTEM
    lds.l   @r15+, pr
    lds.l   @r15+, mach
    lds.l   @r15+, macl
!SAVED CONTROL (exception)
    ldc.l   @r15+, spc
!    ldc.l   @r15+, sgr
    ldc.l   @r15+, ssr
!CONTROL
    ldc.l   @r15+, vbr
    ldc.l   @r15+, gbr
    ldc.l   @r15+, sr
    ldc.l   @r15+, dbr
    ldc.l   @r15+, r7_bank
    ldc.l   @r15+, r6_bank
    ldc.l   @r15+, r5_bank
    ldc.l   @r15+, r4_bank
    ldc.l   @r15+, r3_bank
    ldc.l   @r15+, r2_bank
    ldc.l   @r15+, r1_bank
    ldc.l   @r15+, r0_bank
!FLOATING POINT
#if 0//defined(__SH_FPU_ANY__)
    mov     #0, r2          ! Set known FP flags
    lds     r2, fpscr
    frchg
    fmov.s  @r15+, fr0
    fmov.s  @r15+, fr1
    fmov.s  @r15+, fr2
    fmov.s  @r15+, fr3
    fmov.s  @r15+, fr4
    fmov.s  @r15+, fr5
    fmov.s  @r15+, fr6
    fmov.s  @r15+, fr7
    fmov.s  @r15+, fr8
    fmov.s  @r15+, fr9
    fmov.s  @r15+, fr10
    fmov.s  @r15+, fr11
    fmov.s  @r15+, fr12
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr15
    frchg
    fmov.s  @r15+, fr0
    fmov.s  @r15+, fr1
    fmov.s  @r15+, fr2
    fmov.s  @r15+, fr3
    fmov.s  @r15+, fr4
    fmov.s  @r15+, fr5
    fmov.s  @r15+, fr6
    fmov.s  @r15+, fr7
    fmov.s  @r15+, fr8
    fmov.s  @r15+, fr9
    fmov.s  @r15+, fr10
    fmov.s  @r15+, fr11
    fmov.s  @r15+, fr12
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr15
    lds.l   @r15+, fpscr
    lds.l   @r15+, fpul
#endif
!GENERAL
    mov.l   @r15+, r14
    mov.l   @r15+, r13
    mov.l   @r15+, r12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    mov.l   @r15+, r8
    mov.l   @r15+, r7
    mov.l   @r15+, r6
    mov.l   @r15+, r5
    mov.l   @r15+, r4
    mov.l   @r15+, r3
!   mov.l   @r15+, r2
!   mov.l   @r15+, r1
	
    mov.l   _exception_os_type, r1
    mov #2, r2
    cmp/eq  r2, r1
    bt katana_entry
    mov #3, r2
    cmp/eq  r2, r1
    bt wince_entry

kos_entry:
    mov.l   @r15+, r2
    mov.l   @r15+, r1
    mov.l   @r15+, r4  ! Get the exception code
    jmp     @r0
    mov.l   @r15+, r0

katana_entry:
    mov.l   @r15+, r2
    mov.l   @r15+, r1
    add     #4, r15    ! Skip the exception code
    jmp     @r0
    mov.l   @r15+, r0

wince_entry:
    add     #8, r15    ! Skip restore r1 and r2
    mov     r0, r3
    mov.l   @r15+, r2  ! Get the exception code
    mov     #1, r1
    cmp/eq  r2, r1
    bf wince_int

wince_gen:
    mov.l   @(0x0C,r7), r1
    mov.l   @(8,r7), r0
    cmp/pz  r1
    bra wince_jmp
    nop

wince_int:
    mov.l   @(0x28,r7), r6
    mov.l   wince_interrupt_handler_vector_offset, r2
    stc     vbr, r0
    add     r2, r0
    mov.l   @r0, r0
    mov     r6, r1

wince_jmp:
    jmp     @r3
    stc     sgr, r15

!END REGISTER RESTORE

	.align 4

wince_interrupt_handler_vector_offset:
    .long   0x0000068c
_exception_os_type:
    .long   0
	
	.align 4

#ifdef USE_GDB
_general_sub_handler:
general_exception_handler:
    nop
    mov.l   r0, @-r15
    mov     #1, r0
    mov.l   r0, @-r15
    mov.l   general_handler, r0
    jmp     @r0
    nop

    .align  4

general_handler:
    .long   general_exception_xt
_general_sub_handler_base:
    nop
    bra     general_exception_handler
    nop
_general_sub_handler_end:
#endif

_my_exception_finish:
    stc     sgr, r15    ! Recover in case of missing stack (paranoia)
    rte
    nop

    .align  4

hdl_except:
    .long   _exception_handler

!
! HANDLE CACHE EXCEPTIONS
!
!
!_cache_sub_handler:
!cache_exception_handler:
!    nop
!    mov.l   r0, @-r15
!    mov     #2, r0
!    mov.l   r0, @-r15
!    mov.l   cache_handler, r0
!    jmp     @r0
!    nop
!
!    .align  4
!
!cache_handler:
!    .long   general_exception_xt
!
!_cache_sub_handler_base:
!    nop
!    bra     cache_exception_handler
!    nop
!_cache_sub_handler_end:

!
! HANDLE INTERRUPT EXCEPTIONS
!

_interrupt_sub_handler:
interrupt_exception_handler:
    mov.l   interrupt_stack, r15 ! Only for WinCE, for other replaced by nop
    mov.l   r0, @-r15
    mov     #3, r0
    mov.l   r0, @-r15
    mov.l   interrupt_handler, r0
    jmp     @r0
    nop

    .align  4

interrupt_handler:
    .long   general_exception_xt
_interrupt_stack:
interrupt_stack:
	.long 0x8c011000 ! Only for WinCE

_interrupt_sub_handler_base:
    nop
    bra     interrupt_exception_handler
    nop
_interrupt_sub_handler_end:


!
! Wait enough instructions for the UBC to be refreshed
!
#ifdef USE_GDB
_ubc_wait:
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
    rts
    nop
#endif
