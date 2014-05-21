
#ifndef AICA_REGISTERS_H
#define AICA_REGISTERS_H

#define REG_BUS_REQUEST      0x00802808

#define REG_ARM_INT_ENABLE   0x0080289c
#define REG_ARM_INT_SEND     0x008028a0
#define REG_ARM_INT_RESET    0x008028a4

#define REG_ARM_FIQ_BIT_0    0x008028a8
#define REG_ARM_FIQ_BIT_1    0x008028ac
#define REG_ARM_FIQ_BIT_2    0x008028b0

#define REG_SH4_INT_ENABLE   0x008028b4
#define REG_SH4_INT_SEND     0x008028b8
#define REG_SH4_INT_RESET    0x008028bc

#define REG_ARM_FIQ_CODE     0x00802d00
#define REG_ARM_FIQ_ACK      0x00802d04

/* That value is required on several registers */
#define MAGIC_CODE 0x20

/* Interruption codes */
#define TIMER_INTERRUPT_INT_CODE 2
#define BUS_REQUEST_INT_CODE 5
#define SH4_INTERRUPT_INT_CODE 6

#define AICA_FROM_SH4(x) ((x) + 0x9ff00000)

#endif
