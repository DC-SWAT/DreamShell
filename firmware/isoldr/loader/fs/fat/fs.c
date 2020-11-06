/**
 * DreamShell ISO Loader
 * FAT file system
 * (c)2011-2020 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include "ffconf.h"

#ifdef DEV_TYPE_SD
#	define SZ_TBL 20
#else
#	define SZ_TBL 80
#endif

enum FILE_STATE {
	FILE_STATE_UNUSED = 0,
	FILE_STATE_USED = 1,
	FILE_STATE_POLL = 2
};

typedef struct {

	FIL fp;
	int state;
	fs_callback_f *poll_cb;
	
#if _USE_FASTSEEK
	DWORD lktbl[SZ_TBL];
#endif

} FILE;

static FATFS _fat_fs __attribute__((aligned(32)));
static FILE _files[MAX_OPEN_FILES] __attribute__((aligned(32)));

#ifdef DEV_TYPE_IDE
static int dma_enabled = FS_DMA_SHARED;
#else
static int dma_enabled = 0;
#endif

PARTITION VolToPart[_VOLUMES] = {{0, 0}};

#ifdef LOG
#	define CHECK_FD() \
		if(fd < 0 || fd > (MAX_OPEN_FILES - 1) || _files[fd].state == FILE_STATE_UNUSED) { \
			LOGFF("Bad fd = %d\n", fd); \
			return FS_ERR_NOFILE; \
		}
#else
#	define CHECK_FD()
#endif


static int fs_get_fd() {

	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if(_files[i].state == FILE_STATE_UNUSED) {
			return i;
		}
	}

	return FS_ERR_NUMFILES;
}


#if defined(LOG) && defined(LOG_DCL)

#include "../dcl/include/fs_dcload.h"

#define dclsc(...) ({                                 \
		int rv, old;                                   \
		old = irq_disable();                           \
		do {} while ((*(vuint32 *)0xa05f688c) & 0x20); \
		rv = dcloadsyscall(__VA_ARGS__);               \
		irq_restore(old);                              \
		rv;                                            \
	})

int dcload_type = DCLOAD_TYPE_NONE;

int dcload_reinit() {
    return dclsc(DCLOAD_REINIT, 0, 0, 0);
}

int dcload_write_buffer(const uint8 *data, int len) {
    return dclsc(DCLOAD_WRITE, 1, data, len);
}

#ifdef USE_GDB
size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size) {
    size_t ret = -1;

    /* we have to pack the sizes together because the dcloadsyscall handler
       can only take 4 parameters */
    ret = dclsc(DCLOAD_GDBPACKET, in_buf, (in_size << 16) | (out_size & 0xffff), out_buf);

    
    return ret;
}
#endif

#endif


int fs_init() {

	char path[3] = "0:";
	path[2] = '\0';

	VolToPart[0].pd = 0;
	VolToPart[0].pt = IsoInfo->fs_part + 1;

#if defined(LOG) && defined(LOG_DCL)
    if(*DCLOADMAGICADDR == DCLOADMAGICVALUE) {

		/* Give dcload the 64k it needs to compress data (if on serial) */

		if(dclsc(DCLOAD_ASSIGNWRKMEM, (int*)(loader_addr < 0x8c010000 ? 0x8cfe0000 : 0x8cfb0000)) == -1) {
			dcload_type = DCLOAD_TYPE_IP;
		} else {
			dcload_type = DCLOAD_TYPE_SER;
		}

		printf("dc-load-%s initialized\n", (dcload_type == DCLOAD_TYPE_IP ? "ip" : "serial"));
	}
#endif

	/* Reset file handlers */
	memset(&_files, 0, sizeof(_files));

	if(disk_initialize(0) == 0) {

		printf("Mounting FAT filesystem...\n");

		LOGFF("FATFS at 0x%08lx, common secbuf at 0x%08lx\n", 
					__func__, (uint32)&_fat_fs, (uint32)&_fat_fs.win);

//		LOGFF("FILES at 0x%08lx\n", (uint32)&_files);

		if(f_mount(&_fat_fs, path, 1) != FR_OK)
			return -1;

//		if(f_chdrive(path) != FR_OK)
//			return -1;

		return 0;
	}

	printf("Error, can't init "DEV_NAME".\n");
	return -1;
}


void fs_enable_dma(int state) {

#ifdef LOG
	if(dma_enabled != state)
		LOGFF("%d\n", state);
#endif
	dma_enabled = state;
}

int fs_dma_enabled() {
	return dma_enabled;
}


