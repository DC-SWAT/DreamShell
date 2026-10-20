; DreamShell MIE firmware
; Fixed RAM addresses
; (c)2026 SWAT <http://www.dc-swat.ru>

.module ram
.area _CODE

.globl _reply_buf
.globl _jvs_state
.globl _jvs_node_id
.globl _jvs_mark
.globl _jvs_saved
.globl _active_flag
.globl _pkt_flag
.globl _rumble_fx
.globl _rumble_seq
.globl _jvs_frame
.globl _eeprom_buf
.globl _eeprom_wr
.globl _serial_state
.globl _serial_mark
.globl _serial_latch
.globl _serial_pos
.globl _serial_ptr
.globl _serial_limit
.globl _serial_buf_b
.globl _serial_buf_c
.globl _selftest_byte

_reply_buf = 0xA200
_jvs_state = 0xA480
_jvs_node_id = 0xA482
_jvs_mark = 0xA484
_jvs_saved = 0xA4B0
_active_flag = 0xA4C8
_pkt_flag = 0xA4CE
_rumble_fx = 0xA4D0
_rumble_seq = 0xA4D4
_jvs_frame = 0xA800
_eeprom_buf = 0xB200
_eeprom_wr = 0xB300
_serial_state = 0xB411
_serial_mark = 0xB410
_serial_latch = 0xB412
_serial_pos = 0xB440
_serial_ptr = 0xB442
_serial_limit = 0xB444
_serial_buf_b = 0xB800
_serial_buf_c = 0xBC00
_selftest_byte = 0xF000
