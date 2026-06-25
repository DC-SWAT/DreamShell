/**
 * DreamShell ISO Loader
 * FAT file system
 * (c)2011-2026 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include "ffconf.h"

#define SZ_TBL 32

enum FILE_STATE {
	FILE_STATE_UNUSED = 0,
	FILE_STATE_USED = 1,
	FILE_STATE_POLL = 2,
	FILE_STATE_ASYNC = 3
};

typedef struct {

	FIL fp;
	volatile uint32 state;
	fs_callback_f *poll_cb;
	int dma_mode;

#if _USE_FASTSEEK
	DWORD cltbl[SZ_TBL];
#endif

} FILE;

static FATFS *_fat_fs[_VOLUMES];
static FILE *_files = NULL;

#ifdef DEV_TYPE_IDE
static int dma_mode = FS_DMA_SHARED;
#else
static int dma_mode = 0;
#endif

#if defined(DEV_TYPE_IDE) && defined(DEV_TYPE_SD)
PARTITION VolToPart[_VOLUMES] = {
	{DISK_DRV_IDE, 0},
	{DISK_DRV_SD_SCIF, 0}
};
#else
PARTITION VolToPart[_VOLUMES] = {{0, 0}};
#endif

#ifdef LOG
#	define CHECK_FD() \
		if(fd < 0 || fd > (MAX_OPEN_FILES - 1) || _files[fd].state == FILE_STATE_UNUSED) { \
			LOGFF("Bad fd = %d\n", fd); \
			return FS_ERR_NOFILE; \
		} \
		FILE *file = &_files[fd]
#else
#	define CHECK_FD() FILE *file = &_files[fd]
#endif


static int fs_get_fd() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if(_files[i].state == FILE_STATE_UNUSED) {
			return i;
		}
	}
	return FS_ERR_NUMFILES;
}


static int fs_mount_volume(int vol, BYTE pdrv, int disk_part) {

	char path[4] = "0:";

	path[0] = '0' + vol;

	VolToPart[vol].pd = pdrv;
	VolToPart[vol].pt = disk_part;

	if(disk_initialize(pdrv) != 0) {
		return -1;
	}

	LOGF("Mounting FAT filesystem...\n");

	if(f_mount(_fat_fs[vol], path, 1) == FR_OK) {
		return 0;
	}

	return -1;
}


#ifdef DEV_TYPE_SD
static int fs_mount_sd(int vol, int disk_part) {

	if(fs_mount_volume(vol, DISK_DRV_SD_SCIF, disk_part) == 0) {
		return 0;
	}

	OpenLog();

	return fs_mount_volume(vol, DISK_DRV_SD_SCI, disk_part);
}
#endif


int fs_init(int disk_part) {

	_files = (FILE *) malloc(sizeof(FILE) * MAX_OPEN_FILES);

	if (!_files) {
		LOGFF("Memory failed");
		return -1;
	}

	memset(_files, 0, sizeof(FILE) * MAX_OPEN_FILES);

	for(int vol = 0; vol < _VOLUMES; vol++) {

		_fat_fs[vol] = (FATFS *) aligned_alloc(32, sizeof(FATFS));

		if (!_fat_fs[vol]) {
			LOGFF("Memory failed");
			return -1;
		}

		memset(_fat_fs[vol], 0, sizeof(FATFS));

		LOGF("FATFS[%d]: 0x%08lx, secbuf: 0x%08lx, FILEs: 0x%08lx\n",
			vol, (uint32)_fat_fs[vol], (uint32)_fat_fs[vol]->win, (uint32)_files);
	}

#if defined(DEV_TYPE_IDE) && defined(DEV_TYPE_SD)

	int mounted = 0;

	if(fs_mount_sd(0, disk_part) == 0) {
		mounted++;
	}

	if(fs_mount_volume(1, DISK_DRV_IDE, disk_part) == 0) {
		mounted++;
	}

	if(mounted) {
		return 0;
	}

#else

#if defined(DEV_TYPE_IDE)

	if(fs_mount_volume(0, DISK_DRV_IDE, disk_part) == 0) {
		return 0;
	}

#elif defined(DEV_TYPE_SD)

	if(fs_mount_sd(0, disk_part) == 0) {
		return 0;
	}

#endif

	printf("Error, can't init "DEV_NAME".\n");

#endif

	return -1;
}


int fs_volume_mounted(int vol) {

	if(vol < 0 || vol >= _VOLUMES || !_fat_fs[vol]) {
		return 0;
	}

	return _fat_fs[vol]->fs_type != 0;
}


void fs_enable_dma(int state) {

#ifdef DBG
	if(dma_mode != state)
		LOGFF("%d\n", state);
#endif
	dma_mode = state;
}

int fs_dma_enabled() {
	return dma_mode;
}


int open(const char *path, int flags) {

	if (!_files) {
		return FS_ERR_SYSERR;
	}

	int fd = fs_get_fd();

	if(fd < 0) {
		LOGFF("ERROR, open limit is %d\n", MAX_OPEN_FILES);
		return fd;
	}

	FRESULT r;
	BYTE fat_flags = 0;
	FILE *file = &_files[fd];
	int mode = (flags & O_MODE_MASK);

	switch(mode) {
		case O_RDONLY:
			fat_flags = (FA_OPEN_EXISTING | FA_READ);
			break;
#if _FS_READONLY == 0
		case O_WRONLY:
			fat_flags = FA_WRITE | (flags & O_TRUNC ? FA_CREATE_ALWAYS : FA_CREATE_NEW);
			break;
		case O_RDWR:
			fat_flags = (FA_WRITE | FA_READ) | (flags & O_TRUNC ? FA_CREATE_ALWAYS : FA_OPEN_ALWAYS);
			break;
#endif
		default:
			return FS_ERR_PARAM;
	}

	int old_dma_mode = fs_dma_enabled();
	fs_enable_dma(FS_DMA_DISABLED);

	r = f_open(&file->fp, path, fat_flags);

	if(r != FR_OK) {
		LOGFF("failed to open %s, error %d\n", path, r);
		fs_enable_dma(old_dma_mode);
		if (r == FR_EXIST) {
			return FS_ERR_EXISTS;
		} else if(r == FR_NO_PATH) {
			return FS_ERR_NO_PATH;
		}
		return FS_ERR_NOFILE;
	}

	if((flags & O_APPEND) && file->fp.fsize > 0) {
		f_lseek(&file->fp, file->fp.fsize - 1);
	}

#if _USE_FASTSEEK
	if(mode == O_RDONLY) {

		/* Using fast seek feature */
		file->fp.cltbl = file->cltbl;    /* Enable fast seek feature */
		file->cltbl[0] = SZ_TBL;         /* Set table size to the first item */

		/* Create linkmap */
		r = f_lseek(&file->fp, CREATE_LINKMAP);

		if(r == FR_NOT_ENOUGH_CORE) {

			size_t count = file->fp.cltbl[0];
			DWORD *cltbl = (DWORD *)malloc(count * sizeof(DWORD));

			if (cltbl) {
				memset(cltbl, 0, count * sizeof(DWORD));
				cltbl[0] = count;
				file->fp.cltbl = cltbl;
				r = f_lseek(&file->fp, CREATE_LINKMAP);
			}
		}

		if(r != FR_OK) {
			file->fp.cltbl = NULL;
			// NOTE: This line causes build error with flto=auto
			// LOGFF("ERROR, creating linkmap required %d dwords, code %d\n", file->fp.cltbl[0], r);
		} else {
			LOGFF("Created linkmap with %d sequences\n", file->fp.cltbl[0]);
		}
	}