int open(const char *path, int mode) {

	int fd = fs_get_fd();
	FRESULT r;

	if(fd < 0) {
		LOGFF("ERROR, open limit is %d\n", MAX_OPEN_FILES);
		return fd;
	}

	BYTE flags = 0;

#if !_FS_READONLY

	if(mode == O_RDONLY) {
		flags = FA_OPEN_EXISTING | FA_READ;
	} else {
		flags = FA_CREATE_ALWAYS | FA_WRITE;
	}

#else
	(void)mode;
	flags = FA_OPEN_EXISTING | FA_READ;
#endif

#ifdef DEV_TYPE_IDE
	int old_dma_enabled = dma_enabled;
	dma_enabled = 0;
#endif

	r = f_open(&_files[fd].fp, path, flags);
	
	if(r != FR_OK) {

		LOGFF("ERROR %d\n", r);

#ifdef DEV_TYPE_IDE
		dma_enabled = old_dma_enabled;
#endif
		return FS_ERR_NOFILE;
	}

#if _USE_FASTSEEK

#if !_FS_READONLY
if(mode == O_RDONLY) {
#endif

	/* Using fast seek feature */
	_files[fd].fp.cltbl = _files[fd].lktbl;    /* Enable fast seek feature */
	_files[fd].lktbl[0] = SZ_TBL;              /* Set table size to the first item */

	/* Create linkmap */
	r = f_lseek(&_files[fd].fp, CREATE_LINKMAP);

	if(r == FR_NOT_ENOUGH_CORE) {

#if 0 // Disable it for now
		uint32 addr, sz;
		sz = _files[fd].fp.cltbl[0];

		/**
		 * FIXME: Dangerous code!
		 * Next file with the same error replace it!
		 */
		if((uint32)IsoInfo >= 0x8c004000) {
			addr = 0x8c001300;
		} else {
			addr = 0x8cff4800;
		}

		LOGFF("WARNING, linkmap table at 0x08lx\n", addr);

		_files[fd].fp.cltbl = (DWORD *)addr;
		_files[fd].fp.cltbl[0] = sz;
		memset(_files[fd].fp.cltbl, 0, sz * sizeof(DWORD));
	}

	/* Create linkmap */
	r = f_lseek(&_files[fd].fp, CREATE_LINKMAP);

#else
		_files[fd].fp.cltbl = NULL;
	}
#endif

#ifdef LOG
	if(r != FR_OK) {
		LOGFF("ERROR, creating linkmap required %d (avail %d) dwords, code %d\n", _files[fd].lktbl[0], SZ_TBL, r);
	} else {
		LOGFF("Created linkmap with %d sequences\n", _files[fd].lktbl[0]);
	}
#endif

#if !_FS_READONLY
}
#endif

#endif /* _USE_FASTSEEK */

#ifdef DEV_TYPE_IDE
	dma_enabled = old_dma_enabled;
#endif

	_files[fd].state = FILE_STATE_USED;
	return fd;
}

int close(int fd) {

	FRESULT rc;
	CHECK_FD();

	if(_files[fd].poll_cb) {
		LOGFF("WARNING, aborting async for fd %d\n", fd);
		abort_async(fd);
	}

	rc = f_close(&_files[fd].fp);
	memset(&_files[fd], 0, sizeof(FILE));

	if(rc != FR_OK) {
		LOGFF("ERROR, fd %d code %d\n", fd, rc);
		return FS_ERR_SYSERR;
	}

	return 0;
}


int read(int fd, void *ptr, unsigned int size) {

	uint br;
	CHECK_FD();

	if(_files[fd].poll_cb) {
		LOGFF("WARNING, aborting async for fd %d\n", fd);
		abort_async(fd);
	}

#ifdef LOG
	FRESULT r;

	if((r = f_read(&_files[fd].fp, ptr, size, &br)) == FR_OK) {

		if(size != br) {
			LOGFF("%d != %d\n", size, br);
		}

		return br;

	} else {
		LOGFF("ERROR %d\n", r);
	}

#else

	if(f_read(&_files[fd].fp, ptr, size, &br) == FR_OK) {
	    return br;
	}

#endif

	return FS_ERR_SYSERR;
}

int pre_read(int fd, unsigned long offset, unsigned int size) {

	CHECK_FD();
	FRESULT rc = f_pre_read(&_files[fd].fp, offset, size);

	if(rc != FR_OK) {
		LOGFF("ERROR, fd %d code %d\n", fd, rc);
		return FS_ERR_SYSERR;
	}

	return 0;
}

int read_async(int fd, void *ptr, unsigned int size, fs_callback_f *cb) {

	CHECK_FD();

	if(_files[fd].poll_cb) {
		LOGFF("WARNING, aborting async for fd %d\n", fd);
		abort_async(fd);
	}

	_files[fd].poll_cb = cb;

	if(f_read_async(&_files[fd].fp, ptr, size) == FR_OK) {
		return 0;
	}

	_files[fd].poll_cb = NULL;
	return FS_ERR_SYSERR;
}


int abort_async(int fd) {

	CHECK_FD();
	FRESULT rc;

	if(!_files[fd].poll_cb) {
		return FS_ERR_PARAM;
	}

	_files[fd].poll_cb = NULL;
	rc = f_abort(&_files[fd].fp);

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

	if(!_files[fd].poll_cb || _files[fd].state == FILE_STATE_POLL) {
		return 0;
	}

	_files[fd].state = FILE_STATE_POLL;
	rc = f_poll(&_files[fd].fp, &bp);

//	LOGFF("%d %d %d\n", fd, rc, bp);

	switch(rc) {
		case FR_OK:
			cb = _files[fd].poll_cb;
			_files[fd].poll_cb = NULL;
			cb(bp);
			rv = 0;
			break;
		case FR_NOT_READY:
			rv = bp;
			break;
		default:
			LOGFF("ERROR, fd %d code %d bytes %d\n", fd, rc, bp);
			cb = _files[fd].poll_cb;
			_files[fd].poll_cb = NULL;
			cb(-1);
			rv = -1;
			break;
	}

	_files[fd].state = FILE_STATE_USED;
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

	if(f_write(&_files[fd].fp, ptr, size, &bw) == FR_OK) {
		f_sync(&_files[fd].fp);
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
			if(_files[fd].fp.fptr != (uint32)offset)
				r = f_lseek(&_files[fd].fp, offset);
			break;
		case SEEK_CUR:
			r = f_lseek(&_files[fd].fp, _files[fd].fp.fptr + offset);
			break;
		case SEEK_END:
			r = f_lseek(&_files[fd].fp, _files[fd].fp.fsize + offset);
			break;
		default:
			break;
	}

	return r == FR_OK ? (long int)_files[fd].fp.fptr : FS_ERR_SYSERR;
}

long int tell(int fd) {
	CHECK_FD();
	return _files[fd].fp.fptr;
}

unsigned long total(int fd) {
	CHECK_FD();
	return _files[fd].fp.fsize;
}
