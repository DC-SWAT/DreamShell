/**
 * DreamShell ISO Loader
 * SH4 instruction encodings
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef __SH4_OPCODE_H__
#define __SH4_OPCODE_H__

#define SH4_OPCODE_NOP                 0x0009
#define SH4_OPCODE_RTS                 0x000b
#define SH4_OPCODE_JMP_R0              0x402b

#define SH4_OPCODE_MOV_IMM_RN(rn, imm) (0xe000 | (((rn) & 0x0f) << 8) | ((imm) & 0xff))
#define SH4_OPCODE_MOV_0_R0            SH4_OPCODE_MOV_IMM_RN(0, 0)

#define SH4_OPCODE_MOVL_PC_RN(rn)      (0xd000 | (((rn) & 0x0f) << 8))
#define SH4_OPCODE_MOVL_PC_MASK        0xff00
#define SH4_OPCODE_MOVL_PC             0xd000
#define SH4_OPCODE_NIBBLE_MASK         0xf000
#define SH4_OPCODE_MOVL_R0_PC(disp)    (SH4_OPCODE_MOVL_PC_RN(0) | ((disp) >> 2))

#define SH4_OPCODE_BRA_MASK            0xf000
#define SH4_OPCODE_BRA                 0xa000
#define SH4_BRA_DISP_MASK              0x0fff
#define SH4_BRA_DISP_SIGN              0x0800
#define SH4_BRA_PC_BIAS                4

#endif
