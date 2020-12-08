/**
 * DreamShell ISO Loader
 * BIOS syscalls emulation
 * (c)2009-2020 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <exception.h>
#include <asic.h>
#include <mmu.h>
#include <arch/cache.h>
#include <arch/timer.h>
#include <arch/gdb.h>


/**
 * Current GD channel state
 */
static gd_state_t _GDS;

/**
 * Getting GD channel state
 */
gd_state_t *get_GDS(void) {
	return &_GDS;
}

static void reset_GDS(gd_state_t *GDS) {
	GDS->cmd = GDS->status = GDS->requested = GDS->transfered = GDS->dma_status = 0;
	memset(&GDS->param, 0, sizeof(GDS->param));
//	memset(GDS, 0, sizeof(int) * 12);
	GDS->lba = 150;
	GDS->req_count = 0;
	GDS->drv_stat = CD_STATUS_PAUSED;
	GDS->drv_media = IsoInfo->exec.type == BIN_TYPE_KOS ? CD_CDROM_XA : CD_GDROM;
	GDS->cdda_stat = SCD_AUDIO_STATUS_NO_INFO;
	GDS->cdda_track = 0;
	GDS->err = CMD_ERR_OK;
	// GDS->err2 = 0;
	GDS->disc_change = 0;
	GDS->disc_num = 0;

	GDS->gdc.sec_size = 2048;
	GDS->gdc.mode = 2048;
	GDS->gdc.flags = 8192;
}

/* This lock function needed for access to flashrom/bootrom */
static inline void lock_gdsys_wait(void) {
	
	/* Wait GD syscalls if they in process */
	while(_GDS.status == CMD_STAT_PROCESSING)
		vid_waitvbl();
	
	/* Lock GD syscalls */
	while(lock_gdsys())
		vid_waitvbl();
	
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	/* Wait G1 DMA complete because CDDA emulation can use it */
	while(g1_dma_in_progress())
		vid_waitvbl();
#endif
}


#ifdef LOG

static const char cmd_name[48][18] = {
	{0}, {0}, {"CHECK_LICENSE"}, {0}, {"REQ_SPI_CMD"}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
	{"PIOREAD"}, {"DMAREAD"}, {"GETTOC"}, {"GETTOC2"}, 
	{"PLAY"}, {"PLAY2"}, {"PAUSE"}, {"RELEASE"}, {"INIT"}, {"DMA_ABORT"}, {"OPEN_TRAY"},
	{"SEEK"}, {"DMAREAD_STREAM"}, {"NOP"}, {"REQ_MODE"}, {"SET_MODE"}, {"SCAN_CD"},
	{"STOP"}, {"GETSCD"}, {"GETSES"}, {"REQ_STAT"}, {"PIOREAD_STREAM"},
	{"DMAREAD_STREAM_EX"}, {"PIOREAD_STREAM_EX"}, {"GETVER"}, {0}, {0}, {0}, {0}, {0}
};

static const char stat_name[8][12] = {
	{"FAILED"}, {"IDLE"},
	{"PROCESSING"}, {"COMPLETED"},
	{"STREAMING"}, {0}
};

#endif

/** 
 * Params count for commands 
 */
static uint8 cmdp[] = {	4, 4, 2, 2, 3, 3, 0, 
						0, 0, 0, 0, 1, 2, 4, 
						1, 4, 2, 0, 3, 3, 4, 
						2, 3, 3, 1 };

static inline int get_params_count(int cmd) {
	if(cmd < 16 || cmd > 40) {
		return 0;
	}
	return cmdp[cmd - 16];
}


/**
 * Get TOC syscall
 */
static void GetTOC() {
	
	gd_state_t *GDS = get_GDS();
	CDROM_TOC *toc = (CDROM_TOC*)GDS->param[1];
	GDS->transfered = sizeof(CDROM_TOC);
	
	DBGF("%s: ", __func__);
	memcpy(toc, &IsoInfo->toc, sizeof(CDROM_TOC));

	if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI || IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI || IsoInfo->track_lba[0] == 45150) {

		LOGF("Get TOC from %cDI image and prepare for session %d\n", 
				(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI ? 'C' : 'G'), GDS->param[0] + 1);

		if(GDS->param[0] == 0) { /* Session 1 */
		
			if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI || IsoInfo->track_lba[0] == 45150) {
				
				toc->first = (toc->first & 0xfff0ffff) | (1 << 16);
				toc->last  = (toc->last & 0xfff0ffff) | (2 << 16);
				
				for(int i = 2; i < 99; i++) {
					toc->entry[i] = (uint32)-1;
				}
				
				toc->leadout_sector = 0x01001A2C;
				
			} else {
				
				for(int i = 99; i > 0; i--) {
					
					if(TOC_CTRL(toc->entry[i - 1]) == 4) {
						toc->entry[i - 1] = (uint32)-1;
					}
				}
				
				int lt = (toc->last & 0x000f0000) >> 16;
				toc->last = (toc->last & 0xfff0ffff) | (--lt << 16);
			}
			
		} else { /* Session 2 */
			
			if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_CDI) {
				
				toc->entry[0] = (uint32)-1;
				
				for(int i = 99; i > 0; i--) {
					
					if(TOC_CTRL(toc->entry[i - 1]) == 4) {
						toc->first = (toc->first & 0xfff0ffff) | (i << 16);
					}
				}
				
			} else if(IsoInfo->image_type == ISOFS_IMAGE_TYPE_GDI || IsoInfo->track_lba[0] == 45150) {
				
				toc->entry[0] = (uint32)-1;
				toc->entry[1] = (uint32)-1;
			}
		}
		
	} else {
		LOGF("Custom TOC with LBA %d\n", IsoInfo->track_lba[0]);
	}

	GDS->status = CMD_STAT_COMPLETED;
}


/**
 * Get session info syscall
 */
static void get_session_info() {

	gd_state_t *GDS = get_GDS();
	uint8 *buf = (uint8 *)GDS->param[2];
	uint32 lba = IsoInfo->toc.leadout_sector;
	
	buf[0] = CD_STATUS_PAUSED;
	buf[1] = 0;
	buf[2] = 1;
	buf[3] = (lba >> 16) & 0xFF;
	buf[4] = (lba >> 8) & 0xFF;
	buf[5] = lba & 0xFF;
	GDS->transfered = 6;
	GDS->status = CMD_STAT_COMPLETED;
}


/* GD? */
static uint8 scd_all[100] = {
	0x00, 0x15, 0x00, 0x64, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40,
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40,
	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40
};

/* CD */
//static uint8 scd_all[100] = {
//	0x00, 0x15, 0x00, 0x64, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
//	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x40,
//	0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x40, 0x00,
//	0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00,
//	0x00, 0x40, 0x00, 0x00
//};

//static uint8 scd_q[14] = {
//	0x00, 0x15, 0x00, 0x0E, 0x41, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x96
//};

static uint8 scd_media[24] = {
	0x00, 0x15, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00
};

static uint8 scd_isrc[24] = {
	0x00, 0x15, 0x00, 0x18, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00
};

//static uint32 scd_delay = 0;

/**
 * Get sub channel data syscall
 */
static void get_scd() {

	gd_state_t *GDS = get_GDS();
	uint8 *buf = (uint8 *)GDS->param[2];
	uint32 offset = GDS->lba - 150;
	
	// FIXME: Some games works incorrectly without real hw delay in getting of sub channels

	switch(GDS->param[0] & 0xF) {
		
		case SCD_REQ_ALL_SUBCODE:
		
			memcpy(buf, &scd_all, scd_all[SCD_DATA_SIZE_INDEX]);
			buf[1] = GDS->cdda_stat;
			GDS->transfered = scd_all[SCD_DATA_SIZE_INDEX];
			break;
			
		case SCD_REQ_Q_SUBCODE:
		
			buf[0] = 0x00;                      // Reserved
			buf[1] = GDS->cdda_stat;            // Audio Status
			buf[2] = 0x00;                      // DATA Length MSB
			buf[3] = 0x0E;                      // DATA Length LSB
			
			if(GDS->cdda_track) {
				buf[4] = 0x01;                   // Track flags (CTRL and ADR)
				buf[5] = GDS->cdda_track;        // Track NO
				buf[6] = GDS->cdda_track;        // 00: Pause area 01-99: Index number
			} else {
				buf[4] = 0x41;                   // Track flags (CTRL and ADR)
				buf[5] = GDS->data_track;        // Track NO
				buf[6] = GDS->data_track;        // 00: Pause area 01-99: Index number
			}
			
			buf[7] = (offset >> 16) & 0xFF;
			buf[8] = (offset >> 8) & 0xFF;
			buf[9] = (offset & 0xFF);
			buf[10] = 0x00;                     // Reserved
			buf[11] = (GDS->lba >> 16) & 0xFF;
			buf[12] = (GDS->lba >> 8) & 0xFF;
			buf[13] = GDS->lba & 0xFF;

			GDS->transfered = buf[SCD_DATA_SIZE_INDEX];
			break;
			
		case SCD_REQ_MEDIA_CATALOG:
		
			memcpy(buf, &scd_media, scd_media[SCD_DATA_SIZE_INDEX]);
//			buf[0] = 0x00;                      // Reserved
			buf[1] = GDS->cdda_stat;            // Audio Status
//			buf[2] = 0x00;                      // DATA Length MSB
//			buf[3] = 0x18;                      // DATA Length LSB
//			buf[4] = 0x02;                      // Format Code
			GDS->transfered = scd_media[SCD_DATA_SIZE_INDEX];
			break;
			
		case SCD_REQ_ISRC:
			
			memcpy(buf, &scd_isrc, scd_isrc[SCD_DATA_SIZE_INDEX]);
//			buf[0] = 0x00;                      // Reserved
			buf[1] = GDS->cdda_stat;            // Audio Status
//			buf[2] = 0x00;                      // DATA Length MSB
//			buf[3] = 0x18;                      // DATA Length LSB
//			buf[4] = 0x03;                      // Format Code
			GDS->transfered = scd_isrc[SCD_DATA_SIZE_INDEX];
			break;
			
		// case SCD_REQ_RESERVED:
		// 	break;
		
		default:
			break;
	}
					
	GDS->status = CMD_STAT_COMPLETED;
}

/**
 * Request stat syscall
 */
void get_stat(void) {
	
	gd_state_t *GDS = get_GDS();
	
#ifdef HAVE_CDDA
	cdda_ctx_t *cdda = get_CDDA();
	*((uint32*)GDS->param[0]) = cdda->loop << 8 | GDS->drv_stat;
#else
	*((uint32*)GDS->param[0]) = 0 << 8 | GDS->drv_stat;
#endif
	
	if(GDS->cdda_track) {
		*((uint32*)GDS->param[1]) = GDS->cdda_track;
		*((uint32*)GDS->param[2]) = 0x01 << 24 | GDS->lba;
	} else {
		*((uint32*)GDS->param[1]) = GDS->data_track;
		*((uint32*)GDS->param[2]) = 0x41 << 24 | GDS->lba;
	}
	
	*((uint32*)GDS->param[3]) = 1; /* X (Subcode Q index) */
	GDS->status = CMD_STAT_COMPLETED;
}

/**
 * Get GDC version syscall
 */
static void get_ver_str() {
	gd_state_t *GDS = get_GDS();
	char *buf = (char *)GDS->param[0];
	memcpy(buf, "GDC Version 1.10 1999-03-31", 27);
	buf[27] = 0x02; /* Some value from GDS */
	GDS->status = CMD_STAT_COMPLETED;
}


#ifdef _FS_ASYNC

static void data_transfer_cb(size_t size) {

	gd_state_t *GDS = get_GDS();

# ifdef DEV_TYPE_SD
	if(size > 0) {
		dcache_purge_range(GDS->param[2], size);
	}
# endif

	GDS->transfered = size;
	GDS->status = (size > 0 ? CMD_STAT_COMPLETED : CMD_STAT_FAILED);

	DBGFF("%s %d\n", stat_name[GDS->status + 1], GDS->transfered);

	GDS->drv_stat = CD_STATUS_PAUSED;
	GDS->dma_status = 0;
}

#if (defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)) && defined(HAVE_EXPT)
static int dma_check_pass;
#endif

void data_transfer_true_async() {

	gd_state_t *GDS = get_GDS();
	int ps;

#if defined(DEV_TYPE_SD)
	fs_enable_dma(IsoInfo->emu_async);
#elif defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	fs_enable_dma(FS_DMA_SHARED);
# if defined(HAVE_EXPT)
	dma_check_pass = 0;
# endif
#endif

	GDS->dma_status = 1;
	GDS->status = ReadSectors((uint8 *)GDS->param[2], GDS->param[0], GDS->param[1], data_transfer_cb);

	if(GDS->status != CMD_STAT_PROCESSING) {
		LOGFF("ERROR, status %d\n", GDS->status);
		GDS->dma_status = 0;
		return;
	}

	while(GDS->dma_status) {

#if (defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)) && defined(HAVE_EXPT)
		// FIXME: dma_check_pass it's a hack for fix conflict of polling and IRQ (suddenly turned on)
		if (!exception_inited() || !g1_dma_irq_enabled() || (++dma_check_pass > 5 && !g1_dma_in_progress()))
#endif
		{
			ps = poll(iso_fd);

			if (ps < 0) {
				GDS->dma_status = 0;
				GDS->status = CMD_STAT_FAILED;
				LOGFF("ERROR, code %d\n", ps);
				break;
			} else if(ps > 0) {
				GDS->transfered = ps;
			}
		}
		
		DBGFF("%d\n", GDS->transfered);
		gdcExitToGame();
	}
}
#endif /* _FS_ASYNC */


void data_transfer_emu_async() {

	gd_state_t *GDS = get_GDS();
	uint sc, sc_size;

	if(GDS->cmd != CMD_PIOREAD) {
		GDS->dma_status = 1;
	}

	while(GDS->param[1] > 0) {

		if(GDS->param[1] <= (uint32)IsoInfo->emu_async) {
			sc = GDS->param[1];
		} else {
			sc = IsoInfo->emu_async;
		}

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		if(IsoInfo->use_dma) {
			fs_enable_dma((GDS->param[1] - sc) > 0 ? FS_DMA_HIDDEN : FS_DMA_SHARED);
		} else {
			fs_enable_dma(FS_DMA_DISABLED);
		}
#endif

		sc_size = (GDS->gdc.sec_size * sc);

		if(ReadSectors((uint8*)GDS->param[2], GDS->param[0], sc, NULL) == CMD_STAT_FAILED) {
			GDS->status = CMD_STAT_FAILED;
			return;
		}

		GDS->param[1] -= sc;
		GDS->transfered += sc_size;

		if(GDS->cmd != CMD_PIOREAD
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
			&& !IsoInfo->use_dma
#endif
		) {
			dcache_purge_range(GDS->param[2], sc_size);
		}

		if(GDS->param[1] <= 0) {
			GDS->status = CMD_STAT_COMPLETED;
			GDS->drv_stat = CD_STATUS_PAUSED;
			GDS->dma_status = 0;
			return;
		}

		GDS->param[2] += sc_size;
		GDS->param[0] += sc;
		DBGFF("%s %d %d\n", stat_name[GDS->status + 1], GDS->req_count, GDS->transfered);

		gdcExitToGame();
	}
}


/**
 * Data transfer
 */
void data_transfer() {

	gd_state_t *GDS = get_GDS();

#ifdef _FS_ASYNC
	/* Use true async DMA transfer (or pseudo async for SD) */
	if(GDS->cmd == CMD_DMAREAD && GDS->true_async
# ifdef DEV_TYPE_SD
		/* Doesn't use pseudo async for SD in some cases (improve the general loading speed) */
		&& GDS->param[1] > 1 && GDS->param[1] < 100
# endif
	) {
		data_transfer_true_async();
		return;
	}
# ifndef DEV_TYPE_SD
	else {
		fs_enable_dma((GDS->cmd == CMD_DMAREAD && IsoInfo->use_dma) ? FS_DMA_SHARED : FS_DMA_DISABLED);
	}
# endif
#endif /* _FS_ASYNC */


	/**
	 * Read if emu async is disabled or if requested 1/100 sector(s)
	 * 
	 * 100 sectors is additional optimization.
	 * It's looks like the game in loading state (request big data),
	 * so we can increase general loading speed if load it for one frame
	 */
	if(!IsoInfo->emu_async
#ifdef DEV_TYPE_SD
		|| GDS->param[1] == 1 || GDS->param[1] >= 100
#endif
	) {

		GDS->status = ReadSectors((uint8 *)GDS->param[2], GDS->param[0], GDS->param[1], NULL);
		GDS->transfered = (GDS->param[1] * GDS->gdc.sec_size);

		if(GDS->cmd != CMD_PIOREAD
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
			&& !IsoInfo->use_dma
#endif
		) {
			dcache_purge_range(GDS->param[2], GDS->param[1] * GDS->gdc.sec_size);
		}

		GDS->drv_stat = CD_STATUS_PAUSED;
		DBGFF("%s %d %d\n", stat_name[GDS->status + 1], GDS->req_count, GDS->transfered);
		return;
	}

	/* Read in PIO/DMA mode with emu async (if true async disabled or can't be used) */
	data_transfer_emu_async();
}


static void data_transfer_dma_stream() {

	gd_state_t *GDS = get_GDS();

	if(GDS->status == CMD_STAT_PROCESSING) {
		
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		fs_enable_dma(IsoInfo->use_dma ? FS_DMA_SHARED : FS_DMA_DISABLED);
#elif defined(DEV_TYPE_SD)
		fs_enable_dma(IsoInfo->emu_async);
#endif
		/* FIXME: fragmented files */
//		GDS->status = PreReadSectors(GDS->param[0], GDS->param[1]);
//
//		if(GDS->status == CMD_STAT_COMPLETED) {
			GDS->status = CMD_STAT_STREAMING;
//		}
	} else if(GDS->requested <= 0) {
		GDS->status = CMD_STAT_COMPLETED;
		GDS->drv_stat = CD_STATUS_PAUSED;
	}
}

static void data_transfer_pio_stream() {
	
	gd_state_t *GDS = get_GDS();
	
	if(!GDS->param[2]) {
		GDS->status = CMD_STAT_STREAMING;
		return;
	}
	
	GDS->transfered += GDS->param[1];
	
#ifndef DEV_TYPE_DCL
	fs_enable_dma(FS_DMA_DISABLED);
#endif
	ReadSectors((uint8 *)GDS->param[2], GDS->param[0], GDS->param[1], NULL);
	
	if(GDS->requested/* || GDS->transfered != GDS->requested*/) {
		GDS->param[0] = 0;
		GDS->status = CMD_STAT_STREAMING;
	} else {
		GDS->status = CMD_STAT_COMPLETED;
	}
	
	if(GDS->callback != 0) {
		void (*callback)() = (void (*)())(GDS->callback);
		callback(GDS->callback_param);
	}
}

/**
 * Request command syscall
 */
int gdcReqCmd(int cmd, uint32 *param) {

	int gd_chn = GDC_CHN_ERROR;
	
	if(cmd > CMD_MAX || lock_gdsys()) {
		DBGFF("already locked\n");
		return gd_chn;
	}
	
	gd_state_t *GDS = get_GDS();
		
	if(GDS->status == CMD_STAT_IDLE) {
		
		/* I just simulate BIOS code =) */
		if(GDS->req_count++ == 0) {
			GDS->req_count++;
		}
		
		GDS->cmd = cmd;
		GDS->status = CMD_STAT_PROCESSING;
		GDS->transfered = 0;
		GDS->err = CMD_ERR_OK;
		// GDS->err2 = 0;
		GDS->dma_status = 0;
		gd_chn = GDS->req_count;
		
		LOGFF("%d %s %d", cmd, (cmd_name[cmd] ? cmd_name[cmd] : "UNKNOWN"), gd_chn);
		
		for(int i = 0; i < get_params_count(cmd); i++) {
			GDS->param[i] = param[i];
			LOGF(" %08lx", param[i]);
		}
		
		LOGF("\n");

		if(cmd == CMD_PIOREAD || cmd == CMD_DMAREAD ||
			cmd == CMD_DMAREAD_STREAM || cmd == CMD_DMAREAD_STREAM_EX ||
			cmd == CMD_PIOREAD_STREAM || cmd == CMD_PIOREAD_STREAM_EX) {
			
#ifdef HAVE_CDDA
			/* Stop CDDA playback if it's used */
			if(IsoInfo->emu_cdda && GDS->cdda_stat != SCD_AUDIO_STATUS_NO_INFO) {
				CDDA_Stop();
			}
#endif

			GDS->drv_stat = CD_STATUS_PLAYING;

			if(cmd != CMD_PIOREAD && cmd != CMD_DMAREAD) {
				GDS->requested = GDS->param[1] * GDS->gdc.sec_size;
				GDS->drv_stat = CD_STATUS_PAUSED;
			}
#ifdef LOG
			else {
				uint32 lm = ((uint32)IsoInfo) & 0x0fffffff;
				uint32 rs = GDS->param[1] * GDS->gdc.sec_size;
				
				/* Protect the loader memory */
				if((GDS->param[2] <= lm && (GDS->param[2] + rs) > lm) || 
					(GDS->param[2] > lm && (GDS->param[2] + ISOLDR_MAX_MEM_USAGE) < lm)) {

					LOGFF("WARNING! Loader memory overflow: 0x%08lx\n", GDS->param[2]);
						
					/* Just force complete status */
					GDS->status = CMD_STAT_COMPLETED;
					GDS->transfered = GDS->param[1] * GDS->gdc.sec_size;
					
				} else if(!(GDS->param[2] & 0xff000000)) {

					LOGFF("WARNING! Bad buffer pointer: 0x%08lx\n", GDS->param[2]);
								
					/* Just force complete status */
					GDS->status = CMD_STAT_COMPLETED;
					GDS->transfered = GDS->param[1] * GDS->gdc.sec_size;
				}
			}
#endif
		}
	}

	unlock_gdsys();
	return gd_chn;
}


/**
 * Main loop
 */
void gdcMainLoop(void) {

	while(1) {

		gd_state_t *GDS = get_GDS();
		DBGFF(NULL);

#ifdef HAVE_EXPT
		if(!exception_inited())
#endif
		{
#ifdef HAVE_CDDA
			CDDA_MainLoop();
#endif
			apply_patch_list();
		}

		if(GDS->status == CMD_STAT_PROCESSING) {

			switch (GDS->cmd) {
				case CMD_PIOREAD:
				case CMD_DMAREAD:
					if(GDS->param[3]) {
						GDS->status = CMD_STAT_COMPLETED;
					} else {
						data_transfer();
					}
					break;
				case CMD_DMAREAD_STREAM:
					data_transfer_dma_stream();
					break;
				case CMD_DMAREAD_STREAM_EX:
					if(GDS->param[3]) {
						GDS->status = CMD_STAT_COMPLETED;
					} else {
						data_transfer_dma_stream();
					}
					break;
				case CMD_PIOREAD_STREAM:
					data_transfer_pio_stream();
					break;
				case CMD_PIOREAD_STREAM_EX:
					if(GDS->param[3]) {
						GDS->status = CMD_STAT_COMPLETED;
					} else {
						data_transfer_pio_stream();
					}
					break;
				//case CMD_GETTOC:
				case CMD_GETTOC2:
					GetTOC();
					break;
				case CMD_INIT:
					GDS->status = CMD_STAT_COMPLETED;
					break;
				case CMD_GET_VERS:
					get_ver_str();
					break;
				case CMD_GETSES:
					get_session_info();
					break;
				case CMD_GETSCD:
					get_scd();
					break;
#ifdef HAVE_CDDA
				case CMD_PLAY:
					GDS->status = CDDA_Play(GDS->param[0], GDS->param[1], GDS->param[2]);
					break;
				case CMD_PLAY2:
					GDS->status = CDDA_Play2(GDS->param[0], GDS->param[1], GDS->param[2]);
					break;
				case CMD_RELEASE:
					GDS->status = CDDA_Release();
					break;
				case CMD_PAUSE:
					GDS->status = CDDA_Pause();
					break;
				case CMD_SEEK:
					GDS->status = CDDA_Seek(GDS->param[0]);
					break;
				case CMD_STOP:
					GDS->status = CDDA_Stop();
					break;
#else
				case CMD_PLAY:
				case CMD_PLAY2:
				case CMD_RELEASE:
					GDS->status = CMD_STAT_COMPLETED;
					GDS->drv_stat = CD_STATUS_PLAYING;
					GDS->cdda_stat = SCD_AUDIO_STATUS_PLAYING;
					break;
				case CMD_PAUSE:
				case CMD_STOP:
					GDS->status = CMD_STAT_COMPLETED;
					GDS->drv_stat = CD_STATUS_PAUSED;
					GDS->cdda_stat = SCD_AUDIO_STATUS_PAUSED;
					break;
				case CMD_SEEK:
					GDS->status = CMD_STAT_COMPLETED;
					break;
#endif
				case CMD_REQ_MODE:
				case CMD_SET_MODE:
					// TODO param[0]
					GDS->status = CMD_STAT_COMPLETED;
					break;
				case CMD_REQ_STAT:
					get_stat();
					break;
				default:
					LOGF("Unhandled command %d %s, force complete status\n", 
							GDS->cmd, (cmd_name[GDS->cmd] ? cmd_name[GDS->cmd] : "UNKNOWN"));
					GDS->status = CMD_STAT_COMPLETED;
					break;
			}

		}
		gdcExitToGame();
	}
}


/**
 * Get status of the command syscall
 */
int gdcGetCmdStat(int gd_chn, uint32 *status) {

	if(lock_gdsys()) {
		DBGF("%s: already locked\n", __func__);
		return CMD_STAT_WAITING;
	}

	int rv = CMD_STAT_IDLE;
	gd_state_t *GDS = get_GDS();
	memset(status, 0, sizeof(uint32) * 4);

	if(gd_chn == 0) {

		LOGFF("WARNING: id = %d\n", gd_chn);

		if(GDS->status != CMD_STAT_IDLE) {
			rv = CMD_STAT_PROCESSING;
		}
		unlock_gdsys();
		return rv;

	} else if(gd_chn != GDS->req_count) {

		LOGFF("ERROR: %d != %d\n", gd_chn, GDS->req_count);
		status[0] = CMD_STAT_ERROR;
		rv = CMD_STAT_FAILED;
		unlock_gdsys();
		return rv;
	}

	LOGFF("%d %s\n", gd_chn, stat_name[GDS->status + 1]);

	switch(GDS->status) {
		case CMD_STAT_PROCESSING:

			status[2] = GDS->transfered;
			status[3] = GDS->dma_status;
			rv = CMD_STAT_PROCESSING;
			break;

		case CMD_STAT_COMPLETED:

			status[2] = GDS->transfered;
			status[3] = GDS->dma_status;
			GDS->status = CMD_STAT_IDLE;
			rv = CMD_STAT_COMPLETED;
			break;

		case CMD_STAT_STREAMING:

			status[2] = GDS->streamed;

			if (GDS->err != CMD_ERR_OK) {
				status[0] = GDS->err;
				status[1] = 0; //GDS->err2;
				rv = CMD_STAT_FAILED;
				break;
			}

			rv = CMD_STAT_STREAMING;
			break;

		case CMD_STAT_FAILED:
			rv = CMD_STAT_FAILED;
			break;

		default:
			break;
	}

	unlock_gdsys();
	return rv;
}


/**
 * Get status of the drive syscall
 */
int gdcGetDrvStat(uint32 *status) {
	
	if(lock_gdsys()) {
		DBGFF("already locked\n");
		return CMD_STAT_WAITING;
	}
	
 	DBGF("%s\r", __func__);
	gd_state_t *GDS = get_GDS();

	if (GDS->disc_change) {

		GDS->disc_change++;

		if (GDS->disc_change == 2) {

			GDS->drv_media = CD_CDDA;
			GDS->drv_stat = CD_STATUS_OPEN;

		} else if (GDS->disc_change > 30) {

			GDS->drv_media = IsoInfo->exec.type == BIN_TYPE_KOS ? CD_CDROM_XA : CD_GDROM;
			GDS->drv_stat = CD_STATUS_PAUSED;
			GDS->disc_change = 0;
			unlock_gdsys();
			return 2;

		} else {
			status[0] = GDS->drv_stat;
			status[1] = GDS->drv_media;
			unlock_gdsys();
			return 1;
		}
	}
	
	status[0] = GDS->drv_stat;
	status[1] = GDS->drv_media;
	unlock_gdsys();
	return 0;
}


/**
 * Set/Get data type syscall
 */
int gdcChangeDataType(int *param) {

	if(lock_gdsys()) {
		return CMD_STAT_WAITING;
	}
	
	gd_state_t *GDS = get_GDS();

	if(param[0] == 0) {
		
		GDS->gdc.flags = param[1];
		GDS->gdc.mode = param[2];
		GDS->gdc.sec_size = param[3];

	} else {
		param[1] = GDS->gdc.flags; 
		param[2] = GDS->gdc.mode;
		param[3] = GDS->gdc.sec_size;
	}
	
	LOGFF("%s: unknown=%d mode=%d sector_size=%d\n",
		(param[0] == 0 ? "SET" : "GET"), param[1], param[2], param[3]);

	unlock_gdsys();
	return 0;
}


/**
 * Initialize syscalls
 * This function calls from GDC ASM after saving registers
 */
void gdcInitSystem(void) {

#if defined(DEV_TYPE_DCL) || defined(LOG_DCL)
	/* Reinit BBA, because the game reset it */
	dcload_reinit();
#endif

#ifdef DEV_TYPE_SD
	/* Reinit SPI, because the game reinited SCIF */
	spi_init();
#else
	/* Reinit logging, because the game reinited SCIF */
	CloseLog();
	OpenLog();
#endif

	LOGFF(NULL);

	gd_state_t *GDS = get_GDS();
	reset_GDS(GDS);

	if(
#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
		IsoInfo->use_dma && !IsoInfo->emu_async &&
#elif defined(DEV_TYPE_SD)
		IsoInfo->emu_async &&
#endif
		IsoInfo->sector_size == 2048
#ifdef HAVE_LZO
		&& IsoInfo->image_type != ISOFS_IMAGE_TYPE_CSO
		&& IsoInfo->image_type != ISOFS_IMAGE_TYPE_ZSO
#endif
	) {

#if defined(DEV_TYPE_SD)
		/* Increase sectors count up to 1.5 if the emu async = 1 */
		IsoInfo->emu_async = IsoInfo->emu_async == 1 ? 6 : (IsoInfo->emu_async << 2);
#endif
		GDS->true_async = 1;
		
	} else {
		GDS->true_async = 0;
	}


#ifdef HAVE_EXPT

	if(IsoInfo->use_irq) {

		int old = irq_disable();
		
		/* Injection to exception handling */
		if (!exception_init(0)) {

			/* Use ASIC interrupts */
			asic_init();

# if defined(DEV_TYPE_GD) || defined(DEV_TYPE_IDE)
			/* Initialize G1 DMA interrupt */
			if (IsoInfo->use_dma) {
				g1_dma_init_irq();
			}
# endif
		}
		
		irq_restore(old);

# ifdef USE_GDB

		gdb_init();
		
	} else {
		
		int old = irq_disable();
		int rs = exception_init(0);
		irq_restore(old);
		
		if(!rs) {
			gdb_init();
		}
	}
# else
	}
# endif /* USE_GDB */
	
#endif /* HAVE_EXPT */

#ifdef HAVE_CDDA
	if (IsoInfo->buff_mode == BUFF_MEM_DYNAMIC) {
		if (malloc_init()) {
			LOGF("Dynamic memory enabled\n");
		} else {
			IsoInfo->buff_mode = BUFF_MEM_STATIC;
		}
	}
#endif

	gdcMainLoop();
}


/**
 * Reset syscall
 */
void gdcReset(void) {

	LOGFF(NULL);
	gd_state_t *GDS = get_GDS();
	reset_GDS(GDS);
	unlock_gdsys();
}


/**
 * Read abort syscall
 */
int gdcReadAbort(int gd_chn) {

	LOGFF("%d\n", gd_chn);
	gd_state_t *GDS = get_GDS();

	if(gd_chn != GDS->req_count) {
		return -1;
	}
	
#if _FS_ASYNC
	if(GDS->dma_status) {
		abort_async(iso_fd);
		GDS->dma_status = 0;
	}
#endif

	switch(GDS->cmd) {
		case CMD_PIOREAD:
		case CMD_DMAREAD:
		case CMD_PLAY:
		case CMD_PLAY2:
		case CMD_PAUSE:
		case CMD_SEEK:
		case CMD_NOP:
		case CMD_SCAN_CD:
		case CMD_STOP:
		case CMD_GETSCD:
		case CMD_DMAREAD_STREAM:
		case CMD_PIOREAD_STREAM:
		case CMD_DMAREAD_STREAM_EX:
		case CMD_PIOREAD_STREAM_EX:
			switch(GDS->status) {
				case CMD_STAT_PROCESSING:
				case CMD_STAT_STREAMING:
				case CMD_STAT_WAITING:
					GDS->status = CMD_STAT_COMPLETED;
					return 0;
				default:
					return 0;
			}
			break;
		default:
			break;
	}

	return -1;
}


#ifdef _FS_ASYNC
static void data_stream_cb(size_t size) {

	gd_state_t *GDS = get_GDS();

	GDS->transfered += size;
	GDS->streamed = size;

	LOGFF("%d %d %d\n", size, GDS->transfered, GDS->requested);

	if(!GDS->requested) {
		GDS->status = CMD_STAT_COMPLETED;
	}

	GDS->dma_status = 0;
}
#endif

/**
 * Request DMA transfer syscall.
 * 
 * In general this syscall should not be blocked
 * but we will work out all possible options.
 */
int gdcReqDmaTrans(int gd_chn, int *dmabuf) {
	
	gd_state_t *GDS = get_GDS();
	
	LOGFF("%d %08lx %d (%d)\n", gd_chn, (uint32)dmabuf[0], dmabuf[1], GDS->requested);

	if(gd_chn != GDS->req_count) {
		LOGF("ERROR: %d != %d\n", gd_chn, GDS->req_count);
		return -1;
	}
	
	if(GDS->status != CMD_STAT_STREAMING || GDS->requested < (uint32)dmabuf[1]) {
		LOGF("ERROR: cmd status = %d, remain = %d, request = %d\n", GDS->status, GDS->requested, dmabuf[1]);
		return -1;
	}

	GDS->requested -= dmabuf[1];
	int offset = GDS->transfered ? 0 : GDS->param[0];

#ifdef _FS_ASYNC
	/* FIXME: fragmented files */
//	pre_read_xfer_start(dmabuf[0], dmabuf[1]);

	if(GDS->true_async) {
		GDS->dma_status = 1;
		GDS->streamed = 32;
		ReadSectors((uint8 *)dmabuf[0], offset, dmabuf[1], data_stream_cb);

# if (defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)) && defined(HAVE_EXPT)
		if(exception_inited()) {
			// This is only right case, returning without waiting
			return 0;
		}
# endif
		while(GDS->dma_status) {

			GDS->streamed = pre_read_xfer_size();

			// NOTE: Just to be sure for gdcCheckDmaTrans 
			if(!GDS->streamed) {
				GDS->streamed = 32;
			}

			if(poll(iso_fd) < 0) {
				GDS->status = CMD_STAT_FAILED;
				break;
			}
		}
		return 0;
	}
#endif /*_FS_ASYNC */

	GDS->streamed = dmabuf[1];
	GDS->transfered += dmabuf[1];
	ReadSectors((uint8 *)dmabuf[0], offset, dmabuf[1], NULL);

	if(!GDS->requested) {
		GDS->status = CMD_STAT_COMPLETED;
	}

	return 0;
}


/**
 * Check DMA transfer syscall
 */
int gdcCheckDmaTrans(int gd_chn, int *size) {

	gd_state_t *GDS = get_GDS();

	LOGFF("%d %s r=%ld t=%ld s=%ld\n", gd_chn, stat_name[GDS->status + 1],
			GDS->requested, GDS->transfered, GDS->dma_status);

	if(gd_chn != GDS->req_count) {
		LOGF("ERROR: %d != %d\n", gd_chn, GDS->req_count);
		return -1;
	}
	
	if(GDS->status != CMD_STAT_STREAMING) {
		LOGF("ERROR: Incorrect status: %s\n", stat_name[GDS->status + 1]);
		return -1;
	}

#ifdef _FS_ASYNC

//	if(pre_read_xfer_busy()) {
//		*size = pre_read_xfer_size();

	if(GDS->dma_status) {
		*size = GDS->streamed;
		return 1;
	}

#endif /* _FS_ASYNC */

	*size = GDS->requested;
	return 0;
}


/**
 * DMA transfer end syscall
 */
void gdcG1DmaEnd(uint32 func, uint32 param) {
	
	LOGFF("%08lx %08lx\n", func, param);

#if defined(DEV_TYPE_GD) || defined(DEV_TYPE_IDE)
	ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = ASIC_NRM_GD_DMA;
#endif

	if(func != 0) {
		void (*callback)() = (void (*)())(func);
		callback(param);
	}
}

void gdcSetPioCallback(uint32 func, uint32 param) {
	
	LOGFF("0x%08lx %ld\n", func, param);
	gd_state_t *GDS = get_GDS();

	if(func != 0) {
		GDS->callback = func;
		GDS->callback_param = param;
	} else {
		GDS->callback = 0;
		GDS->callback_param = 0;
	}
}


int gdcReqPioTrans(int gd_chn, int *piobuf) {
	
	gd_state_t *GDS = get_GDS();
	
	LOGFF("%d 0x%08lx %d (%d)\n", gd_chn, (uint32)piobuf[0], piobuf[1], GDS->requested);

	if(gd_chn != GDS->req_count) {
		LOGF("ERROR: %d != %d\n", gd_chn, GDS->req_count);
		return -1;
	}
	
	if((GDS->status != CMD_STAT_STREAMING && GDS->status != CMD_STAT_PROCESSING) || GDS->requested < (uint32)piobuf[1]) {
		LOGF("ERROR: cmd status = %d, remain = %d, request = %d\n", GDS->status, GDS->requested, piobuf[1]);
		return -1;
	}

	GDS->requested -= piobuf[1];

	GDS->param[2] = piobuf[0];
	GDS->param[1] = piobuf[1];
	
	GDS->status = CMD_STAT_PROCESSING;
	GDS->drv_stat = CD_STATUS_PLAYING;
	return 0;
}


int gdcCheckPioTrans(int gd_chn, int *size) {

	gd_state_t *GDS = get_GDS();

	LOGFF("%d %ld %ld\n", gd_chn, GDS->requested, GDS->transfered);
	
	if(gd_chn != GDS->req_count/* || GDS->status != CMD_STAT_STREAMING*/) {
		LOGF("ERROR: %d != %d\n", gd_chn, GDS->req_count);
		return -1;
	}

	*size = GDS->requested;
	return 0;
}

void gdGdcChangeDisc(int disc_num) {
	LOGFF("%d\n", disc_num);
	gd_state_t *GDS = get_GDS();
	GDS->disc_change = 1;
	GDS->disc_num = disc_num;
}

void gdcDummy(int gd_chn, int *arg2) {
	LOGFF("%d 0x%08lx 0x%08lx\n", gd_chn, arg2[0], arg2[1]);
	(void)gd_chn;
	(void)arg2;
}


/**
 * Menu syscall
 */
int menu_syscall(int func) {

	LOGFF("%d\n", func);
	
	if(IsoInfo->boot_mode != BOOT_MODE_DIRECT) {
		switch(func) {
			case 0:
			case 1:
				// Skip exit to menu
				return 0;
			case 2:
				// Skip disk checking
				return 0;
			default:
				break;
		}
	} else if(func == 1) {
//		irq_disable();
//		Load_DS();
		volatile uint32 *reset_reg = (uint32 *)0xa05f6890, reset_val = 0x00007611;
		*reset_reg = reset_val;
	}

	if((uint32)IsoInfo >= 0x8c004000 && !is_custom_bios()) {
		int (*f)(int);
		f = (void*)(menu_saved_vector);
		return f(func);
	} else {
		return 0;
	}
}

/**
 * FlashROM syscalls
 */
int flashrom_info(int part, uint32 *info) {

	LOGFF("%d 0x%08lx\n", part, info);

	switch(part) {
		case 0: /* SYSTEM */
			info[0] = 0x1A000;
			info[1] = 0x2000;
			break;
		case 1: /* RESERVED */
			info[0] = 0x18000;
			info[1] = 0x2000;
			break;
		case 2: /* GAME INFO? */
			info[0] = 0x1C000;
			info[1] = 0x4000;
			break;
		case 3: /* SETTINGS */
			info[0] = 0x10000;
			info[1] = 0x8000;
			break;
		case 4: /* BLOCK 2 */
			info[0] = 0;
			info[1] = 0x10000;
			break;
		default:
			info[0] = info[1] = 0;
			break;
	}

	return 0;
}


static void safe_memcpy(void* dst, const void* src, size_t cnt) {
	
	uint8 *d = (uint8*)dst;
	const uint8 *s = (const uint8*)src;

	while (cnt--) {
		*d++ = *s++;
	}
}


int flashrom_read(int offset, void *buffer, int bytes) {

	LOGFF("0x%08lx 0x%08lx %d\n", offset, (uint32)buffer, bytes);

#if defined(DEV_TYPE_IDE) || defined(DEV_TYPE_GD)
	// FIXME: lock G1 bus
//	lock_gdsys_wait();
	do {} while(g1_dma_in_progress());
	safe_memcpy(buffer, (void*)(0xa0200000 + offset), bytes);
//	unlock_gdsys();
#else
	safe_memcpy(buffer, (void*)(0xa0200000 + offset), bytes);
#endif

	dcache_purge_range((uint32)buffer, bytes);
	return 0;
}

int flashrom_write(int offset, void * buffer, int bytes) {

	LOGFF("0x%08lx 0x%08lx %d\n", offset, buffer, bytes);

	(void)offset;
	(void)buffer;
	(void)bytes;

	return 0;
}

int flashrom_delete(int offset) {

	LOGFF("0x%08lx\n", offset);
	(void)offset;
	return 0;
}

/**
 * Sysinfo syscalls
 */
int sys_misc_init(void) {
	
	LOGFF(NULL);
	
	safe_memcpy((uint8 *)0x8c000068, (uint8 *)0xa021a056, 8);
	safe_memcpy((uint8 *)0x8c000070, (uint8 *)0xa021a000, 5);
	dcache_purge_range(0x8c000060, 32);
	return 0;
}

int sys_unknown(void) {
	LOGFF(NULL);
	return 0;
}

int sys_icon(int icon, uint8 *dest) {
	
	LOGFF("%d 0x%08lx\n", icon, dest);
	safe_memcpy(dest, (uint8 *)(0xa021a480 + (icon * 704)), 704);
	return 704;
}

uint8 *sys_id(void) {
	LOGFF(NULL);
	return (uint8 *)0x8c000068;
}

void enable_syscalls(int all) {
	
	gdc_syscall_save();
	gdc_syscall_disable();
	gdc_syscall_enable();
	
//	menu_syscall_save();
//	menu_syscall_disable();
//	menu_syscall_enable();
	
	if(all) {
		bfont_syscall_save();
		bfont_syscall_disable();
		bfont_syscall_enable();
		
		flash_syscall_save();
		flash_syscall_disable();
		flash_syscall_enable();
		
		sys_syscall_save();
		sys_syscall_disable();
		sys_syscall_enable();
		
		menu_syscall_save();
		menu_syscall_disable();
		menu_syscall_enable();
		
	} else if(IsoInfo->boot_mode != BOOT_MODE_DIRECT) {
		menu_syscall_save();
		menu_syscall_disable();
		menu_syscall_enable();
	}
}

void disable_syscalls(int all) {
	
	gdc_syscall_disable();
	
	if(all) {
		bfont_syscall_disable();
		flash_syscall_disable();
		sys_syscall_disable();
		menu_syscall_disable();
	} else if(IsoInfo->boot_mode != BOOT_MODE_DIRECT) {
		menu_syscall_disable();
	}
}

/* Patch the GDC driver in the BIOS syscalls */
void gdc_syscall_patch(void) {

	size_t size = bios_patch_end - bios_patch_base;
	
	if((uint32)IsoInfo > (0x8c0010f0 + size)) {

		bios_patch_handler = gdc_redir;
		
		memcpy((uint32 *) 0x8c001000, bios_patch_base, size);
		memcpy((uint32 *) 0x8c0010f0, bios_patch_base, size);
		
		size += 0xf0;
		icache_flush_range(0x8c001000, size);
		dcache_flush_range(0x8c001000, size);
		
	} else {
		patch_memory(0x8c0010f0, (uint32)gdc_redir);
	}
}
