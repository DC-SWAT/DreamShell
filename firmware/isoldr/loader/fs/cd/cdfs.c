/**
 * DreamShell ISO Loader
 * ISO9660 file system
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <asic.h>
#include <arch/cache.h>

#define G1_ATA_DMA_STATUS       0xA05F7418      /* Read/Write */
#define G1_ATA_DMA_LEND         0xA05F74F8      /* Read-only */

static int _g1_dma_irq_enabled = 0;

int g1_dma_in_progress(void) {
	return *(volatile uint32_t *)G1_ATA_DMA_STATUS;
}

uint32_t g1_dma_transfered(void) {
	return *(volatile uint32_t *)G1_ATA_DMA_LEND;
}

int g1_dma_irq_enabled() {
	return _g1_dma_irq_enabled;
}

#define MAKE_SYSCALL(rs, p1, p2, idx) \
	int (*syscall)() = (int (*)())(gdc_saved_vector); \
	rs syscall((p1), (p2), 0, (idx));

/* Reset system functions */
static void gdc_init_system() {
	MAKE_SYSCALL(/**/, 0, 0, 3);
}

/* Submit a command to the system */
static int gdc_req_cmd(int cmd, void *param) {
	MAKE_SYSCALL(return, cmd, param, 0);
}

/* Check status on an executed command */
static int gdc_get_cmd_stat(int f, void *status) {
	MAKE_SYSCALL(return, f, status, 1);
}

/* Execute submitted commands */
static void gdc_exec_server() {
	MAKE_SYSCALL(/**/, 0, 0, 2);
}

/* Check drive status and get disc type */
static int gdc_get_drv_stat(void *param) {
	MAKE_SYSCALL(return, param, 0, 4);
}

/* Set disc access mode */
static int gdc_change_data_type(void *param) {
	MAKE_SYSCALL(return, param, 0, 10);
}

/*  */
//static void gdc_g1_dma_end(uint32 func, uint32 param) {	MAKE_SYSCALL(/**/, func, param, 5); }

/*  */
static int gdc_req_dma_trans(int f, int *dmabuf) {
	MAKE_SYSCALL(return, f, dmabuf, 6);
}

/*  */
//static int gdc_check_dma_trans(int f, int *size) { MAKE_SYSCALL(return, f, size, 7); }

/* Abort the current command */
static void gdc_abort_cmd(int cmd) {
	MAKE_SYSCALL(/**/, cmd, 0, 8);
}

static int dma_enabled = 0;

/* Sector buffer */
static uint cd_sector_buffer[2048/4] __attribute__((aligned(32)));


void fs_enable_dma(int state) {
	dma_enabled = state;
}

int fs_dma_enabled() {
	return dma_enabled;
}


void* g1_dma_handler(void *passer, register_stack *stack, void *current_vector) {

#ifdef LOG
	uint32 st = ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT];
#endif

	LOGFF("IRQ: %08lx NRM: 0x%08lx EXT: 0x%08lx ERR: 0x%08lx\n",
	      *REG_INTEVT, st,
	      ASIC_IRQ_STATUS[ASIC_MASK_EXT_INT],
	      ASIC_IRQ_STATUS[ASIC_MASK_ERR_INT]);
//	dump_regs(stack);

	/* Processing filesystem */
//	poll_all(); FIXME

	if(!_g1_dma_irq_enabled) {
		(void)passer;
		(void)stack;
		_g1_dma_irq_enabled = 1;
	}

	return current_vector;
}


int g1_dma_init_irq() {

#ifdef NO_ASIC_LT
	return 0;
#else

	asic_lookup_table_entry a_entry;

	a_entry.irq = EXP_CODE_ALL;
	a_entry.mask[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
	a_entry.mask[ASIC_MASK_EXT_INT] = 0; //ASIC_EXT_GD_CMD;
	a_entry.mask[ASIC_MASK_ERR_INT] = 0; /*ASIC_ERR_G1DMA_ILLEGAL |
											ASIC_ERR_G1DMA_OVERRUN |
											ASIC_ERR_G1DMA_ROM_FLASH;*/
	a_entry.handler = g1_dma_handler;
	return asic_add_handler(&a_entry, NULL, 0);
#endif
}


/*
 * GDROM system access
 */

static int exec_cmd(int cmd, void *param) {
	int status[4];
	int f, n;

	/* Submit the command to the system */
	f = gdc_req_cmd(cmd, param);

	/* Wait for the command to complete */
	do {
		gdc_exec_server();
	} while((n = gdc_get_cmd_stat(f, status))==1);

	/* If command completed normally, return 0 */
	if(n == CMD_STAT_COMPLETED)
		return 0;

	if(n == CMD_STAT_WAIT)
		return f;

	/* Detect conditions that have special error codes */
	else switch(status[0]) {
		case 2:
			return FS_ERR_NODISK;
		case 6:
			return FS_ERR_DISKCHG;
		default:
			return FS_ERR_SYSERR;
		}
}

static int init_drive() {
	int i, r;
	unsigned int param[4];
	int cdxa;

	/*
	 * Retry the init disc command a few times if it fails,
	 * for example if the disc has been changed
	 */
	for(i=0; i<8; i++)
		if(!(r = exec_cmd(CMD_INIT, NULL)))
			break;

	/* If it failed all times, give up */
	if(r)
		return r;

	/* Check type of disc */
	gdc_get_drv_stat(param);


	/* CD/XA? */
	cdxa = (param[1] == 32);

	/* Select disc access mode */
	param[0] = 0;                    /* 0 = set, 1 = get */
	param[1] = 8192;                 /* ?                */
	param[2] = (cdxa ? 2048 : 1024); /* mode 1/2         */
	param[3] = 2048;                 /* sector size      */
	if(gdc_change_data_type(param)<0) {
		DBGFF("FS_ERR_SYSERR: %d\n", param[1]);
		return FS_ERR_SYSERR;
	}

	/* All done */
	return 0;
}


static int read_toc(CDROM_TOC *toc, int session) {
	struct {
		int session;
		void *buffer;
	} param;
	param.session = session;
	param.buffer = toc;
	return exec_cmd(CMD_GETTOC2, &param);
}

//int req_stat(void *param) {
//	return exec_cmd(CMD_REQ_STAT, param);
//}

//static int gdc_req_dma_trans(void *param) { MAKE_SYSCALL(return, param, 0, 6); }
//static int gdc_check_dma_trans(void *param) { MAKE_SYSCALL(return, param, 0, 7); }


static int read_sectors(char *buf, int sec, int num) {

	DBGFF("%s %d from %d to 0x%08lx\n",
	      (dma_enabled ? "DMA" : "PIO"), num, sec, (uint32)buf);

	/* Buffer aling workaround */
	if((uint32)buf & (dma_enabled ? 0x1F : 0x01)) {
		while(num) {
			read_sectors((char *)&cd_sector_buffer, sec, 1);
			memcpy(buf, cd_sector_buffer, 2048);
			buf += 2048;
			sec++;
			num--;
		}
		return 0;
	}

	if(dma_enabled && (((uint32_t)buf) & 0xF0000000)) {
		dcache_inval_range((uint32)buf, num * 2048);
	}

	struct {
		int sec, num;
		void *buffer;
		int dunno;
	} param;
	param.sec = sec;    /* Starting sector number */
	param.num = num;    /* Number of sectors      */
	param.buffer = buf; /* Pointer to buffer      */
	param.dunno = 0;

	return exec_cmd(dma_enabled ? CMD_DMAREAD : CMD_PIOREAD, &param);
}

static int read_sectors_async(char *buf, int sec, int num) {

	DBGFF("%s %d from %d to 0x%08lx\n",
	      (dma_enabled ? "DMA" : "PIO"), num, sec, (uint32)buf);

	/* Buffer aling workaround */
	if((uint32)buf & 0x1F) {
		dma_enabled = 0;
	}

	if(dma_enabled && (((uint32_t)buf) & 0xF0000000)) {
		dcache_inval_range((uint32)buf, num * 2048);
	}

	struct {
		int sec, num;
		void *buffer;
		int dunno;
	} param;
	param.sec = sec;    /* Starting sector number */
	param.num = num;    /* Number of sectors      */
	param.buffer = buf; /* Pointer to buffer      */
	param.dunno = 0;

	return gdc_req_cmd(dma_enabled ? CMD_DMAREAD : CMD_PIOREAD, &param);
}

static int pre_read_sectors(int sec, int num) {

	DBGFF("%s %d from %d\n", (dma_enabled ? "DMA" : "PIO"), num, sec);

	struct {
		int sec, num, dunno1, dunno2;
	} param;
	param.sec = sec;    /* Starting sector number */
	param.num = num;    /* Number of sectors      */
	param.dunno1 = 0;
	param.dunno2 = 0;

//	return gdc_req_cmd(dma_enabled ? CMD_DMAREAD_STREAM : CMD_PIOREAD_STREAM, &param);
	return exec_cmd(dma_enabled ? CMD_DMAREAD_STREAM : CMD_PIOREAD_STREAM, &param);
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


static unsigned int find_datatrack(CDROM_TOC *toc) {
	/* Find the last track which has a ctrl of 4.
	   This method should work with multisession discs. */

	int i, first, last;
	first = TOC_TRACK(toc->first);
	last = TOC_TRACK(toc->last);

	if(first < 1 || last > 99 || first > last)
		return 0;

	for(i=last; i>=first; --i) {
		if(TOC_CTRL(toc->entry[i-1])==4)
			return TOC_LBA(toc->entry[i-1]);
	}
	return 0;
}

static int find_root(unsigned int *psec, unsigned int *plen) {
	/* Find location and length of root directory.
	   Plain ISO9660 only.                         */

	CDROM_TOC toc;
	int r;
	unsigned int sec;

	if((r=init_drive())!=0)
		return r;
	if((r=read_toc(&toc, 0))!=0)
		return r;
	if(!(sec = find_datatrack(&toc)))
		return FS_ERR_DIRERR;
	if((r=read_sectors((char *)cd_sector_buffer, sec+16, 1))!=0)
		return r;
	if(memcmp((char *)cd_sector_buffer, "\001CD001", 6))
		return FS_ERR_DIRERR;

	/* Need to add 150 to LBA to get physical sector number */
	*psec = ntohlp(((unsigned char *)cd_sector_buffer)+156+6) + 150;
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
		if((r=read_sectors((char *)cd_sector_buffer, sec, 1))!=0)
			return r;
		for(i=0; i<2048 && i<dirlen && rec[0] != 0; i += rec[0], rec += rec[0]) {
			//WriteLog("low_find: %s = %s\n", fname, rec+33);
			if((rec[25]&2) == isdir && fncompare(fname, fnlen, (char*)rec+33, rec[32])) {
				/* Entry found.  Copy start sector and length.  Add 150 to LBA. */
				*psec = ntohlp(rec+6)+150;
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
	int gd_chn;
	fs_callback_f *poll_cb;
} fh[MAX_OPEN_FILES];

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
	if((r=find_root(&sec, &len)))
		return r;

	/* If the file we want is in a subdirectory, first locate
	   this subdirectory                                      */
	while((p = strchr0(path, '/'))) {
		if(p != path)
			if((r = low_find(sec, len, 1, &sec, &len, path, p-path)))
				return r;
		path = p+1;
	}

	/* Locate the file in the resulting directory */
	if(*path) {
		if((r = low_find(sec, len, oflag&O_DIR, &sec, &len, path,
		                 strchr0(path, '\0')-path)))
			return r;
	} else {
		/* If the path ends with a slash, check that it's really
		   the dir that is wanted                                */
		if(!(oflag&O_DIR))
			return FS_ERR_NOFILE;
	}

	/* Fill in the file handle and return the fd */
	//WriteLog("File: %s %d %d\n", (strchr0(path, '\0')-path), sec, len);

	fh[fd].sec0 = sec;
	fh[fd].loc = 0;
	fh[fd].len = len;
	fh[fd].gd_chn = 0;
	fh[fd].poll_cb = NULL;

	return fd;
}

int close(int fd) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES)
		return FS_ERR_PARAM;

	/* Zeroing the sector number marks the handle as unused */
	fh[fd].sec0 = 0;

	if(fh[fd].gd_chn > 0) {
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
		if((r = read_sectors(buf, fh[fd].sec0 + (offset>>11), nbyte>>11)))
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
		/* Just one sector.  Read it and copy the relevant part. */
		if((r = read_sectors((char *)cd_sector_buffer, fh[fd].sec0+(offset>>11), 1)))
			return r;

		memcpy(buf, ((char *)cd_sector_buffer)+(offset&2047), nbyte);
		t += nbyte;
	}
	return t;
}

int read(int fd, void *buf, unsigned int nbyte) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	} else {

		/* Use pread to read at the current position */
		int r = pread(fd, buf, nbyte, fh[fd].loc);

		/* Update current position */
		if(r > 0)
			fh[fd].loc += r;

		return r;
	}
}


int poll(int fd) {

	if(fd < 0)
		return FS_ERR_NOFILE;

	if(!fh[fd].poll_cb || fh[fd].gd_chn < 1) {
		return 0;
	}

	int status[4];
	int rs;

	gdc_exec_server();
	rs = gdc_get_cmd_stat(fh[fd].gd_chn, status);

	DBGFF("stat=%d size=%d err=%d\n", rs, status[2], status[0]);

	if(rs == PROCESSING)
		return status[2] ? status[2] : 32;

	fh[fd].gd_chn = 0;

	/* If command completed normally, return 0 */
	if(rs == COMPLETED) {
		fh[fd].loc += status[2];
		fh[fd].poll_cb(status[2]);
		fh[fd].poll_cb = NULL;
		return 0;
	}

	fh[fd].poll_cb(-1);
	fh[fd].poll_cb = NULL;

	/* Detect conditions that have special error codes */
	switch(status[0]) {
		case CD_STATUS_STANDBY:
			return FS_ERR_NODISK;
		case CD_STATUS_OPEN:
			return FS_ERR_DISKCHG;
		default:
			return FS_ERR_SYSERR;
	}
}


void poll_all() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if(fh[i].sec0 > 0 && fh[i].gd_chn > 0) {
			poll(i);
		}
	}
}


