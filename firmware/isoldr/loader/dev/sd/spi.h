/* DreamShell ##version##

   sd/spi.h
   Copyright (C) 2011-2016 SWAT
*/

#ifndef _SPI_H
#define _SPI_H

#include <arch/types.h>
#include <arch/timer.h>

/**
 * \file
 * SPI interface for Sega Dreamcast SCIF
 *
 * \author SWAT
 */

/**
 * \brief Initializes the SPI interface
 */
int spi_init();

/**
 * \brief Shutdown the SPI interface
 */
int spi_shutdown();

/**
 * \brief Switch on CS pin and lock SPI bus
 *
 * \param[in] the CS pin
 */
void spi_cs_on();

/**
 * \brief Switch off CS pin and unlock SPI bus
 *
 * \param[in] the CS pin
 */
void spi_cs_off();

/**
 * \brief Sends a byte over the SPI bus.
 *
 * \param[in] b The byte to send.
 */

void spi_send_byte(register uint8 b);

/**
 * \brief Receives a byte from the SPI bus.
 *
 * \returns The received byte.
 */
uint8 spi_rec_byte();

/**
 * \brief Send and receive a byte to/from the SPI bus.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_sr_byte(register uint8 b);

/**
 * \brief Slow send and receive a byte to/from the SPI bus. 
 * Used for SDC/MMC commands.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_slow_sr_byte(register uint8 b);

/**
 * \brief Sends data contained in a buffer over the SPI bus.
 *
 * \param[in] data A pointer to the buffer which contains the data to send.
 * \param[in] len The number of bytes to send.
 */
void spi_send_data(const uint8* data, uint16 data_len);
void spi_send_data_fast(const uint8* data, uint16 data_len);

/**
 * \brief Receives multiple bytes from the SPI bus and writes them to a buffer.
 *
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] len The number of bytes to read.
 */
void spi_rec_data(uint8* buffer, uint16 buffer_len);
void spi_rec_data_fast(uint8* buffer, uint16 buffer_len);


#endif /* _SPI_H */