#endif /* _USE_FASTSEEK */

	fs_enable_dma(old_dma_mode);

	if(flags & O_PIO) {
		file->dma_mode = FS_DMA_DISABLED;
	} else {
		file->dma_mode = -1;
	}

	file->state = FILE_STATE_USED;
	return fd;
}

static void abort_current_async(int fd) {
#if _FS_ASYNC
	LOGF("FS: aborting async, fd=%d\n", fd);
	abort_async(fd);
	do {} while (pre_read_xfer_busy());
	pre_read_xfer_end();
#else
	(void)fd;
#endif
}

int close(int fd) {

	FRESULT rc;
	CHECK_FD();

	if(file->state == FILE_STATE_ASYNC) {
		abort_current_async(fd);
		file->state = FILE_STATE_USED;
	}

	rc = f_close(&file->fp);

	if (file->fp.cltbl && file->fp.cltbl[0] > SZ_TBL) {
		free(file->fp.cltbl);
	}

	memset(file, 0, sizeof(FILE));

	if(rc != FR_OK) {
		LOGFF("ERROR, fd %d code %d\n", fd, rc);
		return FS_ERR_SYSERR;
	}

	return 0;
}


int read(int fd, void *ptr, unsigned int size) {

	CHECK_FD();

	if(file->state == FILE_STATE_ASYNC) {
		abort_current_async(fd);
		file->state = FILE_STATE_USED;
	}

	int old_dma_mode = fs_dma_enabled();

	if(file->dma_mode > -1) {
		fs_enable_dma(file->dma_mode);
	}

	uint br;
	FRESULT rs = f_read(&file->fp, ptr, size, &br);

	fs_enable_dma(old_dma_mode);

	if(rs != FR_OK) {
		LOGFF("ERROR %d\n", rs);
		return FS_ERR_SYSERR;
	}

	return br;
}

#if _FS_ASYNC

