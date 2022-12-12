/**
 * DreamShell ISO Loader
 * File system
 * (c)2011-2022 SWAT <http://www.dc-swat.ru>
 */
#ifndef __FS_H__
#define __FS_H__

#if defined(DEV_TYPE_NET)

#include "commands.h"

#elif defined(DEV_TYPE_SD) || defined(DEV_TYPE_IDE)

#include "ff.h"
#include "diskio.h"

#if defined(DEV_TYPE_SD)
#include "spi.h"
#elif defined(DEV_TYPE_IDE)
#include <ide/ide.h>
#endif

#elif defined(DEV_TYPE_GD)

#include <ide/ide.h>

#endif /* DEV_TYPE_NET */

#ifndef MAX_OPEN_FILES
# define MAX_OPEN_FILES 3
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
#define O_PIO       0x1000	/* do not use DMA */

enum FS_DMA_STATE {
	FS_DMA_DISABLED = 0,
	FS_DMA_SHARED   = 1,
	FS_DMA_HIDDEN   = 2,
	FS_DMA_NO_IRQ   = 3
};

enum FS_IOCTL_CMD {
	FS_IOCTL_GET_LBA = 0
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
int ioctl(int fd, int cmd, void *data);

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

#if defined(DEV_TYPE_GD) || defined(DEV_TYPE_IDE)
#	define pre_read_xfer_start  g1_dma_start
#	define pre_read_xfer_busy   g1_dma_in_progress
#	define pre_read_xfer_size   g1_dma_transfered
#else
#	define pre_read_xfer_start(addr, bytes) do { } while(0)
#	define pre_read_xfer_busy() 0
#	define pre_read_xfer_size() 0
#endif

#endif /* __FS_H__ */
