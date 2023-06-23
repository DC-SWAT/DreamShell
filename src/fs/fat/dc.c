/****************************
 * DreamShell ##version##   *
 * DreamShell FAT fs        *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ****************************/

#include "ds.h"
#include "fs.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "fatfs/integer.h"

// #define FATFS_DEBUG 1
// #define FATFS_DMA 1

#define MAX_FAT_MOUNTS        _VOLUMES
#define MAX_FAT_FILES         16
#define FATFS_LINK_TBL_SIZE   32

typedef struct fatfs_mnt {

	FATFS *fs;
	vfs_handler_t *vfsh;
	kos_blockdev_t *dev;

	DSTATUS dev_stat;
	BYTE dev_id;
	BYTE used;

	TCHAR dev_path[16];

#ifdef FATFS_DMA
	BYTE dma;
	uint8 *dmabuf;
#endif

} fatfs_mnt_t;

typedef struct fatfs {

	FIL fil __attribute__((aligned(32)));
	DIR dir;
	int type;
	int used;
	int mode;

	DWORD lktbl[FATFS_LINK_TBL_SIZE];
	dirent_t dent;

	fatfs_mnt_t *mnt;

} fatfs_t;

/* Mutex for file handles */
static mutex_t fat_mutex;
static int initted = 0;

static fatfs_t fh[MAX_FAT_FILES] __attribute__((aligned(32)));
static fatfs_mnt_t fat_mnt[MAX_FAT_MOUNTS];


#if _MULTI_PARTITION	/* Volume - Partition resolution table */

/* Physical drive number; Partition: 0:Auto detect, 1-4:Forced partition) */

PARTITION VolToPart[16] = {
	{0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0},
	{0, 0}
};

#endif


#ifdef FATFS_DEBUG

#   define DBG(x) dbglog x

static void put_rc(FRESULT rc, const char *func) {
	const char *p;
	static const char str[] =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
		"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
		
	FRESULT i;

	for (p = str, i = 0; i != rc && *p; i++) {
		while(*p++);
	}
	DBG((DBG_DEBUG, "FATFS: %s: %u FR_%s\n", func, (UINT)rc, p));
}

#else
#   define DBG(x)
#   define put_rc(r, f)
#endif

static void fatfs_set_errno(FRESULT rc) {
	switch(rc) {
		case FR_OK:					/* (0) Succeeded */
			errno = 0;
			break;
		case FR_DISK_ERR:				/* (1) A hard error occurred in the low level disk I/O layer */
			//errno = EIO; already set in driver
			break;
		case FR_INT_ERR:				/* (2) Assertion failed */
			errno = EFAULT;
			break;
		case FR_NOT_READY:			/* (3) The physical drive cannot work */
			errno = ENODEV;
			break;
		case FR_NO_FILE:				/* (4) Could not find the file */
		case FR_NO_PATH:				/* (5) Could not find the path */
			errno = ENOENT;
			break;
		case FR_INVALID_NAME:			/* (6) The path name format is invalid */
			errno = EINVAL;
			break;
		case FR_DENIED:				/* (7) Access denied due to prohibited access or directory full */
			errno = ENOSPC;
			break;
		case FR_EXIST:				/* (8) Access denied due to prohibited access */
			errno = EACCES;
			break;
		case FR_INVALID_OBJECT:		/* (9) The file/directory object is invalid */
			errno = EBADF;
			break;
		case FR_WRITE_PROTECTED:		/* (10) The physical drive is write protected */
			errno = EROFS;
			break;
		case FR_INVALID_DRIVE:		/* (11) The logical drive number is invalid */
			errno = ENXIO;
			break;
		case FR_NOT_ENABLED:			/* (12) The volume has no work area */
			errno = EIDRM;
			break;
		case FR_NO_FILESYSTEM:		/* (13) There is no valid FAT volume */
			errno = EIO;
			break;
		case FR_MKFS_ABORTED:			/* (14) The f_mkfs() aborted due to any parameter error */
			errno = EINVAL;
			break;
		case FR_TIMEOUT:				/* (15) Could not get a grant to access the volume within defined period */
			errno = ETIME;
			break;
		case FR_LOCKED:				/* (16) The operation is rejected according to the file sharing policy */
			errno = EAGAIN;
			break;
		case FR_NOT_ENOUGH_CORE:		/* (17) LFN working buffer could not be allocated */
			errno = ENOMEM;
			break;
		case FR_TOO_MANY_OPEN_FILES:	/* (18) Number of open files > _FS_SHARE */
			errno = EMFILE;
			break;
		case FR_INVALID_PARAMETER:	/* (19) Given parameter is invalid */
			errno = EINVAL;
			break;
		default:
			errno = 0;
			break;
	}
}


