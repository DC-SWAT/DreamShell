/* KallistiOS ##version##

   dc/cdrom.h
   (c)2000-2001 Dan Potter

*/

#ifndef __DC_CDROM_H
#define __DC_CDROM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/** \file   dc/cdrom.h
    \brief  CD access to the GD-ROM drive.

    This file contains the interface to the Dreamcast's GD-ROM drive. It is
    simply called cdrom.h and cdrom.c because, by design, you cannot directly
    use this code to read the high-density area of GD-ROMs. This is the way it
    always has been, and always will be.

    The way things are set up, as long as you're using fs_iso9660 to access the
    CD, it will automatically detect and react to disc changes for you.

    This file only facilitates reading raw sectors and doing other fairly low-
    level things with CDs. If you're looking for higher-level stuff, like normal
    file reading, consult with the stuff for the fs and for fs_iso9660.

    \author Dan Potter
    \see    kos/fs.h
    \see    dc/fs_iso9660.h
*/

/** \defgroup cd_cmd_codes          CD-ROM syscall command codes

    These are the syscall command codes used to actually do stuff with the
    GD-ROM drive. These were originally provided by thanks maiwe.

    @{
*/
#define CMD_PIOREAD 16  /**< \brief Read via PIO */
#define CMD_DMAREAD 17  /**< \brief Read via DMA */
#define CMD_GETTOC  18  /**< \brief Read TOC */
#define CMD_GETTOC2 19  /**< \brief Read TOC */
#define CMD_PLAY    20  /**< \brief Play track */
#define CMD_PLAY2   21  /**< \brief Play sectors */
#define CMD_PAUSE   22  /**< \brief Pause playback */
#define CMD_RELEASE 23  /**< \brief Resume from pause */
#define CMD_INIT    24  /**< \brief Initialize the drive */
#define CMD_SEEK    27  /**< \brief Seek to a new position */
#define CMD_READ    28  /**< \brief Read raw sectors */
#define CMD_STOP    33  /**< \brief Stop the disc from spinning */
#define CMD_GETSCD  34  /**< \brief Get subcode data */
#define CMD_GETSES  35  /**< \brief Get session */
/** @} */

/** \defgroup cd_cmd_response       CD-ROM command responses

    These are the values that the various functions can return as error codes.
    @{
*/
#define ERR_OK          0   /**< \brief No error */
#define ERR_NO_DISC     1   /**< \brief No disc in drive */
#define ERR_DISC_CHG    2   /**< \brief Disc changed, but not reinitted yet */
#define ERR_SYS         3   /**< \brief System error */
#define ERR_ABORTED     4   /**< \brief Command aborted */
#define ERR_NO_ACTIVE   5   /**< \brief System inactive? */
/** @} */

/** \defgroup cd_cmd_status         CD-ROM Command Status responses

    These are the raw values the status syscall returns.
    @{
*/
#define FAILED      -1  /**< \brief Command failed */
#define NO_ACTIVE   0   /**< \brief System inactive? */
#define PROCESSING  1   /**< \brief Processing command */
#define COMPLETED   2   /**< \brief Command completed successfully */
#define ABORTED     3   /**< \brief Command aborted before completion */
/** @} */

/** \defgroup cdda_read_modes       CDDA read modes

    Valid values to pass to the cdrom_cdda_play() function for the mode
    parameter.
    @{
*/
#define CDDA_TRACKS     1   /**< \brief Play by track number */
#define CDDA_SECTORS    2   /**< \brief Play by sector number */
/** @} */

/** \defgroup cd_status_values      CD-ROM status values

    These are the values that can be returned as the status parameter from the
    cdrom_get_status() function.
    @{
*/
#define CD_STATUS_BUSY      0   /**< \brief Drive is busy */
#define CD_STATUS_PAUSED    1   /**< \brief Disc is paused */
#define CD_STATUS_STANDBY   2   /**< \brief Drive is in standby */
#define CD_STATUS_PLAYING   3   /**< \brief Drive is currently playing */
#define CD_STATUS_SEEKING   4   /**< \brief Drive is currently seeking */
#define CD_STATUS_SCANNING  5   /**< \brief Drive is scanning */
#define CD_STATUS_OPEN      6   /**< \brief Disc tray is open */
#define CD_STATUS_NO_DISC   7   /**< \brief No disc inserted */
/** @} */

/** \defgroup cd_disc_types         CD-ROM drive disc types

    These are the values that can be returned as the disc_type parameter from
    the cdrom_get_status() function.
    @{
*/
#define CD_CDDA     0       /**< \brief Audio CD (Red book) */
#define CD_CDROM    0x10    /**< \brief CD-ROM or CD-R (Yellow book) */
#define CD_CDROM_XA 0x20    /**< \brief CD-ROM XA (Yellow book extension) */
#define CD_CDI      0x30    /**< \brief CD-i (Green book) */
#define CD_GDROM    0x80    /**< \brief GD-ROM */
/** @} */

