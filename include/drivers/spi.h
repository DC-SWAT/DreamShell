/* DreamShell ##version##

   drivers/spi.h
   Copyright (C) 2011-2014 SWAT
*/

#ifndef _SPI_H
#define _SPI_H

#include <arch/types.h>

/**
 * \file
 * SPI interface for Sega Dreamcast SCIF
 *
 * \author SWAT
 */


/**
 * \brief Max speed delay
 */
#define SPI_DEFAULT_DELAY		-1

/**
 * \brief Some predefined delays
 */
#define SPI_SDC_MMC_DELAY		SPI_DEFAULT_DELAY
#define SPI_ENC28J60_DELAY		1000000


/**
 * \brief Chip select pins
 */
#define SPI_CS_SCIF_RTS 	10
#define SPI_CS_GPIO_VGA 	8	// This is a real GPIO index
#define SPI_CS_GPIO_RGB 	9

#define SPI_CS_SDC          SPI_CS_SCIF_RTS
#define SPI_CS_ENC28J60     SPI_CS_GPIO_VGA
#define SPI_CS_SDC_2        SPI_CS_GPIO_RGB


/**
 * \brief Initializes the SPI interface
 */
int spi_init(int use_gpio);

/**
 * \brief Shutdown the SPI interface
 */
int spi_shutdown();

/**
 * \brief Change the SPI delay for clock
 *
 * \param[in] The SPI delay
 */
void spi_set_delay(int delay);

/**
 * \brief Getting current SPI delay
 *
 * \returns The SPI delay
 */
int spi_get_delay();

/**
 * \brief Switch on CS pin and lock SPI bus
 *
 * \param[in] the CS pin
 */
void spi_cs_on(int cs);

/**
 * \brief Switch off CS pin and unlock SPI bus
 *
 * \param[in] the CS pin
 */
void spi_cs_off(int cs);

/**
 * \brief Sends a byte over the SPI bus.
 *
 * \param[in] b The byte to send.
 */

void spi_send_byte(register uint8 b);
void spi_cc_send_byte(register uint8 b);

/**
 * \brief Receives a byte from the SPI bus.
 *
 * \returns The received byte.
 */
uint8 spi_rec_byte();
uint8 spi_cc_rec_byte();

/**
 * \brief Send and receive a byte to/from the SPI bus.
 *
 * \param[in] b The byte to send.
 * \returns The received byte.
 */
uint8 spi_sr_byte(register uint8 b);
uint8 spi_cc_sr_byte(register uint8 b);

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
void spi_cc_send_data(const uint8* data, uint16 data_len);

/**
 * \brief Receives multiple bytes from the SPI bus and writes them to a buffer.
 *
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] len The number of bytes to read.
 */
void spi_rec_data(uint8* buffer, uint16 buffer_len);
void spi_cc_rec_data(uint8* buffer, uint16 buffer_len);


#endif /* _SPI_H */
