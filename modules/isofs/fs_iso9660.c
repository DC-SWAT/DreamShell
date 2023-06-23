/* DreamShell ##version##

   fs_iso9660.c - Virtual ISO9660 filesystem
   Copyright (C)2000,2001,2003 Dan Potter
   Copyright (C)2001 Andrew Kieschnick
   Copyright (C)2002 Bero
   Copyright (C)2011-2023 SWAT
*/

#include <ds.h>
#include <isofs/isofs.h>
#include <isofs/ciso.h>
#include <isofs/cdi.h>
#include <isofs/gdi.h>
#include "internal.h"

/* List of cache blocks (ordered least recently used to most recently) */
#define NUM_CACHE_BLOCKS 4

#ifndef MAX_ISO_FILES
# define MAX_ISO_FILES 16
#endif

/* Holds the data for one cache block, and a pointer to the next one.
   As sectors are read from the disc, they are added to the front of
   this cache. As the cache fills up, sectors are removed from the end
   of it. */
typedef struct {
	int32	sector;			/* CD sector */
	uint8	data[2048];		/* Sector data */
} cache_block_t;


/* ISO Directory entry */
typedef struct {
	uint8	length;			/* 711 */
	uint8	ext_attr_length;	/* 711 */
	uint8	extent[8];		/* 733 */
	uint8	size[8];		/* 733 */
	uint8	date[7];		/* 7x711 */
	uint8	flags;
	uint8	file_unit_size;		/* 711 */
	uint8	interleave;		/* 711 */
	uint8	vol_sequence[4];	/* 723 */
	uint8	name_len;		/* 711 */
	char	name[1];
} virt_iso_dirent_t;


typedef struct isofs {
        
    SLIST_ENTRY(isofs) list;
    vfs_handler_t *vfs;
    const char *fn;
    file_t fd;

	CDROM_TOC toc;
	uint32 session_base;
	uint32 root_extent;							/* Root directory extent */
	uint32 root_size;								/* Root directory size in bytes */
	virt_iso_dirent_t root_dirent;				/* Root dirent */
	int joliet;

	cache_block_t *icache[NUM_CACHE_BLOCKS];	/* inode cache */
	cache_block_t *dcache[NUM_CACHE_BLOCKS];	/* data cache */
	mutex_t cache_mutex;							/* Cache modification mutex */

	uint32 type;
	CISO_header_t *ciso;
	CDI_header_t  *cdi;
	GDI_header_t  *gdi;

} isofs_t;


/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
	uint32		first_extent;	/* First sector */
	int			dir;		/* >0 if a directory */
	uint32		ptr;		/* Current read position in bytes */
	uint32		size;		/* Length of file in bytes */
	dirent_t	dirent;		/* A static dirent to pass back to clients */
	int			broken;		/* >0 if the CD has been swapped out since open */
	isofs_t    *ifs;
} fh[MAX_ISO_FILES];


typedef SLIST_HEAD(isofs_list, isofs) isofs_list_t;
static isofs_list_t virt_iso_list;

/* Mutex for file handles */
static mutex_t fh_mutex = MUTEX_INITIALIZER;
static int inited = 0;

static int virt_iso_init_percd(isofs_t *ifs);
static int virt_iso_reset(isofs_t *ifs);
static void virt_iso_break_all(isofs_t *ifs);


/********************************************************************************/
/* Low-level Joliet utils */

/* Joliet UCS is big endian */
static void utf2ucs(uint8 * ucs, const uint8 * utf) {
	int c;
	
	do {
		c = *utf++;
		if (c <= 0x7f) {
		} else if (c < 0xc0) {
			c = (c & 0x1f) << 6;
			c |= (*utf++) & 0x3f;
		} else {
			c = (c & 0x0f) << 12;
			c |= ((*utf++) & 0x3f) << 6;
			c |= (*utf++) & 0x3f;
		}
		*ucs++ = c >> 8;
		*ucs++ = c & 0xff;
	} while (c);
}

static void ucs2utfn(uint8 * utf, const uint8 * ucs, size_t len) {
	int c;
	
	len = len / 2;
	while (len) {
		len--;
		c = (*ucs++) << 8;
		c |= *ucs++;
		if (c == ';') break;
		if (c <= 0x7f) {
			*utf++ = c;
		} else if (c <= 0x7ff) {
			*utf++ = 0xc0 | (c >> 6);
			*utf++ = 0x80 | (c & 0x3f);
		} else {
			*utf++ = 0xe0 | (c >> 12);
			*utf++ = 0x80 | ((c >> 6) & 0x3f);
			*utf++ = 0x80 | (c & 0x3f);
		}
	}
	*utf = 0;
}

static int ucscompare(const uint8 * isofn, const uint8 * normalfn, int isosize) {
	int i, c0, c1 = 0;

	/* Compare ISO name */
	for (i=0; i<isosize; i+=2) {
		c0 = ((int)isofn[i] << 8) | ((int)isofn[i+1]);
		c1 = ((int)normalfn[i] << 8) | ((int)normalfn[i+1]);

		if (c0 == ';') break;

		/* Otherwise, compare the chars normally */
		if (tolower(c0) != tolower(c1))
			return -1;
	}

	c1 = ((int)normalfn[i] << 8) | (normalfn[i+1]);

	/* Catch ISO name shorter than normal name */
	if (c1 != '/' && c1 != '\0')
		return -1;
	else
		return 0;
}

static int isjoliet(char * p) {
	if (p[0] == '%' && p[1] == '/') {
		switch (p[2]) {
		case '@': return 1;
		case 'C': return 2;
		case 'E': return 3;
		}
	}
	return 0;
}


/********************************************************************************/
/* Low-level ISO utils */

/* Util function to reverse the byte order of a uint32 */
/* static uint32 ntohl_32(const void *data) {
	const uint8 *d = (const uint8*)data;
	return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
} */

/* This seems kinda silly, but it's important since it allows us
   to do unaligned accesses on a buffer */
static uint32 htohl_32(const void *data) {
	const uint8 *d = (const uint8*)data;
	return (d[0] << 0) | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

/* Read red-book section 7.1.1 number (8 bit) */
/* static uint8 virt_iso_711(const uint8 *from) { return (*from & 0xff); } */

/* Read red-book section 7.3.3 number (32 bit LE / 32 bit BE) */
static uint32 virt_iso_733(const uint8 *from) { return htohl_32(from); }

/* Read sectors data */
static int read_data(uint32 sector, uint32 count, void *data, isofs_t *ifs) {
	switch(ifs->type) {
		case ISOFS_IMAGE_TYPE_CSO:
		case ISOFS_IMAGE_TYPE_ZSO:
			return ciso_read_sectors(ifs->ciso, ifs->fd, data, (sector + 150) - ifs->session_base, count);
		case ISOFS_IMAGE_TYPE_CDI:
			return cdi_read_sectors(ifs->cdi, ifs->fd, data, sector, count);
		case ISOFS_IMAGE_TYPE_GDI:
			return gdi_read_sectors(ifs->gdi, data, sector, count);
		case ISOFS_IMAGE_TYPE_ISO:
		default:
			fs_seek(ifs->fd, (((sector + 150) - ifs->session_base) * 2048), SEEK_SET);
			return read_sectors_data(ifs->fd, count, 2048, data);
	}
}


#define ROOT_DIRECTORY_HORIZON 64

// TODO: CSO/ZSO
static int isofile_read(file_t fd, off_t offset, size_t size, uint8 *buff) {
	fs_seek(fd, offset, SEEK_SET);
	return fs_read(fd, buff, size);
}

static int isofile_find_lba(isofs_t *ifs) {
	uint32 sec;
	uint8 buf1[0x22], buf2[0x22];

	for (sec = 16; sec < ROOT_DIRECTORY_HORIZON; sec++) {

		if (isofile_read(ifs->fd, sec << 11, 6, buf1) < 0)
			return 150;
		if (!memcmp(buf1, "\001CD001", 6))
			break;
		else if(!memcmp(buf1, "\377CD001", 6))
			return 150;
	}
	
	if (sec >= ROOT_DIRECTORY_HORIZON)
		return 150;

#ifdef DEBUG
	ds_printf("DS_ISOFS: PVD is at %d\n", sec);
#endif

	if (isofile_read(ifs->fd, (sec << 11) + 0x9c, 0x22, buf1) < 0)
		return -1;

	while (++sec < ROOT_DIRECTORY_HORIZON) {
		if (!isofile_read(ifs->fd, sec << 11, 0x22, buf2))
			return 150;
		if (!memcmp(buf1, buf2, 0x12) && !memcmp(buf1 + 0x19, buf2 + 0x19, 0x9))
			break;
	}
	
	if (sec >= ROOT_DIRECTORY_HORIZON)
		return 150;

#ifdef DEBUG
	ds_printf("DS_ISOFS: Root directory is at %d\n", sec);
#endif

	sec = ((((((buf1[5]<<8)|buf1[4])<<8)|buf1[3])<<8)|buf1[2])+150-sec;
	
#ifdef DEBUG
	ds_printf("DS_ISOFS: Session offset is %d\n", sec);
#endif

//	ifs->session_base = sec;
	return sec;
}



/********************************************************************************/
/* Low-level block cacheing routines. This implements a simple queue-based 
   LRU/MRU cacheing system. Whenever a block is requested, it will be placed
   on the MRU end of the queue. As more blocks are loaded than can fit in
   the cache, blocks are deleted from the LRU end. */

/* Graduate a block from its current position to the MRU end of the cache */
static void bgrad_cache(cache_block_t **cache, int block) {
	int		i;
	cache_block_t	*tmp;
	
	/* Don't try it with the end block */
	if (block < 0 || block >= (NUM_CACHE_BLOCKS-1)) return;
	
	/* Make a copy and scoot everything down */
	tmp = cache[block];
	for (i=block; i<(NUM_CACHE_BLOCKS - 1); i++)
		cache[i] = cache[i+1];
	cache[NUM_CACHE_BLOCKS-1] = tmp;
}


static int bread_cache(cache_block_t **cache, uint32 sector, isofs_t *ifs) {
	int i, j, rv;
	
#ifdef DEBUG
	ds_printf("%s: %d\n", __func__, sector);
#endif

	rv = -1;	
	mutex_lock(&ifs->cache_mutex);

	/* Look for a pre-existing cache block */
	for (i=NUM_CACHE_BLOCKS-1; i>=0; i--) {
		if (cache[i]->sector == sector) {
			bgrad_cache(cache, i);
			rv = NUM_CACHE_BLOCKS - 1;
			goto bread_exit;
		}
	}
	
	/* If not, look for an open cache slot; if we find one, use it */
	for (i=0; i<NUM_CACHE_BLOCKS; i++) {
		if (cache[i]->sector == -1) break;
	}
	
	/* If we didn't find one, kick an LRU block out of cache */
	if (i >= NUM_CACHE_BLOCKS) { i = 0; }

	j = read_data(sector, 1, cache[i]->data, ifs);
	
	if (j < 0) {
#ifdef DEBUG
		ds_printf("DS_ERROR: Can't read sector %d\n", sector);
#endif
		goto bread_exit;
	}
	
	cache[i]->sector = sector;
	
	/* Move it to the most-recently-used position */
	bgrad_cache(cache, i);
	rv = NUM_CACHE_BLOCKS - 1;

	/* Return the new cache block index */
bread_exit:
	mutex_unlock(&ifs->cache_mutex);
	return rv;
}

/* read data block */
static int bdread(uint32 sector, isofs_t *ifs) {
	return bread_cache(ifs->dcache, sector, ifs);
}

/* read inode block */
static int biread(uint32 sector, isofs_t *ifs) {
	return bread_cache(ifs->icache, sector, ifs);
}

/* Clear both caches */
static void bclear(isofs_t *ifs) {

	int i;
	mutex_lock(&ifs->cache_mutex);

	for (i = 0; i < NUM_CACHE_BLOCKS; i++) {
		ifs->icache[i]->sector = -1;
		ifs->dcache[i]->sector = -1;
	}

	mutex_unlock(&ifs->cache_mutex);
}

/********************************************************************************/
/* Higher-level ISO9660 primitives */

//#define IPBIN_TOC_OFFSET 260

static int get_lba_from_mki(isofs_t *ifs) {
	
	ds_printf("DS_ISOFS: Searching MKI string...\n");
	int lba = -1;
	int s = 17;
	uint8 mki[2048];
	char mkisofs[256];

	while(s < 25) {

		if(ifs->ciso != NULL) {
			ciso_read_sectors(ifs->ciso, ifs->fd, mki, s, 1);
//		} else if(ifs->cdi != NULL) {
//			cdi_read_sectors(ifs->cdi, ifs->fd, mki, s, 1);
//		} else if(ifs->gdi != NULL) {
//			gdi_read_sectors(ifs->gdi, mki, s, 1);
		} else {
			fs_seek(ifs->fd, s << 1, SEEK_SET);
			fs_read(ifs->fd, mki, sizeof(mki));
		}
		
		/*
			MKI Mon Aug 20 14:10:58 2001
			mkisofs 1.13a03 -l -C 0,11700 -V BASS2_ECH -o tmp.iso bass
			
			MKI Wed Mar  2 22:34:30 2011
			mkisofs 1.15a12 -V DC_GAME -G sys/katana/IP.BIN -joliet -rock -l -o image.iso ./cd.
			
			MKI Tue Nov 20 06:14:40 2007
			mkisofs 2.01-bootcd.ru -C 0,11702 -V D2_A -sort Temp_SORT.TXT -l -o DATA.ISO FILE
		 */
		
		if(mki[0] == 0x4D && mki[1] == 0x4B && mki[2] == 0x49 && mki[3] == 0x20) {
			ds_printf("DS_ISOFS: %s\n", mki);
			if(sscanf((char*)mki+29, "mkisofs %[^,],%d", mkisofs, &lba) > 1) {
				ds_printf("DS_ISOFS: Detected LBA: %d + 150\n", lba);
				lba += 150;
				break;
			} else {
				lba = 0;
				ds_printf("DS_ISOFS: Can't find LBA from MKI.\n");
				break;
			}
		}
		s++;
	}
	return lba;
}

/*
static int get_toc_and_lba(isofs_t *ifs) {

	uint8 ipbin[2048];
	int i, lba = 150, mk = 0;
	
	if(ifs->ciso != NULL) {
		ciso_read_sectors(ifs->ciso, ifs->fd, ipbin, 0, 1);
	} else if(ifs->cdi != NULL) {
        
        cdi_get_toc(ifs->cdi, &ifs->toc);
        return cdrom_locate_data_track(&ifs->toc);
		
	} else if(ifs->gdi != NULL) {
        
        gdi_get_toc(ifs->gdi, &ifs->toc);
        return 45150;
        
	} else {
	
		fs_seek(ifs->fd, 0, SEEK_SET);

		if(!fs_read(ifs->fd, ipbin, 2048)) {
			ds_printf("DS_ERROR: Can't read IP.BIN from ISO\n");
			return lba;
		}
	}


	if(ipbin[0x0] == 'S' && ipbin[0x1] == 'E' && ipbin[(IPBIN_TOC_OFFSET)-1] == 0x31) {

		memcpy_sh4(&ifs->toc, ipbin + IPBIN_TOC_OFFSET, sizeof(CDROM_TOC));
		//lba = cdrom_locate_data_track(&toc) + 150;

		for (i = 0; i < 99; i++) {
			if(TOC_CTRL(ifs->toc.entry[i]) == 4) {
				
				lba = TOC_LBA(ifs->toc.entry[i]);
				
				if(lba >= 45150) {
					
					mk = get_lba_from_mki(ifs);
					
					if(mk > 150) {
						return mk;
					} else if(mk < 0) {
						return lba;
					} else {
						return isofile_find_lba(ifs);
					}
				}
				
				break;
			}
		}
		
	} else {
		mk = get_lba_from_mki(ifs);
		
		if(mk > 0) {
			return mk;
		} else {
			return 150;
		}
	}

	return lba;
}
*/

static int get_toc_and_lba(isofs_t *ifs) {

	int lba = 150;

	switch(ifs->type) {
		case ISOFS_IMAGE_TYPE_CDI:

			cdi_get_toc(ifs->cdi, &ifs->toc);
			return cdrom_locate_data_track(&ifs->toc);

		case ISOFS_IMAGE_TYPE_GDI:

			gdi_get_toc(ifs->gdi, &ifs->toc);
			return 45150;

		case ISOFS_IMAGE_TYPE_CSO:
		case ISOFS_IMAGE_TYPE_ZSO:

			lba = get_lba_from_mki(ifs);
			
			if(lba <= 0) {
				lba = 150; // TODO: isofile_find_lba(ifs);
			}
			
			spoof_multi_toc_cso(&ifs->toc, ifs->ciso, lba);
			break;

		case ISOFS_IMAGE_TYPE_ISO:
		default:

			lba = get_lba_from_mki(ifs);
			
			if(lba <= 0) {
				lba = isofile_find_lba(ifs);
			}
			
			if(lba >= 45150) {
				spoof_multi_toc_3track_gd(&ifs->toc);
			} else {
				spoof_multi_toc_iso(&ifs->toc, ifs->fd, lba);
			}

			break;
	}

	return lba;
}


/* Per-disc initialization; this is done every time it's discovered that
   a new CD has been inserted. */
static int virt_iso_init_percd(isofs_t *ifs) {
	int	i, blk;
	
	/* Start off with no cached blocks and no open files*/
	// virt_iso_reset(ifs);
	
	ifs->session_base = get_toc_and_lba(ifs);
	ds_printf("DS_ISOFS: Session base: %d\n", ifs->session_base);
	
	//dbglog(DBG_DEBUG, "virt_iso_init_percd: session base = %d\n", ifs->session_base);
	
	/* Check for joliet extensions */
	ifs->joliet = 0;
	for (i=1; i<=3; i++) {
		blk = biread(ifs->session_base + i + 16 - 150, ifs);
		if (blk < 0) return blk;
		if (memcmp((char *)ifs->icache[blk]->data, "\02CD001", 6) == 0) {
			ifs->joliet = isjoliet((char *)ifs->icache[blk]->data+88);
//			ds_printf("DS_ISOFS: joliet level %d extensions detected\n", ifs->joliet);
			if (ifs->joliet) break;
		}
	}

	/* If that failed, go after standard/RockRidge ISO */
	if (!ifs->joliet) {
		/* Grab and check the volume descriptor */	
		blk = biread(ifs->session_base + 16 - 150, ifs);
		if (blk < 0) return i;
		if (memcmp((char*)ifs->icache[blk]->data, "\01CD001", 6)) {
			ds_printf("DS_ERROR: CD/GD image is not ISO9660\n");
			return -1;
		}
	}

	/* Locate the root directory */
	memcpy_sh4(&ifs->root_dirent, ifs->icache[blk]->data+156, sizeof(virt_iso_dirent_t));
	ifs->root_extent = virt_iso_733(ifs->root_dirent.extent);
	ifs->root_size = virt_iso_733(ifs->root_dirent.size);
	
	return 0;
}

/* Compare an ISO9660 filename against a normal filename. This takes into
   account the version code on the end and is not case sensitive. Also
   takes into account the trailing period that some CD burning software
   adds. */
static int fncompare(const char *isofn, int isosize, const char *normalfn) {
	int i;

	/* Compare ISO name */
	for (i=0; i<isosize; i++) {
		/* Weed out version codes */
		if (isofn[i] == ';') break;

		/* Deal with crap '.' at end of filenames */
		if (isofn[i] == '.' &&
				(i == (isosize-1) || isofn[i+1] == ';'))
			break;

		/* Otherwise, compare the chars normally */
		if (tolower((int)isofn[i]) != tolower((int)normalfn[i]))
			return -1;
	}

	/* Catch ISO name shorter than normal name */
	if (normalfn[i] != '/' && normalfn[i] != '\0')
		return -1;
	else
		return 0;
}

/* Locate an ISO9660 object in the given directory; this can be a directory or
   a file, it works fine for either one. Pass in:
   
   fn:		object filename (relative to the passed directory)
   dir:		0 if looking for a file, 1 if looking for a dir
   dir_extent:	directory extent to start with
   dir_size:	directory size (in bytes)
   
   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static virt_iso_dirent_t *find_object(const char *fn, int dir,
		uint32 dir_extent, uint32 dir_size, isofs_t *ifs) {
	int	i, c;
	virt_iso_dirent_t *de;

	/* RockRidge */
	int		len;
	uint8	*pnt;
	char	rrname[NAME_MAX];
	int		rrnamelen;
	int		size_left;

	/* We need this to be signed for our while loop to end properly */
	size_left = (int)dir_size;

	/* Joliet */
	uint8		* ucsname = (uint8 *)rrname;

	/* If this is a Joliet CD, then UCSify the name */
	if (ifs->joliet)
		utf2ucs(ucsname, (uint8 *)fn);
	
	while (size_left > 0) {
		c = biread(dir_extent, ifs);
		if (c < 0) return NULL;
		
		for (i=0; i<2048 && i<size_left; ) {
			/* Locate the current dirent */
			de = (virt_iso_dirent_t *)(ifs->icache[c]->data + i);
			if (!de->length) break;

			/* Try the Joliet filename if the CD is a Joliet disc */
			if (ifs->joliet) {
				if (!ucscompare((uint8 *)de->name, ucsname, de->name_len)) {
					if (!((dir << 1) ^ de->flags))
						return de;
				}
			} else {
				/* Assume no Rock Ridge name */
				rrnamelen = 0;
		
				/* Check for Rock Ridge NM extension */
				len = de->length - sizeof(virt_iso_dirent_t)
					+ sizeof(de->name) - de->name_len;
				pnt = (uint8*)de + sizeof(virt_iso_dirent_t)
					- sizeof(de->name) + de->name_len;
				if ((de->name_len & 1) == 0) {
					pnt++; len--;
				}
				while ((len >= 4) && ((pnt[3] == 1) || (pnt[3] == 2))) {
					if (strncmp((char *)pnt, "NM", 2) == 0) {
						rrnamelen = pnt[2] - 5;
						strncpy(rrname, (char *)(pnt+5), rrnamelen);
						rrname[rrnamelen] = 0;
					}
					len -= pnt[2];
					pnt += pnt[2];
				}
			
				/* Check the filename against the requested one */
				if (rrnamelen > 0) {
					char *p = strchr(fn, '/');
					int fnlen;
				
					if (p)
						fnlen = p - fn;
					else
						fnlen = strlen(fn);
					if (!strncasecmp(rrname, fn, fnlen) && ! *(rrname + fnlen)) {
						if (!((dir << 1) ^ de->flags))
							return de;
					}
				} else {
					if (!fncompare(de->name, de->name_len, fn)) {
						if (!((dir << 1) ^ de->flags))
							return de;
					}
				}
			}
			
			i += de->length;
		}
		
		dir_extent++;
		size_left -= 2048;
	}
	
	return NULL;
}

/* Locate an ISO9660 object anywhere on the disc, starting at the root,
   and expecting a fully qualified path name. This is analogous to find_object
   but it searches with the path in mind.

   fn:		object filename (relative to the passed directory)
   dir:		0 if looking for a file, 1 if looking for a dir
   dir_extent:	directory extent to start with
   dir_size:	directory size (in bytes)
   
   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static virt_iso_dirent_t *find_object_path(const char *fn, int dir, virt_iso_dirent_t *start, isofs_t *ifs) {
	char		*cur;

	/* If the object is in a sub-tree, traverse the trees looking
	   for the right directory */
	while ((cur = strchr(fn, '/'))) {
		if (cur != fn) {
			/* Note: trailing path parts don't matter since find_object
			   only compares based on the FN length on the disc. */
			start = find_object(fn, 1, virt_iso_733(start->extent), virt_iso_733(start->size), ifs);
			if (start == NULL) return NULL;
		}
		fn = cur + 1;
	}

	/* Locate the file in the resulting directory */
	if (*fn) {
		start = find_object(fn, dir, virt_iso_733(start->extent), virt_iso_733(start->size), ifs);
		return start;
	}
	else {
		if (!dir)
			return NULL;
		else
			return start;
	}
}

/********************************************************************************/
/* File primitives */

/* Break all of our open file descriptor. This is necessary when the disc
   is changed so that we don't accidentally try to keep on doing stuff
   with the old info. As files are closed and re-opened, the broken flag
   will be cleared. */
static void virt_iso_break_all(isofs_t *ifs) {
	int i;

	mutex_lock(&fh_mutex);

	for (i = 0; i < MAX_ISO_FILES; i++) {
		if(ifs == fh[i].ifs) {
			fh[i].broken = 1;
		}
	}

	mutex_unlock(&fh_mutex);
}

/* Open a file or directory */
static void * virt_iso_open(vfs_handler_t * vfs, const char *fn, int mode) {
	file_t		fd;
	virt_iso_dirent_t	*de;
	isofs_t *ifs = (isofs_t *)vfs->privdata;

	/* Make sure they don't want to open things as writeable */
	if ((mode & O_MODE_MASK) != O_RDONLY)
		return 0;


	/* Find the file we want */
	de = find_object_path(fn, (mode & O_DIR)?1:0, &ifs->root_dirent, ifs);
	if (!de) return 0;
	
	/* Find a free file handle */
	mutex_lock(&fh_mutex);
	for (fd=0; fd<MAX_ISO_FILES; fd++)
		if (fh[fd].first_extent == 0) {
			fh[fd].first_extent = -1;
			break;
		}
	mutex_unlock(&fh_mutex);
	if (fd >= MAX_ISO_FILES)
		return 0;

	/* Fill in the file handle and return the fd */
	fh[fd].first_extent = virt_iso_733(de->extent);
	fh[fd].dir = (mode & O_DIR)?1:0;
	fh[fd].ptr = 0;
	fh[fd].size = virt_iso_733(de->size);
	fh[fd].broken = 0;
	fh[fd].ifs = ifs;

	return (void *)fd;
}

/* Close a file or directory */
static int virt_iso_close(void * h) {
	file_t fd = (file_t)h;

	/* Check that the fd is valid */
	if (fd < MAX_ISO_FILES) {
		/* No need to lock the mutex: this is an atomic op */
		fh[fd].first_extent = 0;
	}
	
	return 0;
}

/* Read from a file */
static ssize_t virt_iso_read(void * h, void *buf, size_t bytes) {
	int rv, toread, thissect, c;
	uint8 * outbuf;
	file_t fd = (file_t)h;

	/* Check that the fd is valid */
	if (fd >= MAX_ISO_FILES || fh[fd].first_extent == 0 || fh[fd].broken)
		return -1;

	rv = 0;
	outbuf = (uint8 *)buf;

	/* Read zero or more sectors into the buffer from the current pos */
	while (bytes > 0) {
		/* Figure out how much we still need to read */
		toread = (bytes > (fh[fd].size - fh[fd].ptr)) ?
			fh[fd].size - fh[fd].ptr : bytes;
		if (toread == 0) break;

		/* How much more can we read in the current sector? */
		thissect = 2048 - (fh[fd].ptr % 2048);

		/* If we're on a sector boundary and we have more than one
		   full sector to read, then short-circuit the cache here
		   and use the multi-sector reads from the image */
		if (thissect == 2048 && toread >= 2048) {
			/* Round it off to an even sector count */
			thissect = toread / 2048;
			toread = thissect * 2048;

#ifdef DEBUG
			ds_printf("%s: Short-circuit read for %d sectors\n", __func__, thissect);
#endif
			/* Do the read */
			c = read_data(fh[fd].first_extent + fh[fd].ptr/2048, thissect, outbuf, fh[fd].ifs);
			if (c < 0) {
				return -1;
			}
			
		} else { 
			
			toread = (toread > thissect) ? thissect : toread;
//			uint8 tmp[2048];
		
			/* Do the read */
			c = bdread(fh[fd].first_extent + fh[fd].ptr/2048, fh[fd].ifs);
//			c = read_data(fh[fd].first_extent + fh[fd].ptr/2048, 1, tmp, fh[fd].ifs) < 0)
			if (c < 0) {
				return -1;
			}
			memcpy_sh4(outbuf, fh[fd].ifs->dcache[c]->data + (fh[fd].ptr % 2048), toread);
//			memcpy_sh4(outbuf, tmp + (fh[fd].ptr % 2048), toread);
		}
		
		/* Adjust pointers */
		outbuf += toread;
		fh[fd].ptr += toread;
		bytes -= toread;
		rv += toread;
	}
	
	return rv;
}

/* Seek elsewhere in a file */
static off_t virt_iso_seek(void * h, off_t offset, int whence) {
	file_t fd = (file_t)h;

	/* Check that the fd is valid */
	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0 || fh[fd].broken)
		return -1;

	/* Update current position according to arguments */
	switch (whence) {
		case SEEK_SET:
			fh[fd].ptr = offset;
			break;
		case SEEK_CUR:
			fh[fd].ptr += offset;
			break;
		case SEEK_END:
			fh[fd].ptr = fh[fd].size + offset;
			break;
		default:
			return -1;
	}
	
	/* Check bounds */
	if (fh[fd].ptr < 0) fh[fd].ptr = 0;
	if (fh[fd].ptr > fh[fd].size) fh[fd].ptr = fh[fd].size;
	
	return fh[fd].ptr;
}

/* Tell where in the file we are */
static off_t virt_iso_tell(void * h) {
	file_t fd = (file_t)h;

	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0 || fh[fd].broken)
		return -1;

	return fh[fd].ptr;
}

/* Tell how big the file is */
static size_t virt_iso_total(void * h) {
	file_t fd = (file_t)h;

	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0 || fh[fd].broken)
		return -1;

	return fh[fd].size;
}

/* Helper function for readdir: post-processes an ISO filename to make
   it a bit prettier. */
static void fn_postprocess(char *fnin) {
	char	* fn = fnin;

	while (*fn && *fn != ';') {
		*fn = tolower((int)*fn);
		fn++;
	}
	*fn = 0;

	/* Strip trailing dots */
	if (fn > fnin && fn[-1] == '.') {
		fn[-1] = 0;
	}
}

/* Read a directory entry */
static dirent_t *virt_iso_readdir(void * h) {
	int		c;
	virt_iso_dirent_t	*de;

	/* RockRidge */
	int		len;
	uint8		*pnt;

	file_t fd = (file_t)h;

	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0 || !fh[fd].dir || fh[fd].broken)
		return NULL;

	/* Scan forwards until we find the next valid entry, an
	   end-of-entry mark, or run out of dir size. */
	c = -1; de = NULL;
	while(fh[fd].ptr < fh[fd].size) {
		/* Get the current dirent block */
		c = biread(fh[fd].first_extent + fh[fd].ptr/2048, fh[fd].ifs);
		if (c < 0) return NULL;
	
		de = (virt_iso_dirent_t *)(fh[fd].ifs->icache[c]->data + (fh[fd].ptr%2048));
		if (de->length) break;

		/* Skip to the next sector */
		fh[fd].ptr += 2048 - (fh[fd].ptr%2048);
	}
	if (fh[fd].ptr >= fh[fd].size) return NULL;
	
	/* If we're at the first, skip the two blank entries */
	if (!de->name[0] && de->name_len == 1) {
		fh[fd].ptr += de->length;
		de = (virt_iso_dirent_t *)(fh[fd].ifs->icache[c]->data + (fh[fd].ptr%2048));
		fh[fd].ptr += de->length;
		de = (virt_iso_dirent_t *)(fh[fd].ifs->icache[c]->data + (fh[fd].ptr%2048));
		if (!de->length) return NULL;
	}

	if (fh[fd].ifs->joliet) {
		ucs2utfn((uint8 *)fh[fd].dirent.name, (uint8 *)de->name, de->name_len);
	} else {
		/* Fill out the VFS dirent */
		strncpy(fh[fd].dirent.name, de->name, de->name_len);
		fh[fd].dirent.name[de->name_len] = 0;
		fn_postprocess(fh[fd].dirent.name);

		/* Check for Rock Ridge NM extension */
		len = de->length - sizeof(virt_iso_dirent_t) + sizeof(de->name) - de->name_len;
		pnt = (uint8*)de + sizeof(virt_iso_dirent_t) - sizeof(de->name) + de->name_len;
		if ((de->name_len & 1) == 0) {
			pnt++; len--;
		}
		while ((len >= 4) && ((pnt[3] == 1) || (pnt[3] == 2))) {
			if (strncmp((char *)pnt, "NM", 2) == 0) {
				strncpy(fh[fd].dirent.name, (char *)(pnt+5), pnt[2] - 5);
				fh[fd].dirent.name[pnt[2] - 5] = 0;
			}
			len -= pnt[2];
			pnt += pnt[2];
		}
	}

	if (de->flags & 2) {
		fh[fd].dirent.size = -1;
		fh[fd].dirent.attr = O_DIR;
	} else {
		fh[fd].dirent.size = virt_iso_733(de->size);
		fh[fd].dirent.attr = 0;
	}

	fh[fd].ptr += de->length;
	
	return &fh[fd].dirent;
}


static int virt_iso_rewinddir(void * h) {
	file_t fd = (file_t)h;

	if(fd >= MAX_ISO_FILES || fh[fd].first_extent == 0 || !fh[fd].dir ||
		fh[fd].broken) {
		errno = EBADF;
		return -1;
	}

	/* Rewind to the beginning of the directory. */
	fh[fd].ptr = 0;
	return 0;
}


static int virt_iso_reset(isofs_t *ifs) {
	virt_iso_break_all(ifs);
	bclear(ifs);
	return 0;
}


/** 
 * Needed for isoldr module 
 */
static int virt_iso_ioctl(void * hnd, int cmd, va_list ap) {
	
	file_t fd = (file_t)hnd;
	
#ifdef DEBUG
	ds_printf("%s: %d\n", __func__, cmd);
#endif
	
	void *data = va_arg(ap, void *);

	switch(cmd) {
		case ISOFS_IOCTL_RESET:
		
			virt_iso_reset(fh[fd].ifs);
			break;
			
		case ISOFS_IOCTL_GET_FD_LBA:
		
			memcpy_sh4(data, &fh[fd].first_extent, sizeof(uint32));
			break;
			
		case ISOFS_IOCTL_GET_DATA_TRACK_FILENAME:
		{
			if(fh[fd].ifs->gdi != NULL) {
				GDI_track_t *trk = gdi_get_track(fh[fd].ifs->gdi, fh[fd].ifs->session_base - 150);
				memcpy_sh4(data, trk->filename, sizeof(trk->filename));
			} else {
				return -1;
			}
			break;
		}
		case ISOFS_IOCTL_GET_DATA_TRACK_FILENAME2:
		{
			if(fh[fd].ifs->gdi != NULL) {
				
				GDI_track_t *trk = gdi_get_track(fh[fd].ifs->gdi, fh[fd].first_extent);
				char *p = strrchr(trk->filename, '/');
				char *fn = (char*)data;
				
				strncpy(fn, p + 1, 11);
				fn[11] = 0;
				
			} else {
				return -1;
			}
			break;
		}
		case ISOFS_IOCTL_GET_DATA_TRACK_LBA:
		
			memcpy_sh4(data, &fh[fd].ifs->session_base, sizeof(uint32));
			break;
			
		case ISOFS_IOCTL_GET_DATA_TRACK_LBA2:
		{
			if(fh[fd].ifs->gdi != NULL) {
				
				GDI_track_t *trk = gdi_get_track(fh[fd].ifs->gdi, fh[fd].first_extent);
				uint32 lba = trk->start_lba + 150;
				memcpy_sh4(data, &lba, sizeof(uint32));
				
			} else {
				return -1;
			}
			break;
		}
		case ISOFS_IOCTL_GET_DATA_TRACK_OFFSET:
		{
			memset_sh4(data, 0, sizeof(uint32));
			
			if(fh[fd].ifs->cdi != NULL) {
				
				uint16 ssz = 0;
				uint32 offset = cdi_get_offset(fh[fd].ifs->cdi, fh[fd].ifs->session_base - 150, &ssz);
				memcpy_sh4(data, &offset, sizeof(uint32));
			}
			
			break;
		}
		case ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE:
		{
			uint32 sec_size;
			
			if(fh[fd].ifs->cdi != NULL) {
				
				CDI_track_t *ctrk = cdi_get_track(fh[fd].ifs->cdi, fh[fd].ifs->session_base - 150);
				sec_size = cdi_track_sector_size(ctrk);
				
			} else if(fh[fd].ifs->gdi != NULL) {
				
				GDI_track_t *gtrk = gdi_get_track(fh[fd].ifs->gdi, fh[fd].ifs->session_base - 150);
				sec_size = gdi_track_sector_size(gtrk);
				
			} else {
				sec_size = 2048;
			}
			
			memcpy_sh4(data, &sec_size, sizeof(sec_size));
			break;
		}
		case ISOFS_IOCTL_GET_IMAGE_TYPE:
		{
			memcpy_sh4(data, &fh[fd].ifs->type, sizeof(uint32));
			break;
		}
		case ISOFS_IOCTL_GET_IMAGE_HEADER_PTR:
		{
			uint32 lnk = 0;
			
			if(fh[fd].ifs->cdi != NULL) {
				lnk = (uint32)fh[fd].ifs->cdi;
			} else if(fh[fd].ifs->gdi != NULL) {
				lnk = (uint32)fh[fd].ifs->gdi;
			} else if(fh[fd].ifs->ciso != NULL) {
				lnk = (uint32)fh[fd].ifs->ciso;
			} else {
				return -1;
			}
			
			memcpy_sh4(data, &lnk, sizeof(uint32));
			break;
		}
		case ISOFS_IOCTL_GET_IMAGE_FD:
		{
			memcpy_sh4(data, &fh[fd].ifs->fd, sizeof(uint32));
			break;
		}
		case ISOFS_IOCTL_GET_TOC_DATA:
		
			memcpy_sh4(data, &fh[fd].ifs->toc, sizeof(CDROM_TOC));
			break;
			
		case ISOFS_IOCTL_GET_BOOT_SECTOR_DATA:
		{
			if(read_data(fh[fd].ifs->session_base - 150, 1, data, fh[fd].ifs) < 0) {
				return -1;
			}
			break;
		}
		case ISOFS_IOCTL_GET_CDDA_OFFSET:
		{	
			if(fh[fd].ifs->cdi == NULL) {
				return -1;
			}
			
			uint32 *offset = (uint32 *)data;
			uint16 ssz = 0;
			int i;
			
			for(i = 2; i < 99; i++) {
				
				if(fh[fd].ifs->toc.entry[i] == (uint32)-1)
					break;
					
				if(TOC_CTRL(fh[fd].ifs->toc.entry[i]) == 0) {
					offset[i] = cdi_get_offset(fh[fd].ifs->cdi, TOC_LBA(fh[fd].ifs->toc.entry[i]), &ssz);
				}
			}
			
			break;
		}
		case ISOFS_IOCTL_GET_TRACK_SECTOR_COUNT:
		{
			uint32 val = *(uint32*)data;
			uint32 sec_size = 2048;
			
			if(!val) {
				val = fh[fd].ifs->session_base - 150;
			} else {
				val -= 150;
			}
			
			if(fh[fd].ifs->cdi != NULL) {
				
				CDI_track_t *ctrk = cdi_get_track(fh[fd].ifs->cdi, val);
				sec_size = cdi_track_sector_size(ctrk);
				
				val = ctrk->total_length / sec_size;
				
			} else if(fh[fd].ifs->gdi != NULL) {
				
				GDI_track_t *gtrk = gdi_get_track(fh[fd].ifs->gdi, val);
				sec_size = gdi_track_sector_size(gtrk);
				
				val = fs_total(fh[fd].ifs->gdi->track_fd) / sec_size;
			
			} else if(fh[fd].ifs->ciso != NULL) {
				
				val = fh[fd].ifs->ciso->total_bytes / fh[fd].ifs->ciso->block_size;
			
			} else {
				val = fs_total(fh[fd].ifs->fd) / sec_size;
			}
			
			memcpy_sh4(data, &val, sizeof(uint32));
			break;
		}
		default:
			return -1;
	}

	return 0;
}


static int virt_iso_fcntl(void *h, int cmd, va_list ap) {
	file_t fd = (file_t)h;
	int rv = -1;

	(void)ap;

	if(fd >= MAX_ISO_FILES || !fh[fd].first_extent || fh[fd].broken) {
		errno = EBADF;
		return -1;
	}

	switch(cmd) {
		case F_GETFL:
			rv = O_RDONLY;

			if(fh[fd].dir)
				rv |= O_DIR;

			break;

		case F_SETFL:
		case F_GETFD:
		case F_SETFD:
			rv = 0;
			break;

		default:
			errno = EINVAL;
	}

	return rv;
}


static int virt_iso_fstat(void *h, struct stat *st) {
	file_t fd = (file_t)h;

	if(fd >= MAX_ISO_FILES || !fh[fd].first_extent || fh[fd].broken) {
		errno = EBADF;
		return -1;
	}

	memset(st, 0, sizeof(struct stat));

	if(fh[fd].dir) {
		st->st_size = 0;
		st->st_dev = 'c' | ('d' << 8);
		st->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR |
			S_IXGRP | S_IXOTH;
		st->st_nlink = 1;
		st->st_blksize = 2048;
	}
	else {
		st->st_size = fh[fd].size;
		st->st_dev = 'c' | ('d' << 8);
		st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR |
			S_IXGRP | S_IXOTH;
		st->st_nlink = 1;
		st->st_blksize = 2048;
	}

	return 0;
}


/* Put everything together */
static vfs_handler_t vh = {
	/* Name handler */
	{
		{ 0 },		/* name */
		0,		/* tbfi */
		0x00010000,	/* Version 1.0 */
		0,		/* flags */
		NMMGR_TYPE_VFS,	/* VFS handler */
		NMMGR_LIST_INIT
	},

	0, NULL,	/* no cacheing, privdata */
	
	virt_iso_open,
	virt_iso_close,
	virt_iso_read,
	NULL,               /* write XXX */
	virt_iso_seek,
	virt_iso_tell,
	virt_iso_total,
	virt_iso_readdir,
	virt_iso_ioctl,
	NULL,               /* rename XXX */
	NULL,               /* unlink XXX */
	NULL,               /* mmap XXX */
	NULL,               /* complete */
	NULL,               /* stat XXX */
	NULL,               /* mkdir XXX */
	NULL,               /* rmdir XXX */
	virt_iso_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    virt_iso_rewinddir,
    virt_iso_fstat
};


/* Initialize the file system */
int fs_iso_init() {

	if(inited) {
		return 0;
	}

	/* Reset fd's */
	memset(fh, 0, sizeof(fh));
	
	/* Mark the first as active so we can have an error FD of zero */
	fh[0].first_extent = -1;
	
	SLIST_INIT(&virt_iso_list);
	mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);

	inited = 1;
	return 0;
}

/* De-init the file system */
int fs_iso_shutdown() {

	if(!inited) {
		return 0;
	}
	
	isofs_t *c, *n;
	int i;

	mutex_lock(&fh_mutex);
	c = SLIST_FIRST(&virt_iso_list); 

	while(c) {

		n = SLIST_NEXT(c, list); 
		nmmgr_handler_remove(&c->vfs->nmmgr); 

		if(c->ciso != NULL) {
			ciso_close(c->ciso);
		}

		if(c->cdi != NULL) {
			cdi_close(c->cdi);
		}
		
		if(c->gdi != NULL) {
			gdi_close(c->gdi);
		}

		fs_close(c->fd);
		
		/* Dealloc cache block space */
		mutex_lock(&c->cache_mutex);

		for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
			free(c->icache[i]);
			free(c->dcache[i]);
		}

		mutex_unlock(&c->cache_mutex);
		mutex_destroy(&c->cache_mutex);

		free(c->vfs);
		free(c); 
		c = n; 
	}
	
	SLIST_INIT(&virt_iso_list); 
	inited = 0;

	mutex_unlock(&fh_mutex);
	mutex_destroy(&fh_mutex);
	return 0;
}


int fs_iso_mount(const char *mountpoint, const char *filename) {

	isofs_t *ifs;
	vfs_handler_t *vfs;
	file_t fd;
	int i;
	
	if(mountpoint == NULL) {
		return -1;
	}

	fd = fs_open(filename, O_RDONLY);

	if(fd < 0) {
		return -1;
	}

	vfs = (vfs_handler_t*) malloc(sizeof(vfs_handler_t));

	if(vfs == NULL) {
		fs_close(fd);
		return -1;
	}
	
	ifs = (isofs_t*) malloc(sizeof(isofs_t));

	if(ifs == NULL) {
		fs_close(fd);
		free(vfs);
		return -1;
	}

	memcpy_sh4(vfs, &vh, sizeof(vfs_handler_t));
	strncpy(vfs->nmmgr.pathname, mountpoint, NAME_MAX);
	vfs->privdata = (void*)ifs;

	memset_sh4(ifs, 0, sizeof(isofs_t));
	ifs->fd = fd;
	ifs->vfs = vfs;
	
	ifs->ciso = ciso_open(fd);
	
	if(ifs->ciso != NULL) {
		
		switch(ifs->ciso->magic[0]) {
			case 'Z':
				ifs->type = ISOFS_IMAGE_TYPE_ZSO;
				break;
			case 'C':
			default:
				ifs->type = ISOFS_IMAGE_TYPE_CSO;
				break;
		}
		
	} else {
		
		ifs->cdi = cdi_open(fd);
		
		if(ifs->cdi != NULL) {
			ifs->type = ISOFS_IMAGE_TYPE_CDI;
		} else {
			
			ifs->gdi = gdi_open(fd, filename);
			
			if(ifs->gdi != NULL) {
				ifs->type = ISOFS_IMAGE_TYPE_GDI;
			} else {
				ifs->type = ISOFS_IMAGE_TYPE_ISO;
			}
		}
	}
	
	ifs->session_base = 0;

	/* Allocate cache block space */
	for (i = 0; i < NUM_CACHE_BLOCKS; i++) {
		ifs->icache[i] = malloc(sizeof(cache_block_t));
		ifs->icache[i]->sector = -1;
		ifs->dcache[i] = malloc(sizeof(cache_block_t));
		ifs->dcache[i]->sector = -1;
	}

	mutex_init(&ifs->cache_mutex, MUTEX_TYPE_DEFAULT);

	if(virt_iso_init_percd(ifs) < 0) {

		if(ifs->ciso != NULL) {
			ciso_close(ifs->ciso);
		}

		if(ifs->cdi != NULL) {
			cdi_close(ifs->cdi);
		}
		
		if(ifs->gdi != NULL) {
			gdi_close(ifs->gdi);
		}

		fs_close(fd);
		free(ifs->vfs);

		/* Dealloc cache block space */
		for (i = 0; i < NUM_CACHE_BLOCKS; i++) {
			free(ifs->icache[i]);
			free(ifs->dcache[i]);
		}

		mutex_destroy(&ifs->cache_mutex);

		free(ifs);
		return -1;
	}

	mutex_lock(&fh_mutex);

	SLIST_INSERT_HEAD(&virt_iso_list, ifs, list);
	nmmgr_handler_add(&ifs->vfs->nmmgr); // TODO check for errors

	mutex_unlock(&fh_mutex);
#ifdef DEBUG
	ds_printf("DS_ISOFS: Mounted %s\n", mountpoint);
#endif
	return 0;
}



int fs_iso_unmount(const char *mountpoint) {

	isofs_t *n;
	int i;
	
	if(mountpoint == NULL) {
		return -1;
	}

	mutex_lock(&fh_mutex);

	SLIST_FOREACH(n, &virt_iso_list, list) {
					 
		if (!strncasecmp(mountpoint, n->vfs->nmmgr.pathname, NAME_MAX)) {
			
			nmmgr_handler_remove(&n->vfs->nmmgr); // TODO check for errors
			free(n->vfs);

			if(n->ciso != NULL) {
				ciso_close(n->ciso);
			}

			if(n->cdi != NULL) {
				cdi_close(n->cdi);
			}
			
			if(n->gdi != NULL) {
				gdi_close(n->gdi);
			}

			fs_close(n->fd);
	
			/* Dealloc cache block space */
			mutex_lock(&n->cache_mutex);

			for (i = 0; i < NUM_CACHE_BLOCKS; i++) {
				free(n->icache[i]);
				free(n->dcache[i]);
			}

			mutex_unlock(&n->cache_mutex);
			mutex_destroy(&n->cache_mutex);
			
			SLIST_REMOVE(&virt_iso_list, n, isofs, list); 
			free(n);
#ifdef DEBUG
			ds_printf("DS_ISOFS: Unmounted %s\n", mountpoint);
#endif
			mutex_unlock(&fh_mutex);
			return 0;
		}
	}

	mutex_unlock(&fh_mutex);
	return -1;
}