int pre_read(int fd, unsigned int size) {

	CHECK_FD();

#ifdef DEV_TYPE_SD
	if(file->state == FILE_STATE_ASYNC) {
		abort_current_async(fd);
	}
	file->state = FILE_STATE_ASYNC;
#endif

	FRESULT rc = f_pre_read(&file->fp, size);

	if(rc != FR_OK) {
		LOGFF("ERROR, fd %d code %d\n", fd, rc);
		return FS_ERR_SYSERR;
	}

	return 0;
}

int read_async(int fd, void *ptr, unsigned int size, fs_callback_f *cb) {

	CHECK_FD();

	if(file->state == FILE_STATE_ASYNC) {
		// abort_current_async(fd);
		file->state = FILE_STATE_USED;
	}

	if (fs_dma_enabled() == FS_DMA_DISABLED) {
		int rv = read(fd, ptr, size);
		if(cb) {
			cb(rv);
		}
		return 0;
	}

	file->poll_cb = cb;
	file->state = FILE_STATE_ASYNC;

	if(f_read_async(&file->fp, ptr, size) == FR_OK) {
		return 0;
	}

	file->poll_cb = NULL;
	file->state = FILE_STATE_USED;
	return FS_ERR_SYSERR;
}


int abort_async(int fd) {

	CHECK_FD();

	file->poll_cb = NULL;
	file->state = FILE_STATE_USED;
	f_abort(&file->fp);

	return 0;
}


int fs_async_active(int fd) {

	CHECK_FD();

	return file->state == FILE_STATE_ASYNC || file->poll_cb != NULL;
}


int poll(int fd) {

	UINT bp = 0;
	FRESULT rc;
	int rv = 0;
	fs_callback_f *cb;

	CHECK_FD();

	if(file->state == FILE_STATE_POLL) {
		LOGFF("Busy\n");
		return 1;
	}
	if(file->state != FILE_STATE_ASYNC) {
		return 0;
	}

	file->state = FILE_STATE_POLL;
	rc = f_poll(&file->fp, &bp);

	DBGFF("fd=%d rc=%d bp=%d\n", fd, rc, bp);

	switch(rc) {
		case FR_OK:
			cb = file->poll_cb;
			file->poll_cb = NULL;
			file->state = FILE_STATE_USED;
			if(cb) {
				cb(bp);
			}
			rv = 0;
			break;
		case FR_NOT_READY:
			rv = bp;
			file->state = FILE_STATE_ASYNC;
			break;
		default:
			LOGFF("ERROR, fd %d code %d bytes %d\n", fd, rc, bp);
			cb = file->poll_cb;
			file->poll_cb = NULL;
			file->state = FILE_STATE_USED;
			if(cb) {
				cb(-1);
			}
			rv = -1;
			break;
	}

	return rv;
}

void poll_all(int err) {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		FILE *file = &_files[i];
		if(file->state == FILE_STATE_ASYNC) {
			if (err) {
				fs_callback_f *cb = file->poll_cb;
				file->poll_cb = NULL;
				file->state = FILE_STATE_USED;
				if(cb) {
					cb(err);
				}
			}
			else {
				poll(i);
			}
		}
	}
}

#endif

#if _FS_READONLY == 0

int write(int fd, void *ptr, unsigned int size) {

	CHECK_FD();

	int old_dma_mode = fs_dma_enabled();

	if(file->dma_mode > -1) {
		fs_enable_dma(file->dma_mode);
	}

	uint bw;
	FRESULT rs = f_write(&file->fp, ptr, size, &bw);

	fs_enable_dma(old_dma_mode);

	if(rs != FR_OK) {
		LOGFF("ERROR %d\n", rs);
		return FS_ERR_SYSERR;
	}

	return bw;
}

#endif

long int lseek(int fd, long int offset, int whence) {

	FRESULT r = FR_OK;
	CHECK_FD();

	switch(whence) {
		case SEEK_SET:
			if(file->fp.fptr != (uint32)offset)
				r = f_lseek(&file->fp, offset);
			break;
		case SEEK_CUR:
			r = f_lseek(&file->fp, file->fp.fptr + offset);
			break;
		case SEEK_END:
			r = f_lseek(&file->fp, file->fp.fsize + offset);
			break;
		default:
			break;
	}

	return r == FR_OK ? (long int)file->fp.fptr : FS_ERR_SYSERR;
}

long int tell(int fd) {
	CHECK_FD();
	return file->fp.fptr;
}

unsigned long total(int fd) {
	CHECK_FD();
	return file->fp.fsize;
}

int ioctl(int fd, int cmd, void *data) {
	CHECK_FD();
	switch(cmd) {
		case FS_IOCTL_GET_LBA:
		{
			unsigned long sec = clust2sect(file->fp.fs, file->fp.sclust);
			memcpy(data, &sec, sizeof(sec));
			return 0;
		}
#if _FS_READONLY == 0
		case FS_IOCTL_SYNC:
			if (f_sync(&file->fp) == FR_OK) {
				return 0;
			}
			break;
#endif
		default:
			return FS_ERR_PARAM;
	}
	return FS_ERR_SYSERR;
}
