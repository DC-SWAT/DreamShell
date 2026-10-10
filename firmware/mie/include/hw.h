/**
 * DreamShell MIE firmware
 * Ports, Maple registers and RAM layout
 * (c)2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef MIE_HW_H
#define MIE_HW_H

#include <stdint.h>

#define MAPLE_BASE          0x7000
#define MAPLE_CTRL          (*(volatile uint8_t *)(MAPLE_BASE + 0))
#define MAPLE_STAT          (*(volatile uint8_t *)(MAPLE_BASE + 1))
#define MAPLE_LREG          (*(volatile uint8_t *)(MAPLE_BASE + 3))
#define MAPLE_BUF           ((volatile uint8_t *)(MAPLE_BASE + 4))
#define MAPLE_PAYLOAD       ((volatile uint8_t *)(MAPLE_BASE + 8))
#define MAPLE_FRAME         32

#define MAPLE_CTRL_TXB      0x01
#define MAPLE_CTRL_CTXB     0x02
#define MAPLE_CTRL_ENDP     0x04

#define MAPLE_STAT_EMP      0x01
#define MAPLE_STAT_PERR     0x02
#define MAPLE_STAT_BFOV     0x08
#define MAPLE_STAT_RXB      0x10
#define MAPLE_STAT_RFB      0x20
#define MAPLE_STAT_TFB      0x40
#define MAPLE_STAT_HRES     0x80

#define MAPLE_ADDR          0x20
#define MAPLE_CMD_DEVINFO   0x01
#define MAPLE_CMD_RESET     0x03
#define MAPLE_CMD_GETCOND   0x09
#define MAPLE_CMD_UPLOAD    0x80
#define MAPLE_CMD_VERSION   0x82
#define MAPLE_CMD_SELFTEST  0x84
#define MAPLE_CMD_IO        0x86
#define MAPLE_IO_PAD        0x51
#define MAPLE_IO_PAD_ACK    0x52

#define MAPLE_RESP_DEVINFO  0x05
#define MAPLE_RESP_OK       0x07
#define MAPLE_RESP_DATATRF  0x08
#define MAPLE_RESP_BADFUNC  0xFE
#define MAPLE_RESP_UPLOAD   0x80
#define MAPLE_RESP_VERSION  0x83
#define MAPLE_RESP_SELFTEST 0x85
#define MAPLE_RESP_IO       0x87
#define MAPLE_RESP_AGAIN    0xFC
#define MAPLE_RESP_BADCMD   0xFD

#define MAPLE_SPIN          0xFF
#define UPLOAD_COPY         0x1A
#define UPLOAD_SUM          0x1C
#define UPLOAD_EXEC         0xFF
#define REPLY_ONE_CHUNK     0x1D
#define REPLY_FIRST         0x1C
#define REPLY_NEXT          0x20
#define REPLY_NEXT_LIMIT    0x21
#define VERSION_FRAME       32
#define SELFTEST_COPIES     4

#define IRQ_ENABLE_MAPLE    0x04
#define IRQ_ENABLE_JVS      0x08
#define IRQ_ENABLE_SERIAL   0x20
#define IRQ_LEVEL_MAPLE     2
#define IRQ_VECTOR_PAGE     0x9F
#define IRQ_VECTORS         0x9FF2
#define IRQ_VECTOR_BYTES    14

#define GPIO_DIR_RUN        0x77
#define GPIO_OUT_MASK       0x0F
#define PORT40_OFF          0x00
#define PORT40_ON           0x0F

#define NODE_BASE           0xC000
#define NODE_STRIDE         0x100
#define NODE_SLOTS          0x20
#define JVS_NODE_MASK       0x1F

#define EEPROM_SIZE         128
#define SERIAL_BUF          0x400

#define JVS_SYNC            0xE0
#define JVS_MARK            0xD0
#define JVS_RX_SPIN         0xC8
#define JVS_SENSE_TRIES     0x7F
#define JVS_SENSE_POLLS     6
#define UART_DIVISOR        8
#define UART_LCR_DLAB       0x83
#define UART_LCR_8N1        0x03
#define UART_FCR_ENABLE     0x01
#define UART_FCR_TX         0x02
#define UART_LSR_DR         0x01
#define UART_LSR_THRE       0x20
#define UART_LSR_TEMT       0x40
#define JVS_TX_OK           0x00
#define JVS_CMD_SWINP       0x20
#define JVS_CMD_ANLINP      0x22
#define JVS_STATUS_OK       0x01
#define JVS_REPORT_OK       0x01
#define JVS_PAD_NODE        0x01
#define JVS_PAD_PLAYERS     0x02
#define JVS_PAD_SW_BYTES    0x02
#define JVS_PAD_ANALOG      0x03
#define JVS_PAD_REQ_LEN     5
#define JVS_PAD_RX          32
#define JVS_RX_FRAME        3
#define JVS_SYS_TEST        0x80
#define JVS_HI_START        0x80
#define JVS_HI_SERVICE      0x40
#define JVS_HI_UP           0x20
#define JVS_HI_DOWN         0x10
#define JVS_HI_LEFT         0x08
#define JVS_HI_RIGHT        0x04
#define JVS_HI_B            0x02
#define JVS_HI_X            0x01
#define JVS_LO_A            0x80
#define JVS_LO_Y            0x20

#define PAD_MODE_OFF        0x00
#define PAD_MODE_ON         0x01
#define PAD_COND_BYTES      8
#define PAD_COND_WORDS      3
#define PAD_DEVINFO_LEN     112
#define PAD_AXIS_CENTER     0x80
#define PAD_ANALOG_IDLE     0x0500
#define PAD_ANALOG_MASK     0xFF7F
#define PAD_WHEEL_NEG       128
#define PAD_WHEEL_POS       127
#define PAD_PEDAL_MAX       255
#define PAD_CAL_COUNT       9
#define PAD_CAL_BYTES       18
#define PAD_PAY_MODE        1
#define PAD_PAY_CAL         2
#define PAD_PAY_DATA        4
#define PAD_CAL_WORDS       6
#define PAD_CAL_WHEEL_MIN   0
#define PAD_CAL_WHEEL_MID   1
#define PAD_CAL_WHEEL_MAX   2
#define PAD_CAL_ACCEL_MIN   3
#define PAD_CAL_ACCEL_MAX   5
#define PAD_CAL_BRAKE_MIN   6
#define PAD_CAL_BRAKE_MAX   8
#define PAD_BTN_C           0x0001
#define PAD_BTN_B           0x0002
#define PAD_BTN_A           0x0004
#define PAD_BTN_START       0x0008
#define PAD_BTN_UP          0x0010
#define PAD_BTN_DOWN        0x0020
#define PAD_BTN_LEFT        0x0040
#define PAD_BTN_RIGHT       0x0080
#define PAD_BTN_Z           0x0100
#define PAD_BTN_Y           0x0200
#define PAD_BTN_X           0x0400
#define PAD_BTN_IDLE        0xFFFF
#define PAD_FUNC_LO         0x00
#define PAD_FUNC_HI         0x01
#define PAD_COND_RTRIG      2
#define PAD_COND_LTRIG      3
#define PAD_COND_JOYX       4
#define PAD_COND_JOYY       5
#define PAD_COND_JOY2X      6
#define PAD_COND_JOY2Y      7
#define PAD_JVS_STATUS      3
#define PAD_JVS_SW_ACK      4
#define PAD_JVS_SYS         5
#define PAD_JVS_P1          6
#define PAD_JVS_P2          8
#define PAD_JVS_AN_ACK      10
#define PAD_JVS_ANALOG      11
#define PAD_JVS_MIN         18

#define JVS_RX_OK           0x00
#define JVS_RX_TIMEOUT      0x02
#define JVS_RX_DEST         0x03
#define JVS_RX_SUM          0x04
#define JVS_RX_NONE         0x05

#define JVS_SLOT_LEN        0x00
#define JVS_SLOT_TAG        0x01
#define JVS_SLOT_GPIO       0x02
#define JVS_SLOT_NODE       0x12
#define JVS_SLOT_SENSE      0x13
#define JVS_SLOT_SAVED      0x14
#define JVS_SLOT_STATUS     0x15
#define JVS_SLOT_STORED     0x16
#define JVS_SLOT_PKT        0x17
#define JVS_SLOT_TAG_VALUE  0x16
#define JVS_LEN_BIAS        7
#define JVS_FETCH_ADD       0x10
#define JVS_FETCH_EMPTY     0x14
#define JVS_REPLY_SUB       0x18
#define JVS_BATCH_SUB       0x26
#define JVS_SIDE_SUB        0x38
#define MACRO_BASE          0xAC00
#define SIDE_BASE           0xB100
#define SAVED_BASE          0xAA00
#define MACRO_INDEX_MAX     0x20
#define MACRO_COPY_MAX      0x11
#define SIDE_COPY_MAX       0x1A
#define JVS_FRAME_GAP       0x5A
#define GPIO_SAMPLE_PORTS   7
#define GPIO_SAMPLE_ADC     8

#define EE_CLK              0x20
#define EE_STROBE           0x40
#define EE_BIT              0x10
#define EE_DIN              0x80
#define EE_ADDR_BIT         0x40
#define EE_ADDR_BITS        7
#define EE_DATA_BITS        8
#define EE_TAIL             10

__sfr __at 0x00 gpio0;
__sfr __at 0x01 gpio1;
__sfr __at 0x02 gpio2;
__sfr __at 0x03 gpio3;
__sfr __at 0x04 gpio4;
__sfr __at 0x05 gpio5;
__sfr __at 0x06 gpio6;
__sfr __at 0x08 gpio_dir;
__sfr __at 0x09 port_09;
__sfr __at 0x0A port_0a;
__sfr __at 0x0B port_0b;
__sfr __at 0x0C port_0c;
__sfr __at 0x0D port_0d;
__sfr __at 0x0F adc;
__sfr __at 0x10 uart_data;
__sfr __at 0x11 uart_ier;
__sfr __at 0x12 uart_fcr;
__sfr __at 0x13 uart_lcr;
__sfr __at 0x14 uart_mcr;
__sfr __at 0x15 uart_lsr;
__sfr __at 0x30 irq_enable;
__sfr __at 0x40 port_40;
__sfr __at 0x50 maple_irq_level;
__sfr __at 0x70 irq_pending;
__sfr __at 0x80 port_80;
__sfr __at 0x81 port_81;
__sfr __at 0x90 jvs_control;
__sfr __at 0x91 jvs_sense;

extern uint8_t reply_buf[MAPLE_FRAME];
extern uint8_t jvs_state;
extern uint8_t jvs_node_id;
extern uint8_t jvs_mark;
extern uint8_t jvs_saved;
extern uint8_t active_flag;
extern uint8_t pkt_flag;
extern uint8_t jvs_frame[];
extern uint8_t eeprom_buf[EEPROM_SIZE];
extern uint8_t eeprom_wr[EEPROM_SIZE];
extern uint8_t serial_state;
extern uint8_t serial_mark;
extern uint8_t serial_latch;
extern uint16_t serial_pos;
extern uint16_t serial_ptr;
extern uint16_t serial_limit;
extern uint8_t serial_buf_b[SERIAL_BUF];
extern uint8_t serial_buf_c[SERIAL_BUF];
extern volatile uint8_t selftest_byte;

void mie_di(void);
void mie_ei(void);
void mie_reboot(void);
void mie_rom(void);

void mie_zero(uint8_t *dst, uint16_t n);
void mie_copy(uint8_t *dst, const uint8_t *src, uint16_t n);

#endif
