/* KallistiOS ##version##

   g2bus.h
   (c)2002 Dan Potter
   
   $Id: g2bus.h,v 1.5 2002/09/13 06:44:41 bardtx Exp $
*/

#ifndef __DC_G2BUS_H
#define __DC_G2BUS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* Read one byte from G2 */
uint8 g2_read_8(uint32 address);

/* Write one byte to G2 */
void g2_write_8(uint32 address, uint8 value);

/* Read one word from G2 */
uint16 g2_read_16(uint32 address);

/* Write one word to G2 */
void g2_write_16(uint32 address, uint16 value);

/* Read one dword from G2 */
uint32 g2_read_32(uint32 address);

/* Write one dword to G2 */
void g2_write_32(uint32 address, uint32 value);

/* Read a block of 8-bit values from G2 */
void g2_read_block_8(uint8 * output, uint32 address, int amt);

/* Write a block 8-bit values to G2 */
void g2_write_block_8(const uint8 * input, uint32 address, int amt);

/* Read a block of 16-bit values from G2 */
void g2_read_block_16(uint16 * output, uint32 address, int amt);

/* Write a block of 16-bit values to G2 */
void g2_write_block_16(const uint16 * input, uint32 address, int amt);

/* Read a block of 32-bit values from G2 */
void g2_read_block_32(uint32 * output, uint32 address, int amt);

/* Write a block of 32-bit values to G2 */
void g2_write_block_32(const uint32 * input, uint32 address, int amt);

/* When writing to the SPU RAM, this is required at least every 8 32-bit
   writes that you execute */
void g2_fifo_wait();

__END_DECLS

#endif	/* __DC_G2BUS_H */

