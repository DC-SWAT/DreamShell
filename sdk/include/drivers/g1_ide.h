/* DreamShell ##version##

   drivers/g1_ide.h
   Copyright (C) 2013-2014 SWAT
   
   Additional functions for G1-ATA devices
*/

#ifndef __G1_IDE_H
#define __G1_IDE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <dc/g1ata.h>


/** \brief  Retrieve the size of the ATA device.

    \return                 On succes, max LBA
                            error, (uint64)-1.
*/
uint64_t g1_ata_max_lba(void);


/** \brief  Send ATA device to standby mode.

    \retval 0               On success.
    \retval -1              On error
*/
int g1_ata_standby(void);


/** \brief  Checking for DCIO board.

    \retval 1               Is DCIO board
    \retval 0               Is NOT
    \retval -1              On error
*/
int g1_ata_is_dcio(void);


__END_DECLS
#endif /* !__G1_IDE_H */
