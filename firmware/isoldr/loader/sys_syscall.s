!   This file is part of DreamShell ISO Loader
!   Copyright (C)2010-2016 SWAT
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
	.globl _sys_syscall_enable
	.globl _sys_syscall_disable
	.globl _sys_syscall_save
	.globl _sys_saved_vector
.align 2
	
_sys_syscall_save:
	mov.l sys_saved_k, r0
	mov.l @r0, r0
	tst r0,r0
	bf already_saved
	mov.l sys_entry_k, r0
	mov.l @r0,r0
	mov.l sys_saved_k, r1
	mov.l r0, @r1
already_saved:
	rts
	nop
	
_sys_syscall_disable:
	mov.l sys_saved_k, r0
	mov.l @r0, r0
	mov.l sys_entry_k, r1
	mov.l r0, @r1
	rts
	nop

_sys_syscall_enable:
	mov.l sys_entry_k, r0
	mov.l sys_redir_k, r1
	mov.l r1, @r0
	rts
	nop
	
.align 4
sys_entry_k:
	.long 0xac0000b0
sys_saved_k:
	.long _sys_saved_vector
_sys_saved_vector:
	.long 0
sys_redir_k:
	.long sys_redir

sys_redir:
	mov r7,r0
	mov #3,r1
	cmp/hi r0,r1
	bf badsyscall
	mov.l sys_syscall,r1
	shll2 r0
	mov.l @(r0,r1),r0
	jmp @r0
	nop
badsyscall:
	mov #-1,r0
	rts
	nop

sys_syscall:
	.long sys_misc_init
sys_misc_init:
	.long _sys_misc_init
sys_unknown:
	.long _sys_unknown
sys_icon:
	.long _sys_icon
sys_id:
	.long _sys_id