#define FAT_GET_HND(hnd, rv)              \
    file_t fd = ((file_t)hnd) - 1;        \
    fatfs_t *sf = NULL;                   \
                                          \
    mutex_lock(&fat_mutex);               \
                                          \
    if(fd > -1 && fd < MAX_FAT_FILES) {   \
        sf = &fh[fd];                     \
    } else {                              \
        errno = ENFILE;                   \
        mutex_unlock(&fat_mutex);         \
        return rv;                        \
    }



static void *fat_open(vfs_handler_t * vfs, const char *fn, int flags) {
  
	file_t fd;
	fatfs_t *sf;
	fatfs_mnt_t *mnt;
	FRESULT rc;
	int fat_flags = 0, mode = (flags & O_MODE_MASK);
	
	mutex_lock(&fat_mutex);
	mnt = (fatfs_mnt_t*)vfs->privdata;
	
	if(mnt == NULL) {
		mutex_unlock(&fat_mutex);
		dbglog(DBG_ERROR, "FATFS: Error, not mounted.\n");
		errno = ENOMEM;
		return NULL;
	}
	
	for(fd = 0; fd < MAX_FAT_FILES; ++fd) {
        if(fh[fd].used == 0) {
			sf = &fh[fd];
			break;
        }
    }

    if(fd >= MAX_FAT_FILES) {
		errno = ENFILE;
		mutex_unlock(&fat_mutex);
		dbglog(DBG_ERROR, "FATFS: The maximum number of opened files exceeded.\n");
		return NULL;
    }
    
	memset_sh4(sf, 0, sizeof(fatfs_t));
	rc = f_chdrive(mnt->dev_path);

	if(rc != FR_OK) {
		dbglog(DBG_ERROR, "FATFS: Error change drive to - %s\n", mnt->dev_path);
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return NULL;
	}

	sf->mode = flags;
	sf->mnt = mnt;

	/* Directory */
	if(flags & O_DIR) {
		
		DBG((DBG_DEBUG, "FATFS: Opening directory - %s%s\n", mnt->dev_path, fn));
		rc = f_opendir(&sf->dir, (const TCHAR*)(fn == NULL ? "/" : fn));
		
		if(rc != FR_OK) {
			DBG((DBG_ERROR, "FATFS: Can't open directory - %s%s\n", mnt->dev_path, fn));
			put_rc(rc, __func__);
			fatfs_set_errno(rc);
			mutex_unlock(&fat_mutex);
			return NULL;
		}

		sf->used = 1;
		sf->type = STAT_TYPE_DIR;
		mutex_unlock(&fat_mutex);

		return (void *)(fd + 1);
	}

	/* File */
	switch(mode) {
		case O_RDONLY:
		
			fat_flags = (FA_OPEN_EXISTING | FA_READ);
			break;

		case O_WRONLY:
		
			fat_flags = FA_WRITE | (flags & O_TRUNC ? FA_CREATE_ALWAYS : FA_CREATE_NEW);
			break;
			
		case O_RDWR:
		
			fat_flags = (FA_WRITE | FA_READ) | (flags & O_TRUNC ? FA_CREATE_ALWAYS : FA_CREATE_NEW);
			break;
			
		default:
			DBG((DBG_ERROR, "FATFS: Uknown flags\n"));
			errno = EINVAL;
			mutex_unlock(&fat_mutex);
			return NULL;
	}


	DBG((DBG_DEBUG, "FATFS: Opening file - %s%s 0x%02x\n", mnt->dev_path, fn, (uint8)(fat_flags & 0xff)));

	sf->type = STAT_TYPE_FILE;
	rc = f_open(&sf->fil, (const TCHAR*)(fn == NULL ? "/" : fn), fat_flags);
	
	if(rc != FR_OK) {
		DBG((DBG_ERROR, "FATFS: Can't open file - %s%s\n", mnt->dev_path, fn));
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return NULL;
	}
	
	if(fat_flags & FA_WRITE) {
		f_sync(&sf->fil);
	}

	if((flags & O_APPEND) && sf->fil.fsize > 0) {
		DBG((DBG_ERROR, "FATFS: Append file...\n"));
		f_lseek(&sf->fil, sf->fil.fsize - 1);
	}

	sf->used = 1;
	mutex_unlock(&fat_mutex);
	return (void *)(fd + 1);
}



static int fat_close(void *hnd) {
	
	FAT_GET_HND(hnd, -1);
	sf->used = 0;     
	FRESULT rc = FR_OK;
	
	DBG((DBG_ERROR, "FATFS: Closing file - %d\n", fd));
	
	switch(sf->type) {
		case STAT_TYPE_FILE:
			if(sf->fil.cltbl != (DWORD*)&sf->lktbl && sf->fil.cltbl != NULL) {
				DBG((DBG_ERROR, "FATFS: Freeing linktable\n"));
				free(sf->fil.cltbl);
				sf->fil.cltbl = NULL;
			}
			rc = f_close(&sf->fil);
			break;
		case STAT_TYPE_DIR:
			rc = f_closedir(&sf->dir);
			break;
		default:
			mutex_unlock(&fat_mutex);
			return -1;
	}
	
	if(rc != FR_OK) {
		DBG((DBG_ERROR, "FATFS: Closing error\n"));
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return -1;
	}

	mutex_unlock(&fat_mutex);
	return 0;
}


