/* KallistiOS ##version##

	fs_vmd.h
	(c)2000-2001 Jordan DeLong
	Copyright (C) 2015 megavolt85
*/

/* \file   fs_vmd.h
    \brief  VMD filesystem driver.

    The VMD filesystem driver mounts itself on /vmd of the VFS. Each memory card
    has its own subdirectory off of that directory (i.e, /vmd/a1 for slot 1 of
    the first controller). VMDs themselves have no subdirectories, so the driver
    itself is fairly simple.

    Files on a VMD must be multiples of 512 bytes in size, and should have a
    header attached so that they show up in the BIOS menu.

    This layer is built off of the vmdfs layer, which does all the low-level
    operations. It is generally easier to work with things at this level though,
    so that you can use the normal libc file access functions.

*/



#ifndef __DC_FS_VMD_H
#define __DC_FS_VMD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/fs.h>

/* \cond */
/* Initialization */
int fs_vmd_init();
int fs_vmd_shutdown();
void fs_vmd_vmdfile(const char *infile);
/* \endcond */


/* \cond */
#define __packed__ __attribute__((packed))
/* \endcond */

/** \brief  BCD timestamp, used several places in the vmdfs.
    \headerfile vmdfs.h
*/
typedef struct {
    uint8   cent;   /**< \brief Century */
    uint8   year;   /**< \brief Year, within century */
    uint8   month;  /**< \brief Month of the year */
    uint8   day;    /**< \brief Day of the month */
    uint8   hour;   /**< \brief Hour of the day */
    uint8   min;    /**< \brief Minutes */
    uint8   sec;    /**< \brief Seconds */
    uint8   dow;    /**< \brief Day of week (0 = monday, etc) */
} __packed__ vmd_timestamp_t;

/** \brief  VMD FS Root block layout.
    \headerfile vmdfs.h
*/
typedef struct {
    uint8           magic[16];      /**< \brief All should contain 0x55 */
    uint8           use_custom;     /**< \brief 0 = standard, 1 = custom */
    uint8           custom_color[4];/**< \brief blue, green, red, alpha */
    uint8           pad1[27];       /**< \brief All zeros */
    vmd_timestamp_t timestamp;      /**< \brief BCD timestamp */
    uint8           pad2[8];        /**< \brief All zeros */
    uint8           unk1[6];        /**< \brief ??? */
    uint16          fat_loc;        /**< \brief FAT location */
    uint16          fat_size;       /**< \brief FAT size in blocks */
    uint16          dir_loc;        /**< \brief Directory location */
    uint16          dir_size;       /**< \brief Directory size in blocks */
    uint16          icon_shape;     /**< \brief Icon shape for this VMS */
    uint16          blk_cnt;        /**< \brief Number of user blocks */
    uint8           unk2[430];      /**< \brief ??? */
} __packed__ vmd_root_t;

/** \brief  VMD FS Directory entries, 32 bytes each.
    \headerfile vmdfs.h
*/
typedef struct {
    uint8           filetype;       /**< \brief 0x00 = no file; 0x33 = data; 0xcc = a game */
    uint8           copyprotect;    /**< \brief 0x00 = copyable; 0xff = copy protected */
    uint16          firstblk;       /**< \brief Location of the first block in the file */
    char            filename[12];   /**< \brief Note: there is no null terminator */
    vmd_timestamp_t timestamp;      /**< \brief File time */
    uint16          filesize;       /**< \brief Size of the file in blocks */
    uint16          hdroff;         /**< \brief Offset of header, in blocks from start of file */
    uint8           dirty;          /**< \brief See header notes */
    uint8           pad1[3];        /**< \brief All zeros */
} __packed__ vmd_dir_t;

#undef __packed__


int vmdfs_readdir(const char *vmdfile, vmd_dir_t ** outbuf, int * outcnt);

int vmdfs_read(const char *vmdfile, const char * fn, void ** outbuf, int * outsize);

int vmdfs_init();

int vmdfs_shutdown();

__END_DECLS

#endif  /* __DC_FS_VMD_H */

