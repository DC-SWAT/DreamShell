; DreamShell MIE firmware
; Reset entry, interrupt vectors and IM 2
; (c)2026 SWAT <http://www.dc-swat.ru>

.module crt0
.globl init
.globl _mie_start
.globl _mie_di
.globl _mie_ei
.globl _mie_reboot
.globl _mie_rom
.globl _maple_on_irq
.globl _jvs_on_irq
.globl _serial_on_irq
.globl s__DATA
.globl l__DATA
.globl s__INITIALIZED
.globl s__INITIALIZER
.globl l__INITIALIZER

.area _CODE

init:
    ld sp, #0x0000
    call gsinit
    ld a, #0x9f
    ld i, a
    im 2
    call _mie_start
    jp _mie_rom

gsinit:
    ld bc, #l__INITIALIZER
    ld a, b
    or c
    jr z, clear_bss
    ld de, #s__INITIALIZED
    ld hl, #s__INITIALIZER
    ldir
clear_bss:
    ld bc, #l__DATA
    ld a, b
    or c
    ret z
    ld hl, #s__DATA
    ld (hl), #0
    dec bc
    ld a, b
    or c
    ret z
    ld e, l
    ld d, h
    inc de
    ldir
    ret

_mie_di:
    di
    ret

_mie_ei:
    ei
    ret

_mie_reboot:
    jp init

_mie_rom:
    jp 0x0000

_isr_nop:
    nop
    ei
    reti

_isr_spin:
    jr _isr_spin

_isr_maple:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    call _maple_on_irq
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ei
    reti

_isr_jvs:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    call _jvs_on_irq
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ei
    reti

_isr_serial:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    call _serial_on_irq
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ei
    reti

.area _VECTORS
vec_tab:
    .dw _isr_nop
    .dw _isr_spin
    .dw _isr_maple
    .dw _isr_jvs
    .dw _isr_nop
    .dw _isr_serial
    .dw _isr_nop
