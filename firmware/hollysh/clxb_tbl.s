!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// CLXB register init table for NAOMI 2 only
!//
!// Copyright 2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .align 4
    .globl _clxb_tbl
    .globl _clxb_tbl_end
.clxb_tbl:
_clxb_tbl:
    .long 0xA25F6900, 0xFFFFFFFF
    .long 0xA25F6908, 0xFFFFFFFF
.clxb_tbl_end:
_clxb_tbl_end:
