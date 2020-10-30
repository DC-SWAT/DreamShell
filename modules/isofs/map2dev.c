/**
 * Copyright (c) 2015-2020 by SWAT <swat@211.ru> www.dc-swat.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 */

#include <ds.h>
#include <isofs/isofs.h>
#include <isofs/ciso.h>
#include <isofs/cdi.h>
#include <isofs/gdi.h>

//#define DEBUG 1
#define SECBUF_COUNT 32


int fs_iso_map2dev(const char *filename, kos_blockdev_t *dev) {
	
	file_t fd, fdi;
	uint8 *data;
	uint32 image_type = 0;
	uint32 image_hdr = 0;
	uint32 total_length = 0;
	uint32 lba = 150;
	uint32 sector = lba;
	const char mp[] = "/ifsm2d";
	
	if(dev->init(dev) < 0) {
		ds_printf("DS_ERROR: Can't initialize device\n");
		return -1;
	}
	
	if(fs_iso_mount(mp, filename) < 0) {
		return -1;
	}
	
	fd = fs_iso_first_file(mp);
	
	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open first file\n");
		fs_iso_unmount(mp);
		return -1;
	}
	
	fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_TYPE, &image_type);
	fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_HEADER_PTR, &image_hdr);
	fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_FD, &fdi);
	
	fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_LBA, &lba);
	fs_ioctl(fd, ISOFS_IOCTL_GET_TRACK_SECTOR_COUNT, &total_length);
	
	fs_close(fd);
	total_length += lba;
	
	data = memalign(32, SECBUF_COUNT * 2048);
	
	if(data == NULL) {
		ds_printf("DS_ERROR: No free memory\n");
		fs_iso_unmount(mp);
		return -1;
	}
	
	memset_sh4(data, 0, SECBUF_COUNT * 2048);

	for(sector = lba; sector < total_length; sector += SECBUF_COUNT) {
		
#ifdef DEBUG
		dbglog(DBG_DEBUG, "%s: read from %ld\n", __func__, sector);
#endif
		
		switch(image_type) {
			case ISOFS_IMAGE_TYPE_CSO:
			case ISOFS_IMAGE_TYPE_ZSO:
				ciso_read_sectors((CISO_header_t*)image_hdr, fdi, data, sector, SECBUF_COUNT);
				break;
			case ISOFS_IMAGE_TYPE_CDI:
				cdi_read_sectors((CDI_header_t*)image_hdr, fdi, data, sector, SECBUF_COUNT);
				break;
			case ISOFS_IMAGE_TYPE_GDI:
				gdi_read_sectors((GDI_header_t*)image_hdr, data, sector, SECBUF_COUNT);
				break;
			case ISOFS_IMAGE_TYPE_ISO:
			default:
				fs_seek(fdi, ((sector - lba) * 2048), SEEK_SET);
				fs_read(fdi, data, SECBUF_COUNT * 2048);
				break;
		}
		
#ifdef DEBUG
		dbglog(DBG_DEBUG, "%s: write to %ld\n", __func__, (sector - lba) * 4);
#endif
		
		if(dev->write_blocks(dev, (sector - lba) * 4, SECBUF_COUNT * 4, data) < 0) {
			ds_printf("DS_ERROR: Can't write to device\n");
			break;
		}
	}
	
	dev->flush(dev);
	fs_iso_unmount(mp);
	return 0;
}

