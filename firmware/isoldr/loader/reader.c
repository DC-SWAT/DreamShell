/**
 * DreamShell ISO Loader
 * ISO, CSO, CDI and GDI reader
 * (c)2009-2020 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <mmu.h>
#ifdef HAVE_LZO
#include <minilzo.h>
#endif

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
#define SECTOR_BUFFER_PAD 32
#else
#define SECTOR_BUFFER_PAD 4
#endif

int iso_fd = -1;
static int _iso_fd[3] = {-1, -1, -1};

static uint16 b_seek = 0, a_seek = 0;
uint8 *sector_buffer = NULL;
int sector_buffer_size = 0;

#ifdef HAVE_LZO
static int _open_ciso();
static int _read_ciso_sectors(uint8 *buff, uint sector, uint cnt);
//#if defined(_FS_ASYNC)
//static int _read_ciso_sectors_async(uint8 *buff, uint sector, uint cnt, fs_callback_f *cb);
//#endif
#endif
static int _read_data_sectors(uint8 *buff, uint sector, uint cnt, fs_callback_f *cb);
static int _read_data_sectors2(uint8 *buff, uint32 offset, uint32 size, fs_callback_f *cb);
static void switch_gdi_data_track(uint32 lba, gd_state_t *GDS);

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

	uint32 loader_end = loader_addr + loader_size + ISOLDR_PARAMS_SIZE + SECTOR_BUFFER_PAD;

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	loader_end = (loader_end / 32) * 32;
#endif

	sector_buffer = (uint8 *)loader_end;
	sector_buffer_size = ISOLDR_MAX_MEM_USAGE - (ISOLDR_PARAMS_SIZE + loader_size + SECTOR_BUFFER_PAD);

	/* If the loader placed at 0x8c004800 or 0x8c000100, need correct the buffer size */
	// TODO: Try dynamic buffer
	if(loader_addr <= 0x8c000100) {
		sector_buffer_size -= 0x100;
	} else if(loader_end < 0x8c010000 && (loader_end + sector_buffer_size) > 0x8c00C000) {
		sector_buffer_size -= ((loader_end + sector_buffer_size) - 0x8c00C000) - SECTOR_BUFFER_PAD;
	}

	if(fs_init() < 0) {
		return 0;
	}

#ifdef LOG
	if(sector_buffer_size < (int)IsoInfo->sector_size) {
		sector_buffer_size = IsoInfo->sector_size;
	}
#endif

	LOGF("Sector buffer at 0x%08lx size %d\n", (uint32)sector_buffer, sector_buffer_size);

	gd_state_t *GDS = get_GDS();
	memset(GDS, 0, sizeof(gd_state_t));
	GDS->data_track = IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI ? 3 : 1;
	GDS->lba = 150;

	_open_iso();

	if(iso_fd < 0) {
#ifdef LOG
		printf("Error %d, can't open %s\n", iso_fd, IsoInfo->image_file);
#else
		printf("Error, can't open ");
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


static void switch_gdi_data_track(uint32 lba, gd_state_t *GDS) {

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

#	if 0//defined(_FS_ASYNC) && defined(DEV_TYPE_SD)
			if(cb)
				rv = _read_ciso_sectors_async(buf, sec - IsoInfo->track_lba[0], num, cb);
			else
#	endif
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

			if(GDS->cmd == CMD_DMAREAD_STREAM || GDS->cmd == CMD_DMAREAD_STREAM_EX ||
				GDS->cmd == CMD_PIOREAD_STREAM || GDS->cmd == CMD_PIOREAD_STREAM_EX) {

				size_t offset = sec ? (lba * IsoInfo->sector_size) : 0;
				rv = _read_data_sectors2(buf, offset, num, cb);
				DBGFF("Stream reading, offset=%d size=%d\n", offset, num);

			} else {
				rv = _read_data_sectors(buf, lba, num, cb);
			}
			break;
		}
		case ISOFS_IMAGE_TYPE_ISO:
		default:
		{
			size_t offset;
			size_t len;

			if(!sec) {
				offset = 0;
			} else {
				offset = (sec - IsoInfo->track_lba[0]) * IsoInfo->sector_size;
			}

			if(GDS->cmd == CMD_DMAREAD_STREAM || GDS->cmd == CMD_DMAREAD_STREAM_EX ||
				GDS->cmd == CMD_PIOREAD_STREAM || GDS->cmd == CMD_PIOREAD_STREAM_EX) {
				len = num;
			} else {
				len = num * IsoInfo->sector_size;
			}

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

//	LOGF("%s: %d from %d\n", __func__, num, sec);

	gd_state_t *GDS = get_GDS();
	GDS->lba = sec + num;
	size_t offset, len = num * IsoInfo->sector_size;
	uint lba = IsoInfo->track_lba[0];

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI) {
		switch_gdi_data_track(sec, GDS);
		lba = ( (uint32)sec < IsoInfo->track_lba[0] ? 150 : IsoInfo->track_lba[(GDS->data_track == 3 ? 0 : 1)] );
	}

	offset = (sec - lba) * IsoInfo->sector_size;

	if(pre_read(iso_fd, offset, len) < 0) {
		return FAILED;
	}

	return COMPLETED;
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

	if(IsoInfo->sector_size > sec_size) {  /* If not optimized GDI or CDI */

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

//			DBGFF("size=%d count=%d buff=%p\n", tmps * IsoInfo->sector_size, tmps, tmpb);

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

	/* Reading normal data sectors (2048) */
	} else {

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
	}

	return COMPLETED;
}


static int _read_data_sectors2(uint8 *buff, uint32 offset, uint32 size, fs_callback_f *cb) {

	DBGFF("%ld %ld\n", offset, size);

	if(offset > 0) {
		lseek(iso_fd, offset, SEEK_SET);
	}

/**
 * Just save the memory, because the loader is too big
 * and this code doesn't work properly
 */
#if 0

//	gd_state_t *GDS = get_GDS();
	const uint sec_size = 2048; //GDS->gdc.sec_size;
	static uint32 last_read_remain = 0;

	/**
	 * FIXME: Need improve it, because now it's just for compability
	 * and doesn't support async reading.
	 * Also it has problems with MMU because not async reading 
	 * uses memcpy to get a part of sectors.
	 */
	if(IsoInfo->sector_size > sec_size) { /* If not optimized GDI or CDI */
		
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		int old_dma = fs_dma_enabled();
		
		if(old_dma && last_read_remain < size) {
			fs_enable_dma(FS_DMA_HIDDEN);
		}
#endif

		if(last_read_remain) {
			
			if(read(iso_fd, buff, last_read_remain > size ? size : last_read_remain) < 0) {
				return FAILED;
			}
			
			if(last_read_remain >= size) {
				last_read_remain -= size;
				
				if(!last_read_remain) {
					lseek(iso_fd, a_seek, SEEK_CUR);
				}
				
				if(cb) cb(size);
				return COMPLETED;
			}
			
			lseek(iso_fd, a_seek, SEEK_CUR);

			buff += last_read_remain;
			last_read_remain = 0;
		}
		
		int cnt = size / sec_size;
		int end = size % sec_size;
		
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		_read_sector_by_sector(buff, cnt, sec_size, end == 0 ? old_dma : 0);
#else
		_read_sector_by_sector(buff, cnt, sec_size);
#endif
		
		if(end) {
			
			lseek(iso_fd, b_seek, SEEK_CUR);
			
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
			if(old_dma) {
				fs_enable_dma(old_dma);
			}
#endif
			
			if(read(iso_fd, buff, end) < 0) {
				return FAILED;
			}
			
			last_read_remain = sec_size - end;
		}
		
		if(cb) cb(size);
		return COMPLETED;
	}
#endif /* save memory */
	
	if(cb) {
#ifdef _FS_ASYNC
		if(read_async(iso_fd, buff, size, cb) < 0) {
			return FAILED;
		}
		
		return PROCESSING;
#else
		if(read(iso_fd, buff, size) < 0) {
			return FAILED;
		}
		
		cb(size);
#endif
		
	} else if(read(iso_fd, buff, size) < 0) {
		return FAILED;
	}
	
	return COMPLETED;
}


#ifdef HAVE_LZO

#define isCompressed(block) ((block & 0x80000000) == 0)
#define getPosition(block) ((block & 0x7FFFFFFF) << IsoInfo->ciso.align)

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
			LOGFF("lzo_init() failed\n");
			return -1;
		}
		
	} else {
		return -1;
	}
	
	return 0;
}


static struct {
	
	uint cnt;
	uint pkg;
	
	uint8 *p_buff;
	uint32 p_buff_size;
	
	uint8 *c_buff;
	uint32 c_buff_addr;
	uint32 c_buff_size;
	
	uint *blocks;
	fs_callback_f *cb;
	
} cst;


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
	cst.c_buff_size = 0x4000;
	cst.p_buff_size = cnt << 2;
	
	uint len = cst.p_buff_size + sizeof(uint);

	// TODO: Try dynamic buffer
	if((uint32)IsoInfo > 0x8c010000 || (uint32)IsoInfo < 0x8c004000) {
		
		if(IsoInfo->boot_mode != BOOT_MODE_DIRECT)
			cst.c_buff_addr = 0x0CFF0000;
		else
			cst.c_buff_addr = 0x0C00C000 - cst.c_buff_size;
			
	} else {
		cst.c_buff_addr = 0x0D000000 - cst.c_buff_size - 0x18000;
	}
	
	if(len > (uint)sector_buffer_size) {
		len = ((len / 32) + 1) * 32;
		cst.blocks = (uint *)(cst.c_buff_addr);
		cst.c_buff_size -= len;
		cst.c_buff_addr += len;
	} else {
		cst.blocks = (uint *)(sector_buffer);
	}
	
	cst.c_buff = (uint8 *)cst.c_buff_addr;
	
	LOGFF("[0x%08lx %ld %ld] [0x%08lx %ld %ld]\n", 
				(uint32)buff, sector, cnt,
				cst.c_buff_addr, cst.c_buff_size, len);

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

#if 0//defined(_FS_ASYNC) && defined(DEV_TYPE_SD)

static void ciso_read_cb(size_t size) {

	DBGFF("%d\n", size);
	
	if(size > 0) {

		cst.cnt -= cst.pkg;
		
		while(cst.pkg > 0) {
			cst.c_buff += _ciso_dec_sector(cst.blocks, cst.c_buff, cst.p_buff);
			cst.p_buff += IsoInfo->ciso.block_size;
			++cst.blocks;
			--cst.pkg;
		}
		
		if(cst.cnt > 0) {
			
			size_t len;
			cst.c_buff = (uint8 *)cst.c_buff_addr;
			cst.pkg = _ciso_calc_read(cst.blocks, cst.cnt, cst.c_buff_size, &len);

			if(read_async(iso_fd, cst.c_buff, len, ciso_read_cb) < 0) {
				cst.cb(0);
			}
			
		} else {
			cst.cb(cst.p_buff_size);
		}
		
	} else {
		cst.cb(size);
	}
}

static int _read_ciso_sectors_async(uint8 *buff, uint sector, uint cnt, fs_callback_f *cb) {
	
	if(ciso_read_init(buff, sector, cnt, cb) < 0) {
		return FAILED;
	}
	
	uint len;
	cst.pkg = _ciso_calc_read(cst.blocks, cst.cnt, cst.c_buff_size, &len);
	
	if(read_async(iso_fd, cst.c_buff, len, ciso_read_cb) < 0) {
		return FAILED;
	}
	
	return COMPLETED;
}

#endif /* _FS_ASYNC */


/**
 * FIXME
 * Works incorrectly, music in games doesn't played =(
 * This algorithm doesn't use extra memory! Almost ;-)
 */
/*
#if defined(LOG) && defined(DEBUG)
static int _get_ciso_blocks(uint *blocks, uint sector, uint cnt) {
	int r = 0;
	lseek(iso_fd, sizeof(CISO_header_t) + (sector * sizeof(uint)), SEEK_SET);
	r = read(iso_fd, blocks, sizeof(uint) * cnt);
	
	for(uint i = 0; i < cnt; i+=cnt-(cnt > 2 ? 2 : 1)) {
		DBGF(" block[%d]: pos=%d len=%d %s\n", 
			i, 
			getPosition(blocks[i]), 
			getPosition(blocks[i + (i < (cnt-1) ? 1 : 0)]) - getPosition(blocks[i]), 
			isCompressed(blocks[i]) ? "compressed" : "");
	}
			
	return r;
}
#endif

static int _read_ciso_sectors(uint8 *buff, uint sector, uint cnt) {
	
	int res = cnt + 1;
	uint32 dlen = IsoInfo->ciso.block_size;
	uint32 len;
	uint32 buf_size = (IsoInfo->sector_size * cnt);
	uint32 blk_len = res * sizeof(uint);
	uint *blocks;
	uint8 *c_buff; 
	uint32 buff_addr, c_buff_addr;
	
	if(blk_len <= (uint)sector_buffer_size) {
		
		blocks = (uint *)(sector_buffer);
		
	} else {
		
		blocks = (uint *)(0x8cfe0000);
		DBGFF("warning, using memory %08lx\n", (uint32)blocks);
	}

	DBGFF("bstart=0x%08lx bend=0x%08lx bsize=%d blk=0x%08lx blk_len=%d \n", 
				(uint32)buff, (uint32)buff + buf_size, buf_size, blocks, blk_len);
	memset((void*)blocks, 0, blk_len);

#if defined(LOG) && defined(DEBUG)
	_get_ciso_blocks(blocks, sector, res);
#else
	lseek(iso_fd, sizeof(CISO_header_t) + (sector * sizeof(uint)), SEEK_SET);
	read(iso_fd, blocks, blk_len);
#endif

	len = getPosition(blocks[cnt]) - getPosition(blocks[0]);

	if(cnt == 1) {
		c_buff = sector_buffer + blk_len;
	} else {
		c_buff = buff + (buf_size - len - 1); 
	}

#if defined(LOG) && defined(DEBUG)
	uint32 sz = (cnt * IsoInfo->ciso.block_size);
	DBGF("CSO: Comp %d bytes, uncomp %d bytes, eco %d%c, 0x%08lx - 0x%08lx \n", 
			len, sz, (sz - len) / (sz / 100), '%',
			(uint32)c_buff, (uint32)c_buff + len);
#endif

	lseek(iso_fd, getPosition(blocks[0]), SEEK_SET);
	read(iso_fd, c_buff, len);

	for(uint cur = 0; cur < cnt; cur++) {

		len = getPosition(blocks[cur + 1]) - getPosition(blocks[cur]);
	
		if(isCompressed(blocks[cur])) {
			
#if defined(LOG) && defined(DEBUG)
			if(len > 2000)
			DBGF("Decomp %03d %04d 0x%08lx -> 0x%08lx \n", 
						cur, len, (uint32)c_buff, (uint32)buff);
#endif

			buff_addr = (uint32)buff;
			c_buff_addr = (uint32)c_buff;

			// Protect against overloads
			if(cnt > 1 && ((c_buff_addr - buff_addr) < dlen || c_buff_addr <= buff_addr)) {

				DBGF("Moving data 0x%08lx -> 0x%08lx \n", 
					(uint32)c_buff, (uint32)sector_buffer + blk_len);
				memcpy((sector_buffer + blk_len), c_buff, len); 
				res = lzo1x_decompress(sector_buffer + blk_len, len, buff, &dlen, NULL);
				
//				DBGF("Decomp moved data %03d %04d 0x%08lx -> 0x%08lx \n", 
//						cur, len, (uint32)sector_buffer + blk_len, (uint32)buff);
				
			} else {
				res = lzo1x_decompress(c_buff, len, buff, &dlen, NULL);
			}

			if(res != LZO_E_OK) {

				DBGF("Decomp %03d %04d 0x%08lx -> 0x%08lx error: %d %d \n", 
					cur, len, (uint32)c_buff, (uint32)buff, res, dlen);
				dlen = IsoInfo->ciso.block_size;
				// Ignore or not? That's the question =)
				// return FAILED;
			}
#if defined(LOG) && defined(DEBUG)
			else {

 				DBGF("Complete, len=%d \n", dlen);
			}
#endif

		} else {
			
			DBGF("Copy %03d %04d 0x%08lx -> 0x%08lx \n", cur, len, (uint32)c_buff, (uint32)buff);
			memcpy(buff, c_buff, dlen);
		}
		
		c_buff += len;
		buff += dlen;
	}
	
	return COMPLETED;
}
*/

#endif /* HAVE_LZO */