int abort_async(int fd) {

	if(fd < 0 || fd >= MAX_OPEN_FILES)
		return FS_ERR_PARAM;

	if(fh[fd].gd_chn > 0) {
		fh[fd].gd_chn = 0;
		fh[fd].poll_cb = NULL;
		gdc_abort_cmd(dma_enabled ? CMD_DMAREAD : CMD_PIOREAD);
	}

	return 0;
}


int read_async(int fd, void *buf, unsigned int nbyte, fs_callback_f *cb) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0) {
		return FS_ERR_PARAM;
	} else {

		/* If async is not possible, just use sync read */
		if((fh[fd].loc & 2047) || (nbyte & 2047) || (uint32)buf & 0x1F) {
			cb(read(fd, buf, nbyte));
			return 0;
		}

		if(fh[fd].loc + nbyte > fh[fd].len)
			nbyte = fh[fd].len - fh[fd].loc;

		fh[fd].gd_chn = read_sectors_async(buf, fh[fd].sec0 + (fh[fd].loc >> 11), nbyte >> 11);


		DBGFF("chn=%d offset=%d size=%d count=%d\n",
		      fh[fd].gd_chn, fh[fd].loc, nbyte, nbyte >> 11);

		/* Update current position */
		if(fh[fd].gd_chn > 0) {
			fh[fd].poll_cb = cb;
			return 0;
		}

		fh[fd].poll_cb = NULL;
		return FS_ERR_SYSERR;
	}
}

// FIXME
static int pre_read_fd = 0;

int pre_read(int fd, unsigned long offset, unsigned int size) {

	fh[fd].gd_chn = pre_read_sectors(fh[fd].sec0 + (offset >> 11), size >> 11);

	if(fh[fd].gd_chn > 0) {
		pre_read_fd = fd;
		return 0;
	}

	return FS_ERR_SYSERR;
}

//int pre_read_status(int fd) {
//	int status[4];
//	gdc_exec_server();
//	return gdc_get_cmd_stat(fh[fd].gd_chn, status);
//}

int pre_read_xfer_start(uint32_t addr, size_t bytes) {
	int dmabuf[2];
	dmabuf[0] = addr;
	dmabuf[1] = bytes;
	return gdc_req_dma_trans(fh[pre_read_fd].gd_chn, (int*)&dmabuf);
}

//int pre_read_xfer_size() {
//	int size = 0;
//	gdc_check_dma_trans(fh[pre_read_fd].gd_chn, &size);
//	return size;
//}

long int lseek(int fd, long int offset, int whence) {

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0)
		return FS_ERR_PARAM;

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

int ioctl(int fd, int request, void *data) {

	(void)request;

	/* Check that the fd is valid */
	if(fd < 0 || fd >= MAX_OPEN_FILES || fh[fd].sec0 == 0)
		return FS_ERR_PARAM;

	if(!data) {
		return fh[fd].sec0;
	}

	memcpy(data, &fh[fd].sec0, sizeof(fh[fd].sec0));
	return 0;
}

/* Init function */

int fs_init() {
	LOGF("Init gdrom system...\n");
	gdc_init_system();
	return 0;
}