static ssize_t fat_read(void *hnd, void *buffer, size_t size) {

	UINT rs = 0;
	FRESULT rc;

	FAT_GET_HND(hnd, -1);

	if(sf->fil.cltbl == NULL && (sf->mode & O_MODE_MASK) == O_RDONLY) {

		/* Using fast seek feature */
		memset_sh4(&sf->lktbl, 0, FATFS_LINK_TBL_SIZE * sizeof(DWORD));
		sf->fil.cltbl = sf->lktbl;           /* Enable fast seek feature */
		sf->lktbl[0] = FATFS_LINK_TBL_SIZE;  /* Set table size to the first item */

		/* Create CLMT */
		rc = f_lseek(&sf->fil, CREATE_LINKMAP);

		if(rc == FR_NOT_ENOUGH_CORE) {

			DBG((DBG_DEBUG, "FATFS: Creating linkmap %d < %ld, retry...", 
				FATFS_LINK_TBL_SIZE, sf->lktbl[0]));

			size_t lms = sf->fil.cltbl[0];
			sf->fil.cltbl = (DWORD*) calloc(lms, sizeof(DWORD)); 

			if(sf->fil.cltbl != NULL) {

				sf->fil.cltbl[0] = lms;
				rc = f_lseek(&sf->fil, CREATE_LINKMAP);

				if(rc != FR_OK) {
					DBG((DBG_ERROR, "FATFS: Create linkmap %d error: %d", lms, rc));
					free(sf->fil.cltbl);
					sf->fil.cltbl = NULL;
				}
			}
		} else if(rc != FR_OK) {
			sf->fil.cltbl = NULL;
			DBG((DBG_ERROR, "FATFS: Create linkmap %ld error: %d", sf->lktbl[0], rc));
		} else {
			DBG((DBG_DEBUG, "FATFS: Created linkmap %ld dwords\n", sf->lktbl[0]));
		}
	}

	rc = f_read(&sf->fil, buffer, (UINT) size, &rs);

	if(rc != FR_OK) {
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return -1;
	}

//	DBG((DBG_DEBUG, "FATFS: Read %d %d\n", size, rs));

	mutex_unlock(&fat_mutex);
	return (ssize_t) rs;
}


static ssize_t fat_write(void * hnd, const void *buffer, size_t cnt) {

	UINT bw = 0;
	FRESULT rc;
	FAT_GET_HND(hnd, -1);

	rc = f_write(&sf->fil, buffer, (UINT) cnt, &bw);

	if(rc != FR_OK) {
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return -1;
	}

//	DBG((DBG_DEBUG, "FATFS: Write %d %d\n", cnt, bw));
//	f_sync(&sf->fil);
	mutex_unlock(&fat_mutex);
	return (ssize_t)bw;
}


static off_t fat_tell(void * hnd) {
	FAT_GET_HND(hnd, -1);
	off_t off = (off_t)f_tell(&sf->fil);
	mutex_unlock(&fat_mutex);
	return off;
}


static off_t fat_seek(void * hnd, off_t offset, int whence) {
       
	FRESULT rc;
	DWORD off;
	FAT_GET_HND(hnd, -1);
    
    switch (whence) {
    	case SEEK_SET:
			off = (DWORD) offset;
    		break;
    	case SEEK_CUR:
			off = (DWORD) (sf->fil.fptr + offset);
            break;
    	case SEEK_END:
			off = (DWORD) (sf->fil.fsize + offset);
    		break;
    	default:
			errno = EINVAL;
			mutex_unlock(&fat_mutex);
    		return -1;
    }
	
//	DBG((DBG_DEBUG, "FATFS: Seeking: whence=%d req=%ld res=%ld\n", whence, offset, off));

	rc = f_lseek(&sf->fil, off);
	
	if(rc != FR_OK) {
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return -1;
	}
    
	mutex_unlock(&fat_mutex);
	return (off_t) sf->fil.fptr;
}


static size_t fat_total(void * hnd) {
	FAT_GET_HND(hnd, -1);
	size_t sz = (off_t)f_size(&sf->fil);
	mutex_unlock(&fat_mutex);
	return sz;
}


static dirent_t *fat_readdir(void * hnd) {
	
	FILINFO inf;
	FRESULT rc;
	FAT_GET_HND(hnd, NULL);

	memset_sh4(&sf->dent, 0, sizeof(dirent_t));

#if _USE_LFN
	inf.lfname = sf->dent.name;
	inf.lfsize = NAME_MAX;
#endif

	rc = f_readdir(&sf->dir, &inf);

	if(rc != FR_OK) {
		DBG((DBG_ERROR, "FATFS: Error reading directory entry\n"));
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return NULL; 
	}
	
	if(inf.fname[0] == 0) {
		mutex_unlock(&fat_mutex);
		return NULL; 
	}
    
//#ifdef FATFS_DEBUG
//#if _USE_LFN    
//    dbglog(DBG_DEBUG, "FATFS: Dir entry = %s %ld\n", (*inf.lfname ? inf.lfname : inf.fname), inf.fsize);
//#else
//    dbglog(DBG_DEBUG, "FATFS: Dir entry = %s %ld\n", inf.fname, inf.fsize);
//#endif
//#endif

	if(!*inf.lfname) {
		strncpy(sf->dent.name, inf.fname, 12);
	}

	// TODO date and time parsing
	sf->dent.time = (time_t) inf.ftime;

	if(inf.fattrib & AM_DIR) {
		sf->dent.attr = O_DIR;
		sf->dent.size = -1;
	} else {
		sf->dent.attr = 0;
		sf->dent.size = inf.fsize;
	}

//	if(inf.fattrib & AM_RDO) {
//		sf->dent.attr |= O_RDONLY;
//	} else {
//		sf->dent.attr |= O_RDWR;
//	}

	mutex_unlock(&fat_mutex);
	return &sf->dent;
}


static int fat_rewinddir(void * hnd) {

	FRESULT rc;
	FAT_GET_HND(hnd, -1);
	
	rc = f_rewinddir(&sf->dir);

	if(rc != FR_OK) {
		DBG((DBG_ERROR, "FATFS: Error rewind directory\n"));
		put_rc(rc, __func__);
		fatfs_set_errno(rc);
		mutex_unlock(&fat_mutex);
		return -1; 
	}

	mutex_unlock(&fat_mutex);
    return 0;
}


/* !=0: Sector number, 0: Failed - invalid cluster# */
DWORD clust2sect(FATFS *fs, DWORD clst);

static int fat_ioctl(void * hnd, int cmd, va_list ap) {
	
	DRESULT rc = RES_OK;
	FAT_GET_HND(hnd, -1);
	void *data = va_arg(ap, void *);

	switch(cmd) {
		case FATFS_IOCTL_GET_BOOT_SECTOR_DATA:
			rc = disk_read(sf->fil.fs->drv, (BYTE*)data, 0, 1);
			break;
		case FATFS_IOCTL_GET_FD_LBA:
		{
			DWORD lba = clust2sect(sf->fil.fs, sf->fil.sclust);

			if(lba > 0) {
				*(uint32 *)data = lba;
				rc = RES_OK;
			} else {
				rc = RES_ERROR;
			}

			break;
		}
		case FATFS_IOCTL_GET_FD_LINK_MAP:
			if(sf->fil.cltbl[0]) {
				memcpy_sh4(data, &sf->fil.cltbl, sf->fil.cltbl[0] * sizeof(DWORD));
			} else {
				memset_sh4(data, 0, sizeof(DWORD));
			}
			break;
		default:
			rc = disk_ioctl(sf->fil.fs->drv, (BYTE)cmd, data);
			break;
	}

	mutex_unlock(&fat_mutex);
	return rc == RES_OK ? 0 : -1;
}



#define FAT_GET_MNT()                      \
    FRESULT rc = FR_OK;                    \
    fatfs_mnt_t *mnt;                      \
    mutex_lock(&fat_mutex);                \
    mnt = (fatfs_mnt_t*)vfs->privdata;     \
    if(mnt == NULL)                        \
        goto error;                        \
    if(f_chdrive(mnt->dev_path) != FR_OK)  \
        goto error


static int fat_rename(struct vfs_handler * vfs, const char *fn1, const char *fn2) {
	
	FAT_GET_MNT();

	if((rc = f_rename((const TCHAR*)fn1, (const TCHAR*)fn2)) != FR_OK) {
		goto error;
	}

	mutex_unlock(&fat_mutex);
	return 0;
	
error:
	fatfs_set_errno(rc);
	put_rc(rc, __func__);
	mutex_unlock(&fat_mutex);
	return -1;
}



static int fat_unlink(struct vfs_handler * vfs, const char *fn) {
    
	FAT_GET_MNT();
	
	if((rc = f_unlink((const TCHAR*)fn)) != FR_OK) {
		goto error;
	}

	mutex_unlock(&fat_mutex);
	return 0;
	
error:
	fatfs_set_errno(rc);
	put_rc(rc, __func__);
	mutex_unlock(&fat_mutex);
	return -1;
}


static void *fat_mmap(void * hnd) {

	uint8 *data = NULL;
	int size = 0;
	int cnt = 0;

	size = fat_total(hnd);
	DBG((DBG_DEBUG, "FATFS: Mmap %d\n", size));
    
	if(size) {

		data = (uint8*) memalign(32, size);
		cnt = fat_read(hnd, data, size);

		if(cnt != size) {
			free(data);
			return NULL;
		}
		
		return (void*) data;
	}
	
	return NULL;
}


static int fat_complete(void * hnd, ssize_t * rv) {
	
	FRESULT rc;
	FAT_GET_HND(hnd, -1);

	DBG((DBG_DEBUG, "FATFS: fs_complete\n"));

	if((rc = f_sync(&sf->fil)) != FR_OK) {
		goto error;
	}
	
	mutex_unlock(&fat_mutex);
	return 0;
	
error:
	fatfs_set_errno(rc);
	put_rc(rc, __func__);
	mutex_unlock(&fat_mutex);
	return -1;
}


static int fat_mkdir(struct vfs_handler * vfs, const char * fn) {
    
	FAT_GET_MNT();
	
    if((rc = f_mkdir((const TCHAR*)fn)) != FR_OK) {
		goto error;
	}
	
	mutex_unlock(&fat_mutex); 
	return 0;
	
error:
	fatfs_set_errno(rc);
	put_rc(rc, __func__);
	mutex_unlock(&fat_mutex);
	return -1;
}


static int fat_rmdir(struct vfs_handler * vfs, const char * fn) {
    
	FAT_GET_MNT();
	
	if((rc = f_unlink((const TCHAR*)fn)) != FR_OK) {
		goto error;
	}
	
	mutex_unlock(&fat_mutex); 
	return 0;
	
error:
	fatfs_set_errno(rc);
	put_rc(rc, __func__);
	mutex_unlock(&fat_mutex);
	return -1;
}


static int fat_fcntl(void *hnd, int cmd, va_list ap) {
    int rv = -1;
    (void)ap;

	FAT_GET_HND(hnd, -1);

    switch(cmd) {
        case F_GETFL:
            rv = sf->mode;
            break;

        case F_SETFL:
        case F_GETFD:
        case F_SETFD:
            rv = 0;
            break;
        default:
            errno = EINVAL;
    }

    mutex_unlock(&fat_mutex); 
    return rv;
}



static int fat_stat(struct vfs_handler *vfs, const char *fn, struct stat *st, int flag) {

	FILINFO inf;
	FAT_GET_MNT();
	
	if((rc = f_stat((const TCHAR*)fn, &inf)) != FR_OK) {
		goto error;
	}
	
	memset_sh4(st, 0, sizeof(struct stat));
	
	st->st_dev = (dev_t)((ptr_t)vfs);
	st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
	st->st_nlink = 1;

	// TODO
	st->st_atime = inf.fdate + inf.ftime;
	st->st_mtime = inf.fdate + inf.ftime;
	st->st_ctime = inf.fdate + inf.ftime;
	
	if(inf.fattrib & AM_DIR) {

		st->st_mode |= S_IFDIR;
		
	} else {
		
		st->st_mode |= S_IFREG;
		st->st_size = inf.fsize;
		st->st_blksize = 1 << mnt->dev->l_block_size;
		st->st_blocks = inf.fsize >> mnt->dev->l_block_size;
	
		if(inf.fsize & (st->st_blksize - 1)) {
			++st->st_blocks;
		}
	}

	mutex_unlock(&fat_mutex);
	return 0;
	
error:
	fatfs_set_errno(rc);
	put_rc(rc, __func__);
	mutex_unlock(&fat_mutex);
	return -1;
}


static int fat_fstat(void *hnd, struct stat *st) {

	FAT_GET_HND(hnd, -1);
    memset_sh4(st, 0, sizeof(struct stat));
			
	st->st_nlink = 1;
	st->st_blksize = 1 << sf->mnt->dev->l_block_size;
	st->st_dev = (dev_t)((ptr_t)sf->mnt->dev);
	st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;

    if(sf->type == STAT_TYPE_DIR) {
		
		st->st_mode |= S_IFDIR;

    } else {
		
		st->st_mode |= S_IFREG;
		st->st_size = sf->fil.fsize;
		st->st_blocks = sf->fil.fsize >> sf->mnt->dev->l_block_size;

		if(sf->fil.fsize & (st->st_blksize - 1)) {
			++st->st_blocks;
		}
    }

	mutex_unlock(&fat_mutex);
	return 0;
}


int is_fat_partition(uint8 partition_type) {
	switch(partition_type) {
		case 0x04: /* 32MB */
		case 0x06: /* Over 32 to 2GB */ 
			return 16;
		case 0x0B:
		case 0x0C:
			return 32;
		default:
			return 0;
	}
}


#define FAT_GET_MOUNT \
	fatfs_mnt_t *mnt = NULL; \
	if(pdrv < MAX_FAT_MOUNTS && fat_mnt[pdrv].dev != NULL) { \
		mnt = &fat_mnt[pdrv]; \
	} else { \
		DBG((DBG_ERROR, "FATFS: %s[%d] pdrv error\n", __func__, pdrv)); \
		return STA_NOINIT; \
	}


DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
) {
	FAT_GET_MOUNT;

	if(mnt->dev->init(mnt->dev) < 0) {
		mnt->dev_stat |= STA_NOINIT;
	} else {
		mnt->dev_stat &= ~STA_NOINIT;
	}

	DBG((DBG_DEBUG, "FATFS: %s[%d] 0x%02x\n", __func__, pdrv, mnt->dev_stat));
	return mnt->dev_stat;
}


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
) {
	FAT_GET_MOUNT;
//	DBG((DBG_DEBUG, "FATFS: %s[%d] 0x%02x\n", __func__, pdrv, mnt->dev_stat));
	return mnt->dev_stat;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	DWORD count		/* Number of sectors to read */
) {
	FAT_GET_MOUNT;
	uint8 *dest = buff;

#ifdef FATFS_DMA
	if(mnt->dma && ((uint32)buff & 0x1F) && count <= mnt->fs->csize) {
		dest = mnt->dmabuf;
	}
#endif

	DBG((DBG_DEBUG, "FATFS: %s[%d] %ld %d 0x%08lx 0x%08lx\n", 
		__func__, pdrv, sector, (int)count, (uint32)buff, (uint32)dest));

	if(mnt->dev->read_blocks(mnt->dev, sector, count, dest) < 0) {
		DBG((DBG_ERROR, "FATFS: %s[%d] dma error: %d\n", __func__, pdrv, errno));
		return (errno == EOVERFLOW ? RES_PARERR : RES_ERROR);
	}
	if (dest != buff) {
		memcpy_sh4(buff, dest, count << mnt->dev->l_block_size);
	}
	return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	DWORD count			/* Number of sectors to write */
)
{
	FAT_GET_MOUNT;
	uint8 *src = (uint8 *)buff;
	int rc;

#ifdef FATFS_DMA
	if(mnt->dma && ((uint32)buff & 0x1F) && count <= mnt->fs->csize) {
		memcpy_sh4(mnt->dmabuf, buff, count << mnt->dev->l_block_size);
		src = mnt->dmabuf;
	}
#endif

	DBG((DBG_DEBUG, "FATFS: %s[%d] %ld %d 0x%08lx 0x%08lx\n", 
		__func__, pdrv, sector, (int)count, (uint32)buff, (uint32)src));

	rc = mnt->dev->write_blocks(mnt->dev, sector, count, src);

	if(rc < 0) {
		DBG((DBG_ERROR, "FATFS: %s[%d]%s error: %d\n", 
			__func__, pdrv, (mnt->dma ? " dma" : ""), errno));
		return errno == EOVERFLOW ? RES_PARERR : RES_ERROR;
	}
	return RES_OK;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
) {
	FAT_GET_MOUNT;

	switch(cmd) {
		case CTRL_SYNC:
			mnt->dev->flush(mnt->dev);
			DBG((DBG_DEBUG, "FATFS: %s[%d] Sync\n", __func__, pdrv));
			return RES_OK;
		case GET_SECTOR_COUNT:
			*(ulong*)buff = mnt->dev->count_blocks(mnt->dev);
			DBG((DBG_DEBUG, "FATFS: %s[%d] Sector count: %d\n", __func__, pdrv, *(ushort*)buff));
			return RES_OK;
		case GET_SECTOR_SIZE:
			*(ushort*)buff = (1 << mnt->dev->l_block_size);
			DBG((DBG_DEBUG, "FATFS: %s[%d] Sector size: %d\n", __func__, pdrv, *(ushort*)buff));
			return RES_OK;
		case GET_BLOCK_SIZE:
			*(ushort*)buff = (1 << mnt->dev->l_block_size);
			DBG((DBG_DEBUG, "FATFS: %s[%d] Block size: %d\n", __func__, pdrv, *(ushort*)buff));
			return RES_OK;
		case CTRL_TRIM:
			DBG((DBG_DEBUG, "FATFS: %s[%d] Trim sector\n", __func__, pdrv));
			return RES_OK;
		default:
			DBG((DBG_ERROR, "FATFS: %s[%d] Unknown control code: %d\n", __func__, pdrv, cmd));
			return RES_PARERR;
	}
}
#endif

