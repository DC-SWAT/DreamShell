!///////////////////////////////////////////////////////////////////////////////
!// DreamShell HollySH BIOS firmware
!//
!// BIOS hooks
!//
!// Copyright 2026 by SWAT
!//
!///////////////////////////////////////////////////////////////////////////////
    .globl _bios_reset
    .globl _bios_reset_install
    .globl _bios_test
    .globl _bios_game
    .text

_bios_reset:
    mov.l RESET_P2_ADDR, r0
    mov.l PHYS_MASK, r1
    and r1, r0
    mov.l P2_MASK, r1
    or r1, r0
    jmp @r0
    nop

_bios_reset_p2:
    mov.l RESET_INSTALL, r0
    mov.l P2_MASK, r1
    or r1, r0
    jsr @r0
    nop
    mov.l RESET_DST, r0
    jmp @r0
    nop

_bios_reset_install:
    sts.l pr, @-r15
    mov.l RESET_DST, r2
    mov.l BLOB_SRC, r0
    mov.l P2_MASK, r1
    or r1, r0
    mov.l BLOB_END, r3
    or r1, r3
.copy_blob:
    cmp/hs r3, r0
    bt .copy_blob_done
    mov.l @r0+, r4
    mov.l r4, @r2
    bra .copy_blob
    add #4, r2
.copy_blob_done:
    mov.l ELAN_MAGIC_ADDR, r0
    mov.l @r0, r0
    mov.l ELAN_MAGIC_VAL, r1
    cmp/eq r0, r1
    bf .copy_clxa
    mov.l ELAN_SRC, r0
    mov.l ELAN_END, r3
    bsr .copy_tbl
    nop
    mov.l CLXB_SRC, r0
    mov.l CLXB_END, r3
    bsr .copy_tbl
    nop
    mov.l CLXC_SRC, r0
    mov.l CLXC_END, r3
    bsr .copy_tbl
    nop
.copy_clxa:
    mov.l CLXA_SRC, r0
    mov.l CLXA_END, r3
    bsr .copy_tbl
    nop
    xor r0, r0
    mov.l r0, @r2
    add #4, r2
    mov.l r0, @r2
    lds.l @r15+, pr
    rts
    nop

.copy_tbl:
    mov.l P2_MASK, r1
    or r1, r0
    or r1, r3
.copy_tbl_loop:
    cmp/eq r0, r3
    bt .copy_tbl_done
    mov.l @r0+, r4
    mov.l r4, @r2
    bra .copy_tbl_loop
    add #4, r2
.copy_tbl_done:
    rts
    nop

_bios_test:
    mov.l TEST_P2_ADDR, r0
    mov.l PHYS_MASK, r1
    and r1, r0
    mov.l P2_MASK, r1
    or r1, r0
    jmp @r0
    nop

_bios_game:
    mov.l GAME_P2_ADDR, r0
    mov.l PHYS_MASK, r1
    and r1, r0
    mov.l P2_MASK, r1
    or r1, r0
    jmp @r0
    nop

_bios_test_p2:
    mov.l TEST_ENTER_ADR, r4
    bra .bios_enter_p2
    nop

_bios_game_p2:
    mov.l GAME_ENTER_ADR, r4
    bra .bios_enter_p2
    nop

.bios_enter_p2:
    mov.l TEST_CCR_ADDR, r0
    mov.l TEST_CCR_VAL, r1
    mov.l r1, @r0
    mov.l TEST_SR_VAL, r0
    ldc r0, sr
    ldc r0, ssr
    mov.l TEST_STACK, r15
    mov.l TEST_GBR_VAL, r0
    ldc r0, gbr
    mov.l @r4, r0
    tst r0, r0
    bt .enter_fallback
    jmp @r0
    nop
.enter_fallback:
    mov.l TEST_RESET, r0
    jmp @r0
    nop

    .align 4
RESET_P2_ADDR:
    .long _bios_reset_p2
PHYS_MASK:
    .long 0x1FFFFFFF
RESET_INSTALL:
    .long _bios_reset_install
RESET_DST:
    .long 0xAC018000
BLOB_SRC:
    .long _bios_reset_blob
BLOB_END:
    .long _bios_reset_blob_end
P2_MASK:
    .long 0xA0000000
ELAN_MAGIC_ADDR:
    .long 0xA8800000
ELAN_MAGIC_VAL:
    .long 0xE1AD0000
ELAN_SRC:
    .long _elan_tbl
ELAN_END:
    .long _elan_tbl_end
CLXB_SRC:
    .long _clxb_tbl
CLXB_END:
    .long _clxb_tbl_end
CLXC_SRC:
    .long _clxc_tbl
CLXC_END:
    .long _clxc_tbl_end
CLXA_SRC:
    .long _clxa_tbl
CLXA_END:
    .long _clxa_tbl_end
TEST_P2_ADDR:
    .long _bios_test_p2
GAME_P2_ADDR:
    .long _bios_game_p2
TEST_CCR_ADDR:
    .long 0xFF00001C
TEST_CCR_VAL:
    .long 0x00000909
TEST_SR_VAL:
    .long 0x700000F0
TEST_STACK:
    .long 0x8D000000
TEST_GBR_VAL:
    .long 0x8C000000
TEST_ENTER_ADR:
    .long 0xAC017FFC
GAME_ENTER_ADR:
    .long 0xAC017FF8
TEST_RESET:
    .long _bios_reset

    .align 4
_bios_reset_blob:
    mov.l blob_ccr_addr, r0
    mov.l blob_ccr_val, r1
    mov.l r1, @r0
    stc sr, r0
    mov.w blob_sr_and, r1
    mov.l blob_sr_or, r2
    and r1, r0
    or r2, r0
    ldc r0, sr
    mov.l blob_hw_type, r0
    mov.l @r0, r3
    mov.l blob_sfres_val, r4
    tst r3, r3
    bt .blob_holly_sfres
    mov.l blob_g1_sfres, r1
    mov.l r4, @r1
.blob_holly_sfres:
    mov.l blob_holly_sfres, r2
    mov.l r4, @r2
    mov.l blob_delay, r1
.blob_delay_loop:
    dt r1
    bf .blob_delay_loop
    mova reset_tbl, r0
    mov r0, r1
.blob_tbl_loop:
    mov.l @r1+, r0
    cmp/eq #0, r0
    bt .blob_tbl_done
    mov.l @r1+, r2
    bra .blob_tbl_loop
    mov.l r2, @r0
.blob_tbl_done:
    mov.l blob_bios_entry, r0
    jmp @r0
    nop

    .align 4
blob_ccr_addr:
    .long 0xFF00001C
blob_ccr_val:
    .long 0x00000808
blob_sr_or:
    .long 0x100000F0
blob_hw_type:
    .long 0xA05F74B0
blob_sfres_val:
    .long 0x00007611
blob_g1_sfres:
    .long 0xA07F6890
blob_holly_sfres:
    .long 0xA05F6890
blob_delay:
    .long 0x00002000
blob_bios_entry:
    .long 0xA0000000
blob_sr_and:
    .word 0xFF0F
    .word 0x0000
    .align 4
reset_tbl:
_bios_reset_blob_end:
