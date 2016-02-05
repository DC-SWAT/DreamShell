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
	.globl _menu_syscall_enable
	.globl _menu_syscall_disable
	.globl _menu_syscall_save
	.globl _menu_saved_vector
.align 2
	
_menu_syscall_save:
	mov.l menu_saved_k, r0
	mov.l @r0, r0
	tst r0,r0
	bf already_saved
	mov.l menu_entry_k, r0
	mov.l @r0,r0
	mov.l menu_saved_k, r1
	mov.l r0, @r1
already_saved:
	rts
	nop
	
_menu_syscall_disable:
	mov.l menu_saved_k, r0
	mov.l @r0, r0
	mov.l menu_entry_k, r1
	mov.l r0, @r1
	rts
	nop

_menu_syscall_enable:
	mov.l menu_entry_k, r0
	mov.l menu_redir_k, r1
	mov.l r1, @r0
	rts
	nop
	
.align 4
menu_entry_k:
	.long 0xac0000e0
menu_saved_k:
	.long _menu_saved_vector
_menu_saved_vector:
	.long 0
menu_redir_k:
	.long menu_redir

menu_redir:
	mov r7,r0
	mov #2,r1
	cmp/hs r0,r1
	bf badsyscall
	mov.l menu_syscall,r1
	shll2 r0
	mov.l @(r0,r1),r0
	jmp @r0
	nop
badsyscall:
	mov #-1,r0
	rts
	nop

menu_syscall:
	.long menu_syscall
menu_syscall1:
	.long _menu_syscall
menu_syscall2:
	.long _menu_syscall