/* This is a template that will be used for each mount */
static vfs_handler_t vh = {
	/* Name Handler */
	{
		{ 0 },                  /* name */
		0,                      /* in-kernel */
		0x00010000,             /* Version 1.0 */
		NMMGR_FLAGS_NEEDSFREE,  /* We malloc each VFS struct */
		NMMGR_TYPE_VFS,         /* VFS handler */
		NMMGR_LIST_INIT         /* list */
	},
	0, NULL,            /* no cacheing, privdata */
	fat_open,           /* open */
	fat_close,          /* close */
	fat_read,           /* read */
	fat_write,          /* write */
	fat_seek,           /* seek */
	fat_tell,           /* tell */
	fat_total,          /* total */
	fat_readdir,        /* readdir */
	fat_ioctl,          /* ioctl */
	fat_rename,         /* rename */
	fat_unlink,         /* unlink */
	fat_mmap,           /* mmap */
	fat_complete,       /* complete */
	fat_stat,           /* stat */
	fat_mkdir,          /* mkdir */
	fat_rmdir,          /* rmdir */
	fat_fcntl,          /* fcntl */
	NULL,               /* poll */
	NULL,               /* link */
	NULL,               /* symlink */
	NULL,               /* seek64 */
	NULL,               /* tell64 */
	NULL,               /* total64 */
	NULL,               /* readlink */
	fat_rewinddir,      /* rewinddir */
	fat_fstat           /* fstat */
};


int fs_fat_mount(const char *mp, kos_blockdev_t *dev, int dma, int partition) {

	fatfs_mnt_t *mnt = NULL;
	FRESULT rc;
	int i;

	if(!initted) {
		return -1;
	}

	mutex_lock(&fat_mutex);

	for(i = 0; i < MAX_FAT_MOUNTS; i++) {
		if(fat_mnt[i].dev == NULL) {
			mnt = &fat_mnt[i];
			memset_sh4(mnt, 0, sizeof(fatfs_mnt_t));
			mnt->dev_id = i;
			DBG((DBG_DEBUG, "FATFS: Mounting device %d to %s\n", mnt->dev_id, mp));
			break;
		}
    }

	if(mnt == NULL) {
		dbglog(DBG_ERROR, "FATFS: The maximum number of mounts exceeded.\n");
		goto error;
	}

	if(dev->init(dev) < 0) {
		dbglog(DBG_ERROR, "FATFS: Can't initialize block device: %d\n", errno);
		goto error;
	}

	mnt->dev = dev;
	VolToPart[mnt->dev_id].pd = mnt->dev_id;
	VolToPart[mnt->dev_id].pt = partition + 1;

	/* Create a VFS structure */
	if(!(mnt->vfsh = (vfs_handler_t *)malloc(sizeof(vfs_handler_t)))) {
		dbglog(DBG_ERROR, "FATFS: Out of memory for creating vfs handler\n");
		goto error;
	}

	memcpy_sh4(mnt->vfsh, &vh, sizeof(vfs_handler_t));
	strcpy(mnt->vfsh->nmmgr.pathname, mp);
	mnt->vfsh->privdata = mnt;

	/* Create a FATFS structure */
	if(!(mnt->fs = (FATFS *)malloc(sizeof(FATFS)))) {
		dbglog(DBG_ERROR, "FATFS: Out of memory for creating FATFS native mount structure\n");
		goto error;
	}

#ifdef FATFS_DMA
	mnt->dma = dma;
	if (mnt->dma) {
		uint8 tmp_dma[512] __attribute__((aligned(32)));
		mnt->dmabuf = &tmp_dma[0];
	}
#else
	if(dma) {
		dbglog(DBG_WARNING, "FATFS: DMA is disabled, force PIO mode.\n");
	}
#endif

	snprintf((TCHAR*)mnt->dev_path, sizeof(mnt->dev_path), "%d:", mnt->dev_id);
	rc = f_mount(mnt->fs, mnt->dev_path, 1);

#ifdef FATFS_DMA
	mnt->dmabuf = NULL;
#endif

	if(rc != FR_OK) {
		fatfs_set_errno(rc);
		dbglog(DBG_ERROR, "FATFS: Error %d in mounting a logical drive %d\n", errno, mnt->dev_id);
#ifdef FATFS_DEBUG
		put_rc(rc, __func__);
#endif
		goto error;
	}

#ifdef FATFS_DMA
	if(mnt->dma) {
		DBG((DBG_DEBUG, "FATFS: Allocating %d bytes for DMA buffer\n", mnt->fs->csize * _MAX_SS));
		if(!(mnt->dmabuf = (uint8 *)memalign(32, mnt->fs->csize * _MAX_SS))) {
			dbglog(DBG_ERROR, "FATFS: Out of memory for DMA buffer\n");
		} else {
			DBG((DBG_DEBUG, "FATFS: Allocated %d bytes for DMA buffer at %p\n",
				mnt->fs->csize * _MAX_SS, mnt->dmabuf));
		}
	}
#endif

	FATFS *fs;
	DWORD fre_clust, fre_sect, tot_sect;
	rc = f_getfree(mnt->dev_path, &fre_clust, &fs);

	/* Get total sectors and free sectors */
	tot_sect = (fs->n_fatent - 2) * fs->csize;
	fre_sect = fre_clust * fs->csize;

	if(rc == FR_OK) {
		dbglog(DBG_DEBUG, "FATFS: %lu KiB total drive space and %lu KiB available.\n",
				tot_sect / 2, fre_sect / 2);
	}

	DBG((DBG_DEBUG, "FATFS: FAT start sector: %ld\n", mnt->fs->fatbase));
	DBG((DBG_DEBUG, "FATFS: Data start sector: %ld\n", mnt->fs->database));
	DBG((DBG_DEBUG, "FATFS: Root directory start sector:  %ld\n", mnt->fs->dirbase * mnt->fs->csize));

    /* Register with the VFS */
    if(nmmgr_handler_add(&mnt->vfsh->nmmgr)) {
		dbglog(DBG_ERROR, "FATFS: Couldn't add vfs to nmmgr\n");
		goto error;
    }

    mutex_unlock(&fat_mutex);
    return 0;

error:
	if(mnt) {
		if(mnt->vfsh) {
			free(mnt->vfsh);
		}
		if(mnt->fs) {
			free(mnt->fs);
		}
		if(mnt->dev) {
			mnt->dev = NULL;
		}
#ifdef FATFS_DMA
		if(mnt->dmabuf) {
			free(mnt->dmabuf);
		}
#endif
		memset_sh4(mnt, 0, sizeof(fatfs_mnt_t));
	}
	if(dev) {
		dev->shutdown(dev);
	}
	mutex_unlock(&fat_mutex);
	return -1;
}


