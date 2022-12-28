/**
 * DreamShell ISO Loader
 * ISO, CSO, CDI and GDI reader
 * (c)2009-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <mmu.h>
#ifdef HAVE_LZO
#include <minilzo.h>
#endif

#define MAX_OPEN_TRACKS 3
int iso_fd = FILEHND_INVALID;
static int _iso_fd[MAX_OPEN_TRACKS] = {FILEHND_INVALID, FILEHND_INVALID, FILEHND_INVALID};
static uint16 b_seek = 0, a_seek = 0;

#ifdef HAVE_LZO
static int _open_ciso();
static int _read_ciso_sectors(uint8 *buff, uint sector, uint cnt);
#endif
static int _read_data_sectors(uint8 *buff, uint sector, uint cnt, fs_callback_f *cb);

static void _open_iso() {
		
	gd_state_t *GDS = get_GDS();
	
	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI) {
		
		DBGFF("track=%d fd=%d fd[0]=%d fd[1]=%d fd[2]=%d\n",
				GDS->data_track, iso_fd, _iso_fd[0], _iso_fd[1], _iso_fd[2]);

		/**
		 * This magic for GDI with 2 data tracks (keep open both).
		 * Also keep open first track if we can use additional fd.
		 */
		if(GDS->data_track < 3 && _iso_fd[0] > -1) {

			iso_fd = _iso_fd[0];
			return;

		} else if(GDS->data_track == 3 && _iso_fd[1] > -1) {
			
			iso_fd = _iso_fd[1];
			
			if(IsoInfo->emu_cdda && _iso_fd[0] > -1) {
				close(_iso_fd[0]);
				_iso_fd[0] = -1;
			}
			return;

		} else if(GDS->data_track > 3 && _iso_fd[2] > -1) {
			
			iso_fd = _iso_fd[2];
			
			if(IsoInfo->emu_cdda && _iso_fd[0] > -1) {
				close(_iso_fd[0]);
				_iso_fd[0] = -1;
			}
			return;
		}
	}
	
	LOGF("Opening file: %s\n", IsoInfo->image_file);
	iso_fd = open(IsoInfo->image_file, O_RDONLY);
	
	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI) {

		if(GDS->data_track < 3) {
			_iso_fd[0] = iso_fd;
		} else if(GDS->data_track == 3) {
			_iso_fd[1] = iso_fd;
		} else if(GDS->data_track > 3) {
			_iso_fd[2] = iso_fd;
		}
	}
}


int InitReader() {

	iso_fd = FILEHND_INVALID;
	for(int i = 0; i < MAX_OPEN_TRACKS; ++i) {
		_iso_fd[i] = FILEHND_INVALID;
	}

	if(fs_init() < 0) {
		return 0;
	}

	gd_state_t *GDS = get_GDS();
	memset(GDS, 0, sizeof(gd_state_t));
	GDS->lba = 150;

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI) {
		int len = strlen(IsoInfo->image_file);
		IsoInfo->image_file[len - 6] = '0';
		IsoInfo->image_file[len - 5] = '3';
		GDS->data_track = 3;
	} else {
		GDS->data_track = 1;
	}

	_open_iso();

	if(iso_fd < 0) {
#ifdef LOG
		printf("Error %d, can't open:\n%s\n", iso_fd, IsoInfo->image_file);
#else
		printf("Error, can't open: \n");
		printf(IsoInfo->image_file);
		printf("\n");
#endif
		return 0;
	}

#ifdef HAVE_LZO
	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_ZSO || IsoInfo->image_type == ISOFS_IMAGE_TYPE_CSO) {

		LOGF("Opening CISO...\n");

		if(_open_ciso() < 0) {
			printf("Error, can't open CISO image\n");
			return 0;
		}
	}
#endif

	/* Setup sector info of image */
	switch(IsoInfo->sector_size) {
		case 2324: /* MODE2_FORM2 */
			b_seek = 16;
			a_seek = 260;
			break;
		case 2336: /* SEMIRAW_MODE2 */
			b_seek = 8;
			a_seek = 280;
			break;
		case 2352: /* RAW_XA */
			b_seek = 16;
			a_seek = 288;
			break;
		default:
			b_seek = 0;
			a_seek = 0;
			break;
	}

	return 1;
}


void switch_gdi_data_track(uint32 lba, gd_state_t *GDS) {

	if(lba < IsoInfo->track_lba[0] && GDS->data_track != 1) {

		int len = strlen(IsoInfo->image_file);
		IsoInfo->image_file[len - 6] = '0';
		IsoInfo->image_file[len - 5] = '1';
		GDS->data_track = 1;
		_open_iso();

	} else if((IsoInfo->track_lba[0] == IsoInfo->track_lba[1] || lba < IsoInfo->track_lba[1]) && GDS->data_track != 3) {

		int len = strlen(IsoInfo->image_file);
		IsoInfo->image_file[len - 6] = '0';
		IsoInfo->image_file[len - 5] = '3';
		GDS->data_track = 3;
		_open_iso();

	} else if(lba >= IsoInfo->track_lba[1] && GDS->data_track <= 3) {

		int len = strlen(IsoInfo->image_file);
		IsoInfo->image_file[len - 6] = IsoInfo->image_second[5];
		IsoInfo->image_file[len - 5] = IsoInfo->image_second[6];

		uint8 n = (IsoInfo->image_second[5] - '0') & 0xf;
		GDS->data_track = n * 10;
		n = (IsoInfo->image_second[6] - '0');
		GDS->data_track += n;
		_open_iso();
	}

	DBGFF("%d\n", GDS->data_track);
}


int ReadSectors(uint8 *buf, int sec, int num, fs_callback_f *cb) {

	DBGFF("%d from %d\n", num, sec);

	int rv;
	gd_state_t *GDS = get_GDS();
	GDS->lba = sec + num;

	switch(IsoInfo->image_type) {
#ifdef HAVE_LZO
		case ISOFS_IMAGE_TYPE_CSO:
		case ISOFS_IMAGE_TYPE_ZSO:
			rv = _read_ciso_sectors(buf, sec - IsoInfo->track_lba[0], num);
			break;
#endif /* HAVE_LZO */
		case ISOFS_IMAGE_TYPE_CDI:

			rv = _read_data_sectors(buf, sec - IsoInfo->track_lba[0], num, cb);
			break;

		case ISOFS_IMAGE_TYPE_GDI:
		{
			uint32 lba = 0;

			if(sec) {

				switch_gdi_data_track(sec, GDS);
				lba = sec - ( (uint32)sec < IsoInfo->track_lba[0] ? 150 : IsoInfo->track_lba[(GDS->data_track == 3 ? 0 : 1)] );

				/* Check for data exists */
				if(GDS->data_track > 3 && lba > IsoInfo->track_lba[1] + (total(iso_fd) / IsoInfo->sector_size)) {
					LOGFF("ERROR! Track %d LBA %d\n", GDS->data_track, sec);
					return COMPLETED;
				}
			}

			rv = _read_data_sectors(buf, lba, num, cb);
			break;
		}
		case ISOFS_IMAGE_TYPE_ISO:
		default:
		{
			size_t offset = (sec - IsoInfo->track_lba[0]) * IsoInfo->sector_size;
			size_t len = num * IsoInfo->sector_size;

			lseek(iso_fd, offset, SEEK_SET);

			if(cb != NULL) {
#ifdef _FS_ASYNC
				if(read_async(iso_fd, buf, len, cb) < 0) {
					rv = FAILED;
				} else {
					rv = PROCESSING;
				}
#else
				rv = FAILED;
#endif /* _FS_ASYNC */

			} else {
				if(read(iso_fd, buf, len) < 0) {
					rv = FAILED;
				} else {
					rv = COMPLETED;
				}
			}
			break;
		}
	}
	return rv;
}


int PreReadSectors(int sec, int num) {

	DBGFF("%d from %d\n", num, sec);

	gd_state_t *GDS = get_GDS();
	GDS->lba = sec + num;

	uint32 lba = IsoInfo->track_lba[0];

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI) {
		switch_gdi_data_track(sec, GDS);
		lba = ( (uint32)sec < IsoInfo->track_lba[0] ? 150 : IsoInfo->track_lba[(GDS->data_track == 3 ? 0 : 1)] );
	}

	lseek(iso_fd, (sec - lba) * IsoInfo->sector_size, SEEK_SET);

	if(pre_read(iso_fd, num * IsoInfo->sector_size) < 0) {
		return FAILED;
	}
	return PROCESSING;
}


static int _read_sector_by_sector(uint8 *buff, uint cnt, uint sec_size
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
, int old_dma
#endif
) {

	while(cnt-- > 0) {

		lseek(iso_fd, b_seek, SEEK_CUR);

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		if(!cnt && old_dma) {
			fs_enable_dma(old_dma);
		}
#endif

		if(read(iso_fd, buff, sec_size) < 0) {
			return FAILED;
		}

		lseek(iso_fd, a_seek, SEEK_CUR);
		buff += sec_size;
	}

	return COMPLETED;
}


static int _read_data_sectors(uint8 *buff, uint sector, uint cnt, fs_callback_f *cb) {

	const uint sec_size = 2048;
	int tmps = sec_size * cnt;

	lseek(iso_fd, IsoInfo->track_offset + (sector * IsoInfo->sector_size), SEEK_SET);

	/* Reading normal data sectors (2048) */
	if(IsoInfo->sector_size == sec_size) {
		if(cb != NULL) {
#ifdef _FS_ASYNC
			if(read_async(iso_fd, buff, tmps, cb) < 0) {
				return FAILED;
			}
			return PROCESSING;
#else
			return FAILED;
#endif

		} else {
			if(read(iso_fd, buff, tmps) < 0) {
				return FAILED;
			}
		}
		return COMPLETED;
	}

	/* Reading not optimized GDI or CDI */
	uint8 *tmpb;

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	int old_dma = fs_dma_enabled();

	if(old_dma) {
		fs_enable_dma(FS_DMA_HIDDEN);
	}

	if(mmu_enabled()) {
		return _read_sector_by_sector(buff, cnt, sec_size, old_dma);
	}
#endif

	while(cnt > 2) {

		tmpb = buff;
		tmps = (tmps / IsoInfo->sector_size);

		if(read(iso_fd, tmpb, tmps * IsoInfo->sector_size) < 0) {
			return FAILED;
		}

		while(tmps--) {
			memmove(buff, tmpb + b_seek, sec_size);
			tmpb += IsoInfo->sector_size;
			buff += sec_size;
			cnt--;
		}

		tmps = sec_size * cnt;
	}

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	return _read_sector_by_sector(buff, cnt, sec_size, old_dma);
#else
	return _read_sector_by_sector(buff, cnt, sec_size);
#endif
}


#ifdef HAVE_LZO

#define isCompressed(block) ((block & 0x80000000) == 0)
#define getPosition(block) ((block & 0x7FFFFFFF) << IsoInfo->ciso.align)

static struct {
	
	uint cnt;
	uint pkg;
	
	uint8 *p_buff;
	uint32 p_buff_size;
	
	uint8 *c_buff;
	uint32 c_buff_addr;
	uint32 c_buff_size;
	void *c_buff_alloc;
	
	uint *blocks;
	fs_callback_f *cb;
	
} cst;

static int _open_ciso() {
	
	DBGF("Magic: %c%c%c%c\n", 
				IsoInfo->ciso.magic[0], 
				IsoInfo->ciso.magic[1], 
				IsoInfo->ciso.magic[2], 
				IsoInfo->ciso.magic[3]);
	DBGF("Hdr size: %u\n", IsoInfo->ciso.header_size);
	DBGF("Total bytes: %u\n", IsoInfo->ciso.total_bytes);
	DBGF("Block size: %u\n", IsoInfo->ciso.block_size);
	DBGF("Version: %02x\n", IsoInfo->ciso.ver);
	DBGF("Align: %d\n", 1 << IsoInfo->ciso.align);

	if(IsoInfo->ciso.magic[0] == 'C') { 
		;
	} else if(IsoInfo->ciso.magic[0] == 'Z') {
		
		if(lzo_init() != LZO_E_OK) {
			LOGFF("lzo init failed\n");
			return -1;
		}

		cst.c_buff_size = 0x4000;
		cst.c_buff_alloc = malloc(cst.c_buff_size);

		if (!cst.c_buff_alloc) {
			return -1;
		}

	} else {
		return -1;
	}
	
	return 0;
}

static inline uint _ciso_calc_read(uint *blocks, uint cnt, size_t size, size_t *rsize) {

	lseek(iso_fd, getPosition(blocks[0]), SEEK_SET);
	
	if(cnt <= 1) {
		*rsize = getPosition(blocks[cnt]) - getPosition(blocks[0]);
		return cnt;
	}
	
	uint i;
	
	for(i = 1; i < cnt + 1; i++) {
		
		size_t len = getPosition(blocks[i]) - getPosition(blocks[0]);
		
		if(len > size) {
			break;
		} else {
			*rsize = len;
		}
	}

	i--;

	LOGFF("%ld --> %ld %ld\n", cnt, i, *rsize);
	return i;
}


static inline size_t _ciso_dec_sector(uint *blocks, uint8 *src, uint8 *dst) {
	
	uint32 rv = 0;
	uint32 len = getPosition(blocks[1]) - getPosition(blocks[0]);

	if(isCompressed(blocks[0])) {
		
#ifdef LOG_DEBUG
		DBGFF("%d %d\n", len, lzo1x_decompress(src, len, dst, &rv, NULL));
#else
		lzo1x_decompress(src, len, dst, &rv, NULL);
#endif
	} else {
		
		DBGFF("%d copy\n", len);
		memcpy(dst, src, len);
	}
	
	return len;
}


static int ciso_read_init(uint8 *buff, uint sector, uint cnt, fs_callback_f *cb) {

	cst.p_buff = buff;
	cst.cb = cb;
	cst.cnt = cnt;
	cst.pkg = 0;
	cst.c_buff_addr = (uint32)cst.c_buff_alloc;
	cst.c_buff_size = 0x4000;
	cst.p_buff_size = cnt << 2;

	uint blocks_size = cnt * sizeof(uint);
	blocks_size = ((blocks_size / 32) + 1) * 32;

	cst.blocks = (uint *)(cst.c_buff_addr);
	cst.c_buff_size -= blocks_size;
	cst.c_buff_addr += blocks_size;
	cst.c_buff = (uint8 *)cst.c_buff_addr;
	
	LOGFF("[0x%08lx %ld %ld] [0x%08lx %ld %ld]\n", 
				(uint32)buff, sector, cnt,
				cst.c_buff_addr, cst.c_buff_size, blocks_size);

	cnt++;
	
	lseek(iso_fd, sizeof(CISO_header_t) + (sector << 2), SEEK_SET);
	if(read(iso_fd, cst.blocks, cnt << 2) < 0) {
		return -1;
	}
	
	return 0;
}


static int _read_ciso_sectors(uint8 *buff, uint sector, uint cnt) {
	
	if(ciso_read_init(buff, sector, cnt, NULL) < 0) {
		return FAILED;
	}
	
	while(cst.cnt) {

		if(!cst.pkg) {
			
			uint len = 0;
			cst.c_buff = (uint8 *)cst.c_buff_addr;
			cst.pkg = _ciso_calc_read(cst.blocks, cst.cnt, cst.c_buff_size, &len);
			
			if(read(iso_fd, cst.c_buff, len) < 0) {
				return FAILED;
			}
		}

		cst.c_buff += _ciso_dec_sector(cst.blocks, cst.c_buff, cst.p_buff);
		cst.p_buff += IsoInfo->ciso.block_size;
		--cst.pkg;
		--cst.cnt;
		++cst.blocks;
	}
	
	return COMPLETED;
}

#endif /* HAVE_LZO */