/** \brief  TOC structure returned by the BIOS.

    This is the structure that the CMD_GETTOC2 syscall command will return for
    the TOC. Note the data is in FAD, not LBA/LSN.

    \headerfile dc/cdrom.h
*/
typedef struct {
    uint32  entry[99];          /**< \brief TOC space for 99 tracks */
    uint32  first;              /**< \brief Point A0 information (1st track) */
    uint32  last;               /**< \brief Point A1 information (last track) */
    uint32  leadout_sector;     /**< \brief Point A2 information (leadout) */
} CDROM_TOC;

/** \defgroup cd_toc_access         CD-ROM TOC access macros
    @{
*/
/** \brief  Get the FAD address of a TOC entry.
    \param  n               The actual entry from the TOC to look at.
    \return                 The FAD of the entry.
*/
#define TOC_LBA(n) ((n) & 0x00ffffff)

/** \brief  Get the address of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's address.
*/
#define TOC_ADR(n) ( ((n) & 0x0f000000) >> 24 )

/** \brief  Get the control data of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's control value.
*/
#define TOC_CTRL(n) ( ((n) & 0xf0000000) >> 28 )

/** \brief  Get the track number of a TOC entry.
    \param  n               The entry from the TOC to look at.
    \return                 The entry's track.
*/
#define TOC_TRACK(n) ( ((n) & 0x00ff0000) >> 16 )
/** @} */

/** \brief  Set the sector size for read sectors.

    This function sets the sector size that the cdrom_read_sectors() function
    will return. Be sure to set this to the correct value for the type of
    sectors you're trying to read. Common values are 2048 (for reading CD-ROM
    sectors) or 2352 (for reading raw sectors).

    \param  size            The size of the sector data.
*/
void cdrom_set_sector_size(int size);

/** \brief  Execute a CD-ROM command.

    This function executes the specified command using the BIOS syscall for
    executing GD-ROM commands.

    \param  cmd             The command number to execute.
    \param  param           Data to pass to the syscall.

    \return                 \ref cd_cmd_response
*/
int cdrom_exec_cmd(int cmd, void *param);

/** \brief  Get the status of the GD-ROM drive.

    \param  status          Space to return the drive's status.
    \param  disc_type       Space to return the type of disc in the drive.

    \return                 \ref cd_cmd_response
    \see    cd_status_values
    \see    cd_disc_types
*/
int cdrom_get_status(int *status, int *disc_type);

/** \brief  Re-initialize the GD-ROM drive.

    This function is for reinitializing the GD-ROM drive after a disc change,
    or something of the like.

    \return                 \ref cd_cmd_response
*/
int cdrom_reinit();

/** \brief  Read the table of contents from the disc.

    This function reads the TOC from the specified session of the disc.

    \param  toc_buffer      Space to store the returned TOC in.
    \param  session         The session of the disc to read.
    \return                 \ref cd_cmd_response
*/
int cdrom_read_toc(CDROM_TOC *toc_buffer, int session);

/** \brief  Read one or more sector from a CD-ROM.

    This function reads the specified number of sectors from the disc, starting
    where requested. This will respect the size of the sectors set with
    cdrom_set_sector_size(). The buffer must have enough space to store the
    specified number of sectors.

    \param  buffer          Space to store the read sectors.
    \param  sector          The sector to start reading from.
    \param  cnt             The number of sectors to read.
    \return                 \ref cd_cmd_response
*/
int cdrom_read_sectors(void *buffer, int sector, int cnt);

/** \brief  Locate the sector of the data track.

    This function will search the toc for the last entry that has a CTRL value
    of 4, and return its FAD address.

    \param  toc             The TOC to search through.
    \return                 The FAD of the track, or 0 if none is found.
*/
uint32 cdrom_locate_data_track(CDROM_TOC *toc);

/** \brief  Play CDDA audio tracks or sectors.

    This function starts playback of CDDA audio.

    \param  start           The track or sector to start playback from.
    \param  end             The track or sector to end playback at.
    \param  loops           The number of times to repeat (max of 15).
    \param  mode            The mode to play (see \ref cdda_read_modes).
    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_play(uint32 start, uint32 end, uint32 loops, int mode);

/** \brief  Pause CDDA audio playback.

    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_pause();

/** \brief  Resume CDDA audio playback after a pause.

    \return                 \ref cd_cmd_response
*/
int cdrom_cdda_resume();

/** \brief  Spin down the CD.

    This stops the disc in the drive from spinning until it is accessed again.

    \return                 \ref cd_cmd_response
*/
int cdrom_spin_down();

/** \brief  Initialize the GD-ROM for reading CDs.

    This initializes the CD-ROM reading system, reactivating the drive and
    handling initial setup of the disc.

    \retval 0               On success.
    \retval -1              If cdrom_init() has already been called.
*/
int cdrom_init();

/** \brief  Shutdown the CD reading system. */
void cdrom_shutdown();

__END_DECLS

#endif  /* __DC_CDROM_H */
