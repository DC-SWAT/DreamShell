!   This file is part of DreamShell ISO Loader
!   Copyright (C)2009-2016 SWAT
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
	.globl _gdc_syscall_enable
	.globl _gdc_syscall_disable
	.globl _gdc_syscall_save
	.globl _gdc_saved_vector
	.globl _gdc_redir
	.globl _lock_gdsys
	.globl _unlock_gdsys
	.globl _gdc_lock_state
.align 2
	
_gdc_syscall_save:
	mov.l   gdc_saved_k, r0
	mov.l   @r0, r0
	tst     r0, r0
	bf      already_saved
	mov.l   gdc_entry_c0_k, r0
	mov.l   @r0, r0
	mov.l   gdc_saved_k, r1
	mov.l   r0, @r1
already_saved:
	rts
	nop
	
_gdc_syscall_disable:
	mov.l   gdc_saved_k, r0
	mov.l   @r0, r0
	mov.l   gdc_entry_bc_k, r1
	mov.l   r0, @r1
	rts
	nop

_gdc_syscall_enable:
	mov.l   gdc_entry_bc_k, r0
	mov.l   gdc_redir_bc_k, r1
	mov.l   r1, @r0
	mov.l   gdc_entry_c0_k, r0
	mov.l   gdc_redir_c0_k, r1
	mov.l   r1, @r0
	rts
	nop

_lock_gdsys:
	mova    gdc_lock, r0
	tas.b   @r0
	bt      lock_success
	rts
	mov     #1, r0
lock_success:
	rts
	mov     #0, r0
	
_unlock_gdsys:
	mova    gdc_lock, r0
	mov     #0, r2
	rts
	mov.l   r2, @r0

gdcExecServerInternal:
	mova    gdc_lock, r0
	tas.b   @r0
	bt      process_exec_server
	rts
	nop
process_exec_server:
	mova    saved_regs, r0
	add     #40, r0
	sts.l   pr, @-r0
	sts.l   mach, @-r0
	sts.l   macl, @-r0
	mov.l   r14, @-r0
	mov.l   r13, @-r0
	mov.l   r12, @-r0
	mov.l   r11, @-r0
	mov.l   r10, @-r0
	mov.l   r9, @-r0
	mov.l   r8, @-r0
	mov.l   gdcExecServerExternal, r1
	jsr     @r1
	nop
	mova    saved_regs, r0
	mov.l   @r0+, r8
	mov.l   @r0+, r9
	mov.l   @r0+, r10
	mov.l   @r0+, r11
	mov.l   @r0+, r12
	mov.l   @r0+, r13
	mov.l   @r0+, r14
	lds.l   @r0+, macl
	lds.l   @r0+, mach
	lds.l   @r0+, pr
	mova    gdc_lock, r0
	mov     #0, r2
	rts
	mov.l   r2, @r0
	
gdcInitSystemInternal:
	mova    gdc_lock, r0
	mov     #1, r2
	mov.l   r2, @r0
	mova    saved_regs, r0
	add     #40, r0
	sts.l   pr, @-r0
	sts.l   mach, @-r0
	sts.l   macl, @-r0
	mov.l   r14, @-r0
	mov.l   r13, @-r0
	mov.l   r12, @-r0
	mov.l   r11, @-r0
	mov.l   r10, @-r0
	mov.l   r9, @-r0
	mov.l   r8, @-r0
	mov.l   gdcInitSystemExternal, r1
	jsr     @r1
	nop
	mova    saved_regs, r0
	mov.l   @r0+, r8
	mov.l   @r0+, r9
	mov.l   @r0+, r10
	mov.l   @r0+, r11
	mov.l   @r0+, r12
	mov.l   @r0+, r13
	mov.l   @r0+, r14
	lds.l   @r0+, macl
	lds.l   @r0+, mach
	lds.l   @r0+, pr
	mova    gdc_lock, r0
	mov     #0, r2
	rts
	mov.l   r2, @r0

gdc_redir_bc:
	mov     #-1, r1
	cmp/eq  r6, r1
	bt/s    nop_syscall  ! Skip misc syscalls
	mov     #0, r6
gdc_redir_c0:
	mov     r7, r0
	mov     #16, r1
	cmp/hi  r0, r1
	bf      bad_syscall
	mov.l   gdc_syscall, r1
	shll2   r0
	mov.l   @(r0, r1), r0
	jmp     @r0
	nop
bad_syscall:
	mov     #-1, r6
nop_syscall:
	rts
	mov     r6, r0

	.align 4
gdcExecServerExternal:
	.long _gdcExecServer
gdcInitSystemExternal:
	.long _gdcInitSystem
_gdc_lock_state:
gdc_lock:
	.long 0
gdc_entry_bc_k:
	.long 0xac0000bc
gdc_entry_c0_k:
	.long 0xac0000c0
gdc_saved_k:
	.long _gdc_saved_vector
_gdc_saved_vector:
	.long 0
gdc_redir_bc_k:
	.long gdc_redir_bc
gdc_redir_c0_k:
	.long gdc_redir_c0
_gdc_redir:
	.long gdc_redir_bc
saved_regs:
	.long 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	
gdc_syscall:
	.long gdcReqCmd
gdcReqCmd:
	.long _gdcReqCmd
gdcGetCmdStat:
	.long _gdcGetCmdStat
gdcExecServer:
	.long gdcExecServerInternal
gdcInitSystem:
	.long gdcInitSystemInternal
gdcGetDrvStat:
	.long _gdcGetDrvStat
gdcG1DmaEnd:
	.long _gdcG1DmaEnd
gdcReqDmaTrans:
	.long _gdcReqDmaTrans
gdcCheckDmaTrans:
	.long _gdcCheckDmaTrans
gdcReadAbort:
	.long _gdcReadAbort
gdcReset:
	.long _gdcReset
gdcChangeDataType:
	.long _gdcChangeDataType
gdcSetPioCallback:
	.long _gdcSetPioCallback
gdcReqPioTrans:
	.long _gdcReqPioTrans
gdcCheckPioTrans:
	.long _gdcCheckPioTrans
gdcUnk1:
	.long _gdcDummy
gdcUnk2:
	.long _gdcDummy
gdcUnk3:
	.long _gdcDummy
gdcUnk4:
	.long _gdcDummy