int fs_fat_unmount(const char *mp) {
	
	fatfs_mnt_t *mnt;
	int found = 0, rv = 0, i;

	mutex_lock(&fat_mutex);

	for(i = 0; i < MAX_FAT_MOUNTS; i++) {
		if(fat_mnt[i].vfsh != NULL && !strcmp(mp, fat_mnt[i].vfsh->nmmgr.pathname)) {
			mnt = &fat_mnt[i];
			found = 1;
			break;
		}
    }

	if(found) {
		/* TODO Check for open files */
		nmmgr_handler_remove(&mnt->vfsh->nmmgr);
		
		if(mnt) {
			if(mnt->vfsh) {
				free(mnt->vfsh);
			}
			if(mnt->fs) {
				free(mnt->fs);
			}
			if(mnt->dev) {
				mnt->dev->shutdown(mnt->dev);
				mnt->dev = NULL;
			}
#ifdef FATFS_DMA
			if(mnt->dmabuf) {
				free(mnt->dmabuf);
			}
#endif
		}
	} else {
		errno = ENOENT;
		rv = -1;
	}

	mutex_unlock(&fat_mutex);
	return rv;
}


int fs_fat_is_mounted(const char *mp) {

	int i, found = 0;

	mutex_lock(&fat_mutex);

	for(i = 0; i < MAX_FAT_MOUNTS; i++) {
		if(fat_mnt[i].vfsh != NULL && !strcmp(mp, fat_mnt[i].vfsh->nmmgr.pathname)) {
			found = i+1;
			break;
		}
    }

	mutex_unlock(&fat_mutex);
	return found;
}


int fs_fat_init(void) {

	if(initted) {
		return 0;
	}

	/* Reset mounts */
	memset_sh4(fat_mnt, 0, sizeof(fat_mnt));

	/* Reset fd's */
	memset_sh4(fh, 0, sizeof(fh));

	/* Init thread mutex */
	mutex_init(&fat_mutex, MUTEX_TYPE_NORMAL);

	initted = 1;
	return 0;
}

int fs_fat_shutdown(void) {

	int i;
	fatfs_mnt_t *mnt;

    if(!initted) {
        return 0;
	}

	for(i = 0; i < MAX_FAT_MOUNTS; i++) {

		if(fat_mnt[i].dev != NULL) {

			mnt = &fat_mnt[i];
			nmmgr_handler_remove(&mnt->vfsh->nmmgr);

			if(mnt->vfsh)
				free(mnt->vfsh);

			if(mnt->fs)
				free(mnt->fs);

			if(mnt->dev) {
				mnt->dev->shutdown(mnt->dev);
				mnt->dev = NULL;
			}
#ifdef FATFS_DMA
			if(mnt->dmabuf) {
				free(mnt->dmabuf);
			}
#endif
		}
    }

    mutex_destroy(&fat_mutex);
    initted = 0;
    return 0;
}

