/**
 * DreamShell ISO Loader
 * BIOS syscalls emulation
 * (c)2009-2025 SWAT <http://www.dc-swat.ru>
 */

#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <arch/types.h>
#include <dc/cdrom.h>

/** \defgroup cd_cmd_codes CD-ROM syscall command codes
	@{
*/
#define CMD_CHECK_LICENSE       2  /**< \brief  */
#define CMD_REQ_SPI_CMD         4  /**< \brief  */
#define CMD_PIOREAD            16  /**< \brief Read via PIO */
#define CMD_DMAREAD            17  /**< \brief Read via DMA */
#define CMD_GETTOC             18  /**< \brief Read TOC */
#define CMD_GETTOC2            19  /**< \brief Read TOC */
#define CMD_PLAY_TRACKS        20  /**< \brief Play tracks */
#define CMD_PLAY_SECTORS       21  /**< \brief Play sectors */
#define CMD_PAUSE              22  /**< \brief Pause playback */
#define CMD_RELEASE            23  /**< \brief Resume from pause */
#define CMD_INIT               24  /**< \brief Initialize the drive */
#define CMD_DMA_ABORT          25  /**< \brief  */
#define CMD_OPEN_TRAY          26  /**< \brief  */
#define CMD_SEEK               27  /**< \brief Seek to a new position */
#define CMD_DMAREAD_STREAM     28  /**< \brief  */
#define CMD_NOP                29  /**< \brief  */
#define CMD_REQ_MODE           30  /**< \brief  */
#define CMD_SET_MODE           31  /**< \brief  */
#define CMD_SCAN_CD            32  /**< \brief  */
#define CMD_STOP               33  /**< \brief Stop the disc from spinning */
#define CMD_GETSCD             34  /**< \brief Get subcode data */
#define CMD_GETSES             35  /**< \brief Get session */
#define CMD_REQ_STAT           36  /**< \brief  */
#define CMD_PIOREAD_STREAM     37  /**< \brief  */
#define CMD_DMAREAD_STREAM_EX  38  /**< \brief Can get parts < sector size in reqDmaStrans */
#define CMD_PIOREAD_STREAM_EX  39  /**< \brief  */
#define CMD_GET_VERS           40  /**< \brief  */
#define CMD_MAX                47  /**< \brief  */
/** @} */

/** \defgroup cd_cmd_status      CD-ROM Command Status responses

    These are the raw values the status syscall returns.
    @{
*/
#define CMD_STAT_FAILED     -1
#define CMD_STAT_IDLE        0
#define CMD_STAT_PROCESSING  1
#define CMD_STAT_COMPLETED   2
#define CMD_STAT_STREAMING   3
#define CMD_STAT_BUSY        4
#define CMD_STAT_UNK1        5
#define CMD_STAT_UNK2        6
/** @} */

/** \defgroup cd_cmd_error      CD-ROM command errors
    @{
*/
#define CMD_ERR_NOERR          0x00
#define CMD_ERR_RECOVERED      0x01
#define CMD_ERR_NOTREADY       0x02
#define CMD_ERR_MEDIUM         0x03
#define CMD_ERR_HARDWARE       0x04
#define CMD_ERR_ILLEGALREQUEST 0x05
#define CMD_ERR_UNITATTENTION  0x06
#define CMD_ERR_DATAPROTECT    0x07
#define CMD_ERR_ABORTED        0x0B
#define CMD_ERR_NOREADABLE     0x10
#define CMD_ERR_G1SEMAPHORE    0x20
/** @} */


/** \defgroup cd_cmd_wait      CD-ROM command wait types
    @{
*/
#define CMD_WAIT_INTERNAL   0x00
#define CMD_WAIT_IRQ        0x01
#define CMD_WAIT_DRQ_0      0x02
#define CMD_WAIT_DRQ_1      0x03
#define CMD_WAIT_BUSY       0x04
/** @} */

