/**
 * DreamShell ISO Loader
 * FAT file system
 * (c)2011-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include "ffconf.h"

#define SZ_TBL 32

enum FILE_STATE {
	FILE_STATE_UNUSED = 0,
	FILE_STATE_USED = 1,
	FILE_STATE_POLL = 2
};

typedef struct {

	FIL fp;
	uint32 state;
	fs_callback_f *poll_cb;
	int dma_mode;

#if _USE_FASTSEEK
	DWORD cltbl[SZ_TBL];
#endif

} FILE;

static FATFS *_fat_fs = NULL;
static FILE *_files = NULL;

#ifdef DEV_TYPE_IDE
static int dma_mode = FS_DMA_SHARED;
#else
static int dma_mode = 0;
#endif

PARTITION VolToPart[_VOLUMES] = {{0, 0}};

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


int fs_init() {

	char path[3] = "0:";
	path[2] = '\0';

	VolToPart[0].pd = 0;
	VolToPart[0].pt = IsoInfo->fs_part + 1;

	_fat_fs = (FATFS *) malloc(sizeof(FATFS) + 32);
	_files = (FILE *) malloc(sizeof(FILE) * MAX_OPEN_FILES);

	if (!_fat_fs || !_files) {
		LOGFF("Memory failed");
		return -1;
	}

	_fat_fs = (FATFS *)ALIGN32_ADDR((uint32)_fat_fs);

	LOGF("FATFS: 0x%08lx, secbuf: 0x%08lx, FILEs: 0x%08lx\n",
		(uint32)_fat_fs, (uint32)_fat_fs->win, (uint32)_files);

	memset(_fat_fs, 0, sizeof(FATFS));
	memset(_files, 0, sizeof(FILE) * MAX_OPEN_FILES);

	if(disk_initialize(0) == 0) {

		LOGF("Mounting FAT filesystem...\n");

		if(f_mount(_fat_fs, path, 1) != FR_OK) {
			return -1;
		}

		return 0;
	}

	printf("Error, can't init "DEV_NAME".\n");
	return -1;
}


void fs_enable_dma(int state) {

#ifdef LOG
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
#if !_FS_READONLY
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

	int old_dma_mode = dma_mode;
	dma_mode = 0;

	r = f_open(&file->fp, path, fat_flags);

	if(r != FR_OK) {
		LOGFF("failed to open %s, error %d\n", path, r);
		dma_mode = old_dma_mode;
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
			LOGFF("ERROR, creating linkmap required %d dwords, code %d\n", file->fp.cltbl[0], r);
		} else {
			LOGFF("Created linkmap with %d sequences\n", file->fp.cltbl[0]);
		}
	}
#endif /* _USE_FASTSEEK */

	dma_mode = old_dma_mode;
	if(flags & O_PIO) {
		file->dma_mode = FS_DMA_DISABLED;
	} else {
		file->dma_mode = -1;
	}

	file->state = FILE_STATE_USED;
	return fd;
}

int close(int fd) {

	FRESULT rc;
	CHECK_FD();

	if(file->poll_cb) {
		LOGFF("WARNING, aborting async for fd %d\n", fd);
		abort_async(fd);
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

	if(file->poll_cb) {
		LOGFF("WARNING, aborting async for fd %d\n", fd);
		abort_async(fd);
	}

	int old_dma_mode = dma_mode;
	if(file->dma_mode > -1) {
		dma_mode = file->dma_mode;
	}
	uint br;
	FRESULT rs = f_read(&file->fp, ptr, size, &br);

	dma_mode = old_dma_mode;

	if(rs != FR_OK) {
		LOGFF("ERROR %d\n", rs);
		return FS_ERR_SYSERR;
	}

	return br;
}

int pre_read(int fd, unsigned long offset, unsigned int size) {

	CHECK_FD();
	FRESULT rc = f_pre_read(&file->fp, offset, size);

	if(rc != FR_OK) {
		LOGFF("ERROR, fd %d code %d\n", fd, rc);
		return FS_ERR_SYSERR;
	}

	return 0;
}

int read_async(int fd, void *ptr, unsigned int size, fs_callback_f *cb) {

	CHECK_FD();

	if(file->poll_cb) {
		LOGFF("WARNING, aborting async for fd %d\n", fd);
		abort_async(fd);
	}

	file->poll_cb = cb;

	if(f_read_async(&file->fp, ptr, size) == FR_OK) {
		return 0;
	}

	file->poll_cb = NULL;
	return FS_ERR_SYSERR;
}


int abort_async(int fd) {

	CHECK_FD();
	FRESULT rc;

	if(!file->poll_cb) {
		return FS_ERR_PARAM;
	}

	file->poll_cb = NULL;
	rc = f_abort(&file->fp);

	if(rc != FR_OK) {
		LOGFF("ERROR, fd %d code %d\n", fd, rc);
		return FS_ERR_SYSERR;
	}

	return 0;
}


int poll(int fd) {

	UINT bp = 0;
	FRESULT rc;
	int rv = 0;
	fs_callback_f *cb;

	CHECK_FD();

	if(!file->poll_cb || file->state == FILE_STATE_POLL) {
		return 0;
	}

	file->state = FILE_STATE_POLL;
	rc = f_poll(&file->fp, &bp);

//	LOGFF("%d %d %d\n", fd, rc, bp);

	switch(rc) {
		case FR_OK:
			cb = file->poll_cb;
			file->poll_cb = NULL;
			cb(bp);
			rv = 0;
			break;
		case FR_NOT_READY:
			rv = bp;
			break;
		default:
			LOGFF("ERROR, fd %d code %d bytes %d\n", fd, rc, bp);
			cb = file->poll_cb;
			file->poll_cb = NULL;
			cb(-1);
			rv = -1;
			break;
	}

	file->state = FILE_STATE_USED;
	return rv;
}

void poll_all() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if(_files[i].state == FILE_STATE_USED && _files[i].poll_cb != NULL) {
			poll(i);
		}
	}
}

#if !_FS_READONLY

int write(int fd, void *ptr, unsigned int size) {

	uint bw;
	CHECK_FD();

	if(f_write(&file->fp, ptr, size, &bw) == FR_OK) {
		f_sync(&file->fp);
		return bw;
	}
	return FS_ERR_SYSERR;
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
		default:
			return FS_ERR_PARAM;
	}
}
