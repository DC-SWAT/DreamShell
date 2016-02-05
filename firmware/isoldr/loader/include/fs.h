/**
 * DreamShell ISO Loader
 * File system
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */

#if defined(DEV_TYPE_NET)

#include "commands.h"

// Our execution mode
extern int sl_mode;

#define SLMODE_NONE 0	// not initialized yet
#define SLMODE_EXCL 1	// exclusive
#define SLMODE_IMME 2	// immediate
#define SLMODE_COOP 3	// cooperative

#elif defined(DEV_TYPE_DCL)

#include "fs_dcload.h"

#elif defined(DEV_TYPE_SD) || defined(DEV_TYPE_IDE)

#include "ff.h"
#include "diskio.h"

#if defined(DEV_TYPE_SD)
#include "spi.h"
#elif defined(DEV_TYPE_IDE)
#include "g1ata.h"
#endif

#if defined(LOG) && _FS_READONLY == 1
#include "../fs/dcl/include/fs_dcload.h"
#endif

#elif defined(DEV_TYPE_GD)

int g1_dma_in_progress(void);
uint32_t g1_dma_transfered(void);
int g1_dma_irq_enabled();

#endif /* DEV_TYPE_NET */


#if _FS_READONLY == 1
#define MAX_OPEN_FILES 3
#else
#define MAX_OPEN_FILES 4
#endif


#define FS_ERR_SYSERR   -1    /* Generic error from device             */
#define FS_ERR_DIRERR   -2    /* Root directory not found              */
#define FS_ERR_NOFILE   -3    /* File not found                        */
#define FS_ERR_PARAM    -4    /* Invalid parameters passed to function */
#define FS_ERR_NUMFILES -5    /* Max number of open files exceeded     */
#define FS_ERR_NODISK   -6    /* No disc/card present                  */
#define FS_ERR_DISKCHG  -7    /* Disc has been replaced with a new one */


/** \brief  File descriptor type */
typedef int file_t;

/** \brief  Invalid file handle constant (for open failure, etc) */
#define FILEHND_INVALID ((file_t)-1)

/** \brief  Callback function type */
typedef void fs_callback_f(size_t);


/** \defgroup seek_modes            Seek modes

    These are the values you can pass for the whence parameter to fs_seek().

    @{
*/
#define SEEK_SET    0           /**< \brief Set position to offset. */
#define SEEK_CUR    1           /**< \brief Seek from current position. */
#define SEEK_END    2           /**< \brief Seek from end of file. */
/** @} */

#define O_MODE_MASK      0x0f
#define O_RDONLY         0
#define O_DIR            1
#define O_WRONLY         2
#define O_RDWR           3

#define O_APPEND    0x0008	/* append (writes guaranteed at the end) */
#define O_CREAT     0x0200	/* open with file create */
#define O_TRUNC     0x0400	/* open with truncation */

enum FS_DMA_STATE {
	FS_DMA_DISABLED = 0,
	FS_DMA_SHARED   = 1,
	FS_DMA_HIDDEN   = 2
};

int fs_init();
void fs_shutdown();

void fs_enable_dma(int state);
int fs_dma_enabled();

int open(const char *path, int mode);
int close(int fd);

int pread(int fd, void *buf, unsigned int nbyte, unsigned int offset);
int read(int fd, void *buf, unsigned int nbyte);
int write(int fd, void *buf, unsigned int nbyte);
long int lseek(int fd, long int offset, int whence);
long int tell(int fd);
unsigned long total(int fd);
int ioctl(int fd, int request, void *data);

/**
 * Async read feature
 */
int read_async(int fd, void *buf, unsigned int nbyte, fs_callback_f *cb);
int abort_async(int fd);
int poll(int fd);
void poll_all();

/**
 * Async stream read feature
 */
int pre_read(int fd, unsigned long offset, unsigned int size);

#if defined(DEV_TYPE_GD)
#	define pre_read_xfer_busy   g1_dma_in_progress
#	define pre_read_xfer_size   g1_dma_transfered
#elif defined(DEV_TYPE_IDE)
#	define pre_read_xfer_start  g1_dma_start
#	define pre_read_xfer_busy   g1_dma_in_progress
#	define pre_read_xfer_size   g1_dma_transfered
#else
#	define pre_read_xfer_start(addr, bytes) do { } while(0)
#	define pre_read_xfer_busy() 0
#	define pre_read_xfer_size() 0
#endif
