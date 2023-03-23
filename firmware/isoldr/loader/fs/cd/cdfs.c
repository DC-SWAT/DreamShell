/**
 * DreamShell ISO Loader
 * ISO9660 file system
 * (c)2011-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include <mmu.h>
#include <arch/cache.h>

/* Sector buffer */
static void *cd_sector_buffer;
static int dma_mode = 0;

void fs_enable_dma(int state) {
	dma_mode = state;
}

int fs_dma_enabled() {
	return dma_mode;
}

static int read_sectors(char *buf, int sec, int num, int async) {
	return cdrom_read_sectors_ex(buf, sec, num, async, dma_mode, 0);
}

static char *strchr0(const char *s, int c) {
	while(*s!=c)
		if(!*s++)
			return NULL;
	return (char *)s;
}


/*
 * ISO9660 support functions
 */

static int ntohlp(unsigned char *ptr) {
	/* Convert the long word pointed to by ptr from big endian */
	return (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
}


static int tolower(int c) {
	if(('A' <= c) && (c <= 'Z')) {
		return c + ('a' - 'A');
	}
	return c;
}

static int fncompare(const char *fn1, int fn1len, const char *fn2, int fn2len) {
	/* Compare two filenames, disregarding verion number on fn2 if neccessary */
	while(fn2len--)
		if(!fn1len--)
			return *fn2 == ';';
		else if(tolower(*fn1++) != tolower(*fn2++))
			return 0;
	return fn1len == 0;
}


/*
 * Low file I/O
 */

static int find_root(unsigned int *psec, unsigned int *plen) {
	/* Find location and length of root directory.
	   Plain ISO9660 only.                         */

	CDROM_TOC *toc;
	int r;
	unsigned int sec;

	if((r=cdrom_reinit(2048, 0))!=0)
	{
		LOGFF("can't reinit cdrom (%d)\n", r);
		return r;
	}
	if(!(toc = cdrom_get_toc(0, 0)))
	{
		LOGFF("can't get toc (%d)\n", r);
		return r;
	}
	if((sec = cdrom_locate_data_track(toc)) == (u32)-1)
	{
		LOGFF("can't locate data track (%d)\n", FS_ERR_DIRERR);
		return FS_ERR_DIRERR;
	}
	if((r=read_sectors((char *)cd_sector_buffer, sec+16, 1, 0))!=0)
	{
		LOGFF("can't read root (%d)\n", r);
		return r;
	}
	if(memcmp((char *)cd_sector_buffer, "\001CD001", 6))
	{
		LOGFF("bad root (%d)\n", FS_ERR_DIRERR);
		return FS_ERR_DIRERR;
	}

	/* Need to add 150 to LBA to get physical sector number */
	*psec = ntohlp(((unsigned char *)cd_sector_buffer)+156+6);
	
	if (cdrom_get_dev_type(0) == 2)
		*psec += 150;
	
	*plen = ntohlp(((unsigned char *)cd_sector_buffer)+156+14);

	return 0;
}

static int low_find(unsigned int sec, unsigned int dirlen, int isdir,
                    unsigned int *psec, unsigned int *plen,
                    const char *fname, int fnlen) {
	/* Find a named entry in a directory */

	/* sec and dirlen points out the extent of the directory */

	/* psec and plen points to variables that will receive the extent
	   of the file if found                                           */

	isdir = (isdir? 2 : 0);
	while(dirlen>0) {
		unsigned int r, i;
		unsigned char *rec = (unsigned char *)cd_sector_buffer;
		if((r=read_sectors((char *)cd_sector_buffer, sec, 1, 0))!=0)
			return r;
		for(i=0; i<2048 && i<dirlen && rec[0] != 0; i += rec[0], rec += rec[0]) {
			//WriteLog("low_find: %s = %s\n", fname, rec+33);
			if((rec[25]&2) == isdir && fncompare(fname, fnlen, (char*)rec+33, rec[32])) {
				/* Entry found.  Copy start sector and length.  Add 150 to LBA. */
				*psec = ntohlp(rec+6);
				
				if (cdrom_get_dev_type(0) == 2)
					*psec += 150;
				
				*plen = ntohlp(rec+14);
				return 0;
			}
		}
		/* Not found, proceed to next sector */
		sec++;
		dirlen -= (dirlen>2048? 2048 : dirlen);
	}
	/* End of directory.  Entry not found. */
	return FS_ERR_NOFILE;
}


/* File I/O */


/* A file handle. */
static struct {
	unsigned int sec0;  /* First sector                     */
	unsigned int loc;   /* Current read position (in bytes) */
	unsigned int len;   /* Length of file (in bytes)        */
	unsigned int rcnt;
	void *rbuf;
	int async;
	fs_callback_f *poll_cb;
	int dma_mode;
} fh[MAX_OPEN_FILES];

static unsigned int root_sector = 0;
static unsigned int root_len = 0;

int open(const char *path, int oflag) {
	int fd, r;
	unsigned int sec, len;
	char *p;

	/* Find a free file handle */
	for(fd=0; fd<MAX_OPEN_FILES; fd++)
		if(fh[fd].sec0 == 0)
			break;
	if(fd>=MAX_OPEN_FILES)
		return FS_ERR_NUMFILES;

	/* Find the root directory */
	if (!root_sector) {
		if((r=find_root(&sec, &len)))
		{
			LOGFF("can't find root (%d)\n", r);
			return r;
		}
		root_sector = sec;
		root_len = len;
	} else {
		sec = root_sector;
		len = root_len;
	}
	
	int old_dma_mode = dma_mode;
	dma_mode = 0;
	
	/* If the file we want is in a subdirectory, first locate
	   this subdirectory                                      */
	while((p = strchr0(path, '/'))) {
		if(p != path)
			if((r = low_find(sec, len, 1, &sec, &len, path, p-path)))
			{
				LOGFF("can't find file %s in a subdirectory (%d)\n", path, r);
				return r;
			}
		path = p+1;
	}

	/* Locate the file in the resulting directory */
	if(*path) {
		if((r = low_find(sec, len, oflag&O_DIR, &sec, &len, path, strchr0(path, '\0')-path))) 
		{
			dma_mode = old_dma_mode;
			LOGFF("can't find file %s in resulting directory (%d)\n", path, r);
			return r;
		}
	} else {
		/* If the path ends with a slash, check that it's really
		   the dir that is wanted                                */
		if(!(oflag & O_DIR)) {
			dma_mode = old_dma_mode;
			LOGFF("can't find file %s (%d)\n", path, FS_ERR_NOFILE);
			return FS_ERR_NOFILE;
		}
	}

	/* Fill in the file handle and return the fd */
	LOGF("Opened file: %s fd=%d sec=%d len=%d\n", path, fd, sec, len);

	fh[fd].sec0 = sec;
	fh[fd].loc = 0;
	fh[fd].len = len;
	fh[fd].async = 0;
	fh[fd].poll_cb = NULL;
	fh[fd].dma_mode = -1;

	dma_mode = old_dma_mode;

	if(oflag & O_PIO) {
		fh[fd].dma_mode = FS_DMA_DISABLED;
	}
	return fd;
}

int close(int fd) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES)
		return FS_ERR_PARAM;

	LOGF("Closed file: fd=%d sec=%d len=%d\n", fd, fh[fd].sec0, fh[fd].len);

	/* Zeroing the sector number marks the handle as unused */
	fh[fd].sec0 = 0;

	if(fh[fd].async > 0) {
		abort_async(fd);
	}

	return 0;
}

int pread(int fd, void *buf, unsigned int nbyte, unsigned int offset) {
	int r, t;

	/* Check that the fd is valid */
	if(fd<0 || fd>=MAX_OPEN_FILES || fh[fd].sec0==0)
		return FS_ERR_PARAM;

	/* If the read position is beyond the end of the file,
	   return an empty read                                */
	if(offset>=fh[fd].len)
		return 0;

	/* If the full read would span beyond the EOF, shorten the read */
	if(offset+nbyte > fh[fd].len)
		nbyte = fh[fd].len - offset;

	/* Read whole sectors directly into buf if possible */
	if(nbyte>=2048 && !(offset & 2047))
		if((r = read_sectors(buf, fh[fd].sec0 + (offset>>11), nbyte>>11, 0)))
			return r;
		else {
			t = nbyte & ~2047;
			buf = ((char *)buf) + t;
			offset += t;
			nbyte &= 2047;
		}
	else
		t = 0;

	/* If all data has now been read, return */
	if(!nbyte)
		return t;

	/* Need to read parts of sectors */
	if((offset & 2047)+nbyte > 2048) {
		/* If more than one sector is involved, split the read
		   up and recurse                                      */

		DBGFF("0x%08lx %ld %ld (part)\n", (uint32)buf, 2048-(offset & 2047));

		if((r = pread(fd, buf, 2048-(offset & 2047), offset))<0)
			return r;
		else {
			t += r;
			buf = ((char *)buf) + r;
			offset += r;
			nbyte -= r;
		}

		DBGFF("0x%08lx %ld (other)\n", (uint32)buf, nbyte);

		if((r = pread(fd, buf, nbyte, offset))<0)
			return r;
		else
			t += r;
	} else {

		if (!((uint32)buf & PHYS_ADDR(RAM_START_ADDR)) && mmu_enabled()) {

			if((r = cdrom_read_sectors_part(buf, fh[fd].sec0+(offset>>11), offset, nbyte, 0))) {
				return r;
			}

		} else {
			/* Just one sector.  Read it and copy the relevant part. */
			if((r = read_sectors((char *)cd_sector_buffer, fh[fd].sec0+(offset>>11), 1, 0))) {
				return r;
			}

			memcpy(buf, ((char *)cd_sector_buffer)+(offset&2047), nbyte);
		}

		t += nbyte;
	}
	return t;
}

int read(int fd, void *buf, unsigned int nbyte) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	}

	int old_dma_mode = dma_mode;

	if (fh[fd].dma_mode > -1) {
		dma_mode = fh[fd].dma_mode;
	}

	/* Use pread to read at the current position */
	int r = pread(fd, buf, nbyte, fh[fd].loc);

	dma_mode = old_dma_mode;

	/* Update current position */
	if(r > 0)
		fh[fd].loc += r;

	return r;
}

#if _FS_READONLY == 0
int write(int fd, void *buf, unsigned int nbyte) {
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	}
	(void)buf;
	(void)nbyte;
	return FS_ERR_SYSERR;
}
#endif

int poll(int fd) {

	if(fd < 0) {
		return FS_ERR_NOFILE;
	}

	if(!fh[fd].poll_cb || fh[fd].async < 1) {
		LOGFF("error, not async fd\n");
		return 0;
	}
	
	int transfered = g1_dma_transfered();
	
	if (g1_dma_in_progress()) {
		return transfered;
	} else {
		
		fh[fd].loc += transfered;
		
		if (fh[fd].rcnt) {
			if (cdrom_read_sectors_part(fh[fd].rbuf, fh[fd].sec0 + (fh[fd].loc >> 11), fh[fd].loc % 2048, fh[fd].rcnt, 0) < 0) {
				fh[fd].async = 0;
				fs_callback_f *cb = fh[fd].poll_cb;
				fh[fd].poll_cb = NULL;
				cb(-1);
				return FS_ERR_SYSERR;
			}
			
			return transfered + fh[fd].rcnt - 32;
		}

		fs_callback_f *cb = fh[fd].poll_cb;
		fh[fd].poll_cb = NULL;
		cb(transfered);
		return 0;
	}
}

void poll_all(int err) {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if(fh[i].sec0 > 0 && fh[i].async > 0) {
			if (err) {
				fs_callback_f *cb = fh[i].poll_cb;
				fh[i].poll_cb = NULL;
				cb(err);
			} else {
				poll(i);
			}
		}
	}
}


