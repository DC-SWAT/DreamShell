/* DreamShell ##version##

   drivers/sd.h
   Copyright (C) 2014 SWAT
*/

#ifndef _SD_H
#define _SD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/blockdev.h>

/**
 * \file
 * SD driver for Sega Dreamcast
 * Doxygen comments by Lawrence Sebald
 *
 * \author SWAT
 */

/** \brief  Initialize the SD card for use.

    This function initializes the SD card for first use. This includes all steps
    of the basic initialization sequence for SPI mode, as documented in the SD
    card spec and at http://elm-chan.org/docs/mmc/mmc_e.html . This also will
    call scif_sd_init() for you, so you don't have to worry about that ahead of
    time.

    \retval 0               On success.
    \retval -1              On failure. This could indicate any number of
                            problems, but probably means that no SD card was
                            detected.
*/
int sdc_init(void);

/** \brief  Shut down SD card support.

    This function shuts down SD card support, and cleans up anything that the
    sd_init() function set up.

    \retval 0               On success.
    \retval -1              On failure. The only currently defined error is if
                            the card was never initialized to start with.
*/
int sdc_shutdown(void);

/** \brief  Read one or more blocks from the SD card.

    This function reads the specified number of blocks from the SD card from the
    beginning block specified into the buffer passed in. It is your
    responsibility to allocate the buffer properly for the number of bytes that
    is to be read (512 * the number of blocks requested).

    \param  block           The starting block number to read from.
    \param  count           The number of 512 byte blocks of data to read.
    \param  buf             The buffer to read into.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
int sdc_read_blocks(uint32 block, size_t count, uint8 *buf);

/** \brief  Write one or more blocks to the SD card.

    This function writes the specified number of blocks to the SD card at the
    beginning block specified from the buffer passed in. Each block is 512 bytes
    in length, and you must write at least one block at a time. You cannot write
    partial blocks.

    If this function returns an error, you have quite possibly corrupted
    something on the card or have a damaged card in general (unless errno is
    ENXIO).

    \param  block           The starting block number to write to.
    \param  count           The number of 512 byte blocks of data to write.
    \param  buf             The buffer to write from.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
int sdc_write_blocks(uint32 block, size_t count, const uint8 *buf);

/** \brief  Retrieve the size of the SD card.

    This function reads the size of the SD card from the card's CSD register.
    This is the raw size of the card, not its formatted capacity. To get the
    number of blocks from this, divide by 512.

    \return                 On succes, the raw size of the SD card in bytes. On
                            error, (uint64)-1.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
uint64 sdc_get_size(void);

/** \brief  Print identification data of the SD card.

    This function reads the manufacturer info of the SD card from the card's CID register.

    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - SD card support was not initialized
*/
int sdc_print_ident(void);

/** \brief  Get a block device for a given partition on the SD card.

    This function creates a block device descriptor for the given partition on
    the attached SD card. This block device is used to interface with various
    filesystems on the device.

    \param  partition       The partition number (0-3) to use.
    \param  rv              Used to return the block device. Must be non-NULL.
    \param  partition_type  Used to return the partition type. Must be non-NULL.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     ENXIO - SD card support was not initialized \n
    \em     EIO - an I/O error occurred in reading data \n
    \em     EINVAL - invalid partition number was given \n
    \em     EFAULT - rv or partition_type was NULL \n
    \em     ENOENT - no MBR found \n
    \em     ENOENT - no partition at the specified position \n
    \em     ENOMEM - out of memory

    \note   This interface currently only supports MBR-formatted SD cards. There
            is currently no support for GPT partition tables.
*/
int sdc_blockdev_for_partition(int partition, kos_blockdev_t *rv,
                              uint8 *partition_type);

__END_DECLS

#endif /* _SD_H */