/** \defgroup cd_status_values      CD-ROM status values

    These are the values that can be returned as the status parameter from the
    cdrom_get_status() function.
    @{
*/
#define CD_STATUS_CANTREAD -1
#define CD_STATUS_BUSY     0x00
#define CD_STATUS_PAUSED   0x01
#define CD_STATUS_STANDBY  0x02
#define CD_STATUS_PLAYING  0x03
#define CD_STATUS_SEEK     0x04
#define CD_STATUS_SCAN     0x05
#define CD_STATUS_OPEN     0x06
#define CD_STATUS_NODISC   0x07
#define CD_STATUS_RETRY    0x08
#define CD_STATUS_ERROR    0x09
#define CD_STATUS_FATAL    0x0c
/** @} */

/* Values for CMD_GETSCD command */
#define SCD_REQ_ALL_SUBCODE      0x0
#define SCD_REQ_Q_SUBCODE        0x1
#define SCD_REQ_MEDIA_CATALOG    0x2
#define SCD_REQ_ISRC             0x3
#define SCD_REQ_RESERVED         0x4

#define SCD_AUDIO_STATUS_INVALID 0x00
#define SCD_AUDIO_STATUS_PLAYING 0x11
#define SCD_AUDIO_STATUS_PAUSED  0x12
#define SCD_AUDIO_STATUS_ENDED   0x13
#define SCD_AUDIO_STATUS_ERROR   0x14
#define SCD_AUDIO_STATUS_NO_INFO 0x15

#define SCD_DATA_SIZE_INDEX      3

#define GDC_PARAMS_COUNT 4
#define GDC_CHN_ERROR    0


/**
 * GD-ROM params
 */
typedef struct disk_format {
	
	int flags;
	int mode;
	int sec_size;
	
} disk_format_t;

typedef struct gd_state {

	int req_count;
	int cmd;
	int err;
	int status;
	int ata_status;
	int cmd_abort;

	uint32 requested;
	uint32 transfered;

	uint32 callback;
	uint32 callback_param;

	uint32 param[GDC_PARAMS_COUNT];
	uint32 true_async;

	/* Current values */
	uint32 cdda_track;
	uint32 data_track;
	uint32 lba;

	int cdda_stat;
	int drv_stat;
	int drv_media;

#ifdef HAVE_MULTI_DISC
	int disc_change;
	int disc_num;
	int need_reinit;
#endif
	disk_format_t gdc;

} gd_state_t;

gd_state_t *get_GDS(void);

/**
 * Lock/Unlock GD syscalls
 */
int lock_gdsys(void);
int unlock_gdsys(void);

void gdcExitToGame(void);
void gdcReset(void);

extern uint8 bios_patch_base[];
extern void *bios_patch_handler;
extern uint8 bios_patch_end[];

extern void *gdc_redir;
extern uint32 gdc_saved_vector;
extern void gdc_syscall_enable(void);
extern void gdc_syscall_save(void);
extern void gdc_syscall_disable(void);
void gdc_syscall_patch(void);

extern uint32 menu_saved_vector;
extern void menu_syscall_enable(void);
extern void menu_syscall_save(void);
extern void menu_syscall_disable(void);

extern uint32 sys_saved_vector;
extern void sys_syscall_enable(void);
extern void sys_syscall_save(void);
extern void sys_syscall_disable(void);

extern uint32 flash_saved_vector;
extern void flash_syscall_enable(void);
extern void flash_syscall_save(void);
extern void flash_syscall_disable(void);

extern uint32 bfont_saved_vector;
extern uint32 bfont_saved_addr;
extern uint8* get_font_address();
extern void bfont_syscall_enable(void);
extern void bfont_syscall_save(void);
extern void bfont_syscall_disable(void);

void enable_syscalls(int all);
void disable_syscalls(int all);
void restore_syscalls(void);

int sys_misc_init(void);

void menu_exit(void);

#endif