int abort_async(int fd) {

	if(fd < 0 || fd >= MAX_OPEN_FILES)
		return FS_ERR_PARAM;

	if(fh[fd].async > 0) {
		fh[fd].async = 0;
		fh[fd].poll_cb = NULL;
		g1_dma_abort();
	}

	return 0;
}


int read_async(int fd, void *buf, unsigned int nbyte, fs_callback_f *cb) {
	
	unsigned int sector, count; 

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	}

	if(fh[fd].loc + nbyte > fh[fd].len) {
		nbyte = fh[fd].len - fh[fd].loc;
	}
	
	sector = fh[fd].sec0 + (fh[fd].loc >> 11);

	/* If DMA is not possible, just use sync pio read */
	if((uint32)buf & 0x1F) {

		cb(read(fd, buf, nbyte));
		return 0;

	} else if((fh[fd].loc & 2047) || (nbyte & 2047)) {
		
		if((fh[fd].loc & 2047) + nbyte > 2048) {

			count = (2048-(fh[fd].loc & 2047)) >> 11;
			fh[fd].rcnt = nbyte - (count << 11);
			fh[fd].rbuf = buf + (count << 11);

		} else {

			if (cdrom_read_sectors_part(buf, fh[fd].sec0 + (fh[fd].loc >> 11), fh[fd].loc % 2048, nbyte, 0) < 0) {
				fh[fd].async = 0;
				fh[fd].poll_cb = NULL;
				return FS_ERR_SYSERR;
			}
			
			fh[fd].async = 1;
			fh[fd].poll_cb = cb;
			DBGFF("offset=%d size=%d count=%d\n", fh[fd].loc, nbyte, nbyte >> 11);
			return 0;
		}

	} else {
		count = nbyte >> 11;
	}

	if (read_sectors(buf, sector, count, 1) < 0) {
		fh[fd].async = 0;
		fh[fd].poll_cb = NULL;
		return FS_ERR_SYSERR;
	}
	
	fh[fd].async = 1;
	fh[fd].poll_cb = cb;
	DBGFF("offset=%d size=%d count=%d\n", fh[fd].loc, nbyte, nbyte >> 11);
	return 0;
}


int pre_read(int fd, unsigned int size) {
	if (cdrom_pre_read_sectors(fh[fd].sec0 + (fh[fd].loc >> 11), size >> 11, 0) < 0) {
		return FS_ERR_SYSERR;
	}
	fh[fd].async = 1;
	return 0;
}


long int lseek(int fd, long int offset, int whence) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	}

	/* Update current position according to arguments */
	switch(whence) {
	case SEEK_SET:
		return fh[fd].loc = offset;
	case SEEK_CUR:
		return fh[fd].loc += offset;
	case SEEK_END:
		return fh[fd].loc = fh[fd].len + offset;
	default:
		return FS_ERR_PARAM;
	}
}

long int tell(int fd) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0)
		return FS_ERR_PARAM;

	return fh[fd].loc;
}

unsigned long total(int fd) {
	return fh[fd].len;
}

int ioctl(int fd, int cmd, void *data) {
	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	}
	switch(cmd) {
		case FS_IOCTL_GET_LBA:
		{
			memcpy(data, &fh[fd].sec0, sizeof(fh[fd].sec0));
			return 0;
		}
		default:
			return FS_ERR_PARAM;
	}
}

/* Init function */

int fs_init() {

	memset(&fh, 0, sizeof(fh));
	cd_sector_buffer = malloc(2048 + 32);

	if (!cd_sector_buffer) {
		LOGFF("Memory failed");
		return -1;
	}

	cd_sector_buffer = (void *)ALIGN32_ADDR((uint32)cd_sector_buffer);
	memset(cd_sector_buffer, 0, 2048);
	LOGFF("Sector buffer at 0x%08lx\n", (uint32)cd_sector_buffer);

	return g1_bus_init();
}
