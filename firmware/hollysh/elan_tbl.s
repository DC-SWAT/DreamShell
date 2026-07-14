!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// ELAN register init table for NAOMI 2 only
!//
!// Copyright 2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .align 4
.elan_tbl:
    .long 0xA8800010, 0x00000000
    .long 0xA8800014, 0x00002029
    .long 0xA8800018, 0x0000001F
    .long 0xA880001C, 0x87320961
    .long 0xA8800030, 0x00000000
    .long 0xA8800068, 0x80001290
    .long 0xA8800074, 0x0000003C
    .long 0xA8800078, 0x0000000C
    .long 0xA8800080, 0x00000004
    .long 0xA8800084, 0x00000008
    .long 0xA8800090, 0x0000013E
    .long 0xA8800094, 0x32000000
    .long 0xA8800098, 0x00010000
    .long 0xA88000A0, 0xC02A0028
    .long 0xA88000A4, 0x00008088
    .long 0xA88000D0, 0x00000006
.elan_tbl_end:
