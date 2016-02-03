/* KallistiOS ##version##

	vmdfs.c
	Copyright (C) 2003 Dan Potter
	Copyright (C) 2015 megavolt85
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <kos/mutex.h>
#include "fs_vmd.h"
#include <ds.h>

#define BLOCK_SIZE 512

/*

This is a whole new module that sits between the fs_vmd module and the maple
VMD driver. It's based loosely on the stuff in the old fs_vmd, but it's been
rewritten and reworked to be clearer, more clean, use threads better, etc.

Unlike the fs_vmd module, this code is stateless. You make a call and you get
back data (or have written it). There are no handles involved or anything
else like that. The new fs_vmd sits on top of this and provides a (mostly)
nice VFS interface similar to the old fs_vmd.

This module tends to do more work than it really needs to for some
functions (like reading a named file) but it does it that way to have very
clear, concise code that can be audited for bugs more easily. It's not
like you load and save on the VMD every frame or something. ;) But
the user may never give your program another frame of time if it corrupts
their save games! If you want better control to save loading and saving
stuff for a big batch of changes, then use the low-level funcs.

Function comments located in vmdfs.h.

*/

/* ****************** Low level functions ******************** */


/* We need some sort of access control here for threads. This is somewhat
   less than optimal (one mutex for all VMDs) but I doubt it'll really
   be much of an issue :) */
static mutex_t mutex;

static int vmdfs_dir_blocks(vmd_root_t * root_buf) {
    return root_buf->dir_size * 512;
}

static int vmdfs_fat_blocks(vmd_root_t * root_buf) {
    return root_buf->fat_size * 512;
}

/* Common code for both dir_read and dir_write */
static int vmdfs_dir_read(const char *vmdfile, vmd_root_t * root, vmd_dir_t * dir_buf) {
    uint16  dir_block, dir_size;
    unsigned int i;
    int needsop;//, rv;
    int write = 0;

    /* Find the directory starting block and length */
    dir_block = root->dir_loc;
    dir_size = root->dir_size;

    /* The dir is stored backwards, so we start at the end and go back. */
    while(dir_size > 0) {
        if(write) {
            /* Scan this block for changes */
            for(i = 0, needsop = 0; i < 512 / sizeof(vmd_dir_t); i++) {
                if(dir_buf[i].dirty) {
                    needsop = 1;
                }

                dir_buf[i].dirty = 0;
            }
        }
        else
            needsop = 1;

		if(needsop) {
			
			if(!write){
			
				file_t f = fs_open(vmdfile,O_RDONLY);
				fs_seek(f,dir_block*BLOCK_SIZE,SEEK_SET);
				fs_read(f,(uint8 *)dir_buf,BLOCK_SIZE);
				fs_close(f);
			}
        }
      
        dir_block--;
        dir_size--;
        dir_buf += 512 / sizeof(vmd_dir_t); /* == 16 */
    }

    return 0;
}

/* Common code for both fat_read and fat_write */
static int vmdfs_fat_read(const char *vmdfile, vmd_root_t * root, uint16 * fat_buf) {
    uint16  fat_block, fat_size;

    /* Find the FAT starting block and length */
	fat_block = root->fat_loc;
    fat_size = root->fat_size;

    /* We can't reliably handle VMDs with a larger FAT... */
    if(fat_size > 1) {
        dbglog(DBG_ERROR, "vmdfs_fat_read: VMD has >1 (%d) FAT blocks\n",(int)fat_size);
        return -1;
    }
    
    file_t f = fs_open(vmdfile,O_RDONLY);
	fs_seek(f,fat_block*BLOCK_SIZE,SEEK_SET);
	fs_read(f,(uint8 *)fat_buf,BLOCK_SIZE);
	fs_close(f);
    
    if(!*fat_buf) {
        dbglog(DBG_ERROR, "vmdfs_fat_read: can't read block %d\n",(int)fat_block);
        return -2;
    }

    return 0;
}

static int vmdfs_dir_find(vmd_root_t * root, vmd_dir_t * dir, const char * fn) {
    int i;
    int dcnt;

    dcnt = root->dir_size * 512 / sizeof(vmd_dir_t);

    for(i = 0; i < dcnt; i++) {
        /* Not a file -> skip it */
        if(dir[i].filetype == 0)
            continue;

        /* Check the filename */
        if(!strncmp(fn, dir[i].filename, 12))
            return i;
    }

    /* Didn't find anything */
    return -1;
}

static int vmdfs_file_read(const char *vmdfile, uint16 * fat, vmd_dir_t * dirent, void * outbuf) {
    int curblk, blkleft;
    uint8   * out;

    out = (uint8 *)outbuf;

    /* Find the first block */
    curblk = dirent->firstblk;

    /* And the blocks remaining */
    blkleft = dirent->filesize;

    /* While we've got stuff remaining... */
    while(blkleft > 0) {
        /* Make sure the FAT matches up with the directory */
        if(curblk == 0xfffc || curblk == 0xfffa) {
            char fn[13] = {0};
            memcpy(fn, dirent->filename, 12);
            dbglog(DBG_ERROR, "vmdfs_file_read: file '%s' ends prematurely in fat\n",fn);
            return -1;
        }
        
		/* Read the block */
		
		file_t f = fs_open(vmdfile,O_RDONLY);
		fs_seek(f,curblk*BLOCK_SIZE,SEEK_SET);
		int rv = fs_read(f,(uint8 *)out,BLOCK_SIZE);
		fs_close(f);
        
        if(rv != BLOCK_SIZE) {
            dbglog(DBG_ERROR, "vmdfs_file_read: can't read block %d\n",curblk);
            return -2;
        }

        /* Scoot our counters */
        curblk = fat[curblk];
        blkleft--;
        out += 512;
    }

    /* Make sure the FAT matches up with the directory */
    if(curblk != 0xfffa) {
        char fn[13] = {0};
        memcpy(fn, dirent->filename, 12);
        dbglog(DBG_ERROR, "vmdfs_file_read: file '%s' is sized shorter than in the FAT\n",fn);
        return -3;
    }

    return 0;
}

static int vmdfs_mutex_lock() {
    return mutex_lock(&mutex);
}

static int vmdfs_mutex_unlock() {
    return mutex_unlock(&mutex);
}

/* ****************** Higher level functions ******************** */

/* Internal function gets everything setup for you */
static int vmdfs_setup(const char *vmdfile, vmd_root_t * root, vmd_dir_t ** dir, int * dirsize,
                       uint16 ** fat, int * fatsize) {
    /* Check to make sure this is a valid device right now */
    if(!FileExists(vmdfile)) {

		dbglog(DBG_ERROR, "vmdfs_setup: vmd file is invalid\n");

        return -1;
    }

    vmdfs_mutex_lock();

    /* Read its root block */
    file_t f = fs_open(vmdfile,O_RDONLY);
    fs_seek(f,255*BLOCK_SIZE,SEEK_SET);
    fs_read(f,(uint8 *)root,BLOCK_SIZE);
    fs_close(f);
    
    if(!root)
        goto dead;

    if(dir) {
        /* Alloc enough space for the whole dir */
        *dirsize = vmdfs_dir_blocks(root);
        *dir = (vmd_dir_t *)malloc(*dirsize);

        if(!*dir) {
            dbglog(DBG_ERROR, "vmdfs_setup: can't alloc %d bytes for dir\n",*dirsize);
            goto dead;
        }

        /* Read it */
        if(vmdfs_dir_read(vmdfile, root, *dir) < 0) {
            free(*dir);
            *dir = NULL;
            goto dead;
        }
    }

    if(fat) {
        /* Alloc enough space for the fat */
        *fatsize = vmdfs_fat_blocks(root);
        *fat = (uint16 *)malloc(*fatsize);

        if(!*fat) {
            dbglog(DBG_ERROR, "vmdfs_setup: can't alloc %d bytes for FAT\n",*fatsize);
            goto dead;
        }

        /* Read it */
        if(vmdfs_fat_read(vmdfile, root, *fat) < 0)
            goto dead;
    }

    /* Ok, everything's cool */
    return 0;

dead:
    vmdfs_mutex_unlock();
    return -1;
}

/* Internal function to tear everything down for you */
static void vmdfs_teardown(vmd_dir_t * dir, uint16 * fat) {
    if(dir)
        free(dir);

    if(fat)
        free(fat);

    vmdfs_mutex_unlock();
}

int vmdfs_readdir(const char *vmdfile, vmd_dir_t ** outbuf, int * outcnt) {
    vmd_root_t root;
    vmd_dir_t *dir;
    int dircnt, dirsize, rv = 0;
    unsigned int i, j;

    *outbuf = NULL;
    *outcnt = 0;

    /* Init everything */
    if(vmdfs_setup(vmdfile, &root, &dir, &dirsize, NULL, NULL) < 0)
        return -1;

    /* Go through and move all entries to the lowest-numbered spots. */
    dircnt = 0;

    for(i = 0; i < dirsize / sizeof(vmd_dir_t); i++) {
        /* Skip blanks */
        if(dir[i].filetype == 0)
            continue;

        /* Not a blank -- look for an earlier slot that's empty. If
           we don't find one, just leave it alone. */
        for(j = 0; j < i; j++) {
            if(dir[j].filetype == 0) {
                memcpy(dir + j, dir + i, sizeof(vmd_dir_t));
                dir[i].filetype = 0;
                break;
            }
        }

        /* Update the entry count */
        dircnt++;
    }

    /* Resize the buffer to match the number of entries */
    *outcnt = dircnt;
    *outbuf = (vmd_dir_t *)realloc(dir, dircnt * sizeof(vmd_dir_t));

    if(!*outbuf && dircnt) {
        dbglog(DBG_ERROR, "vmdfs_readdir: can't realloc %d bytes for dir\n",dircnt * sizeof(vmd_dir_t));
        free(dir);
        rv = -2;
        goto ex;
    }

ex:
    vmdfs_teardown(NULL, NULL);
    return rv;
}

/* Shared code between read/read_dirent */
static int vmdfs_read_common(const char *vmdfile, vmd_dir_t * dirent, uint16 * fat, void ** outbuf, int * outsize) {
    /* Allocate the output space */
    *outsize = dirent->filesize * 512;
    *outbuf = malloc(*outsize);

    if(!*outbuf) {
        dbglog(DBG_ERROR, "vmdfs_read: can't alloc %d bytes for reading a file\n",*outsize);
        return -1;
    }

    /* Ok, go ahead and read it */
    if(vmdfs_file_read(vmdfile, fat, dirent, *outbuf) < 0) {
        free(*outbuf);
        *outbuf = NULL;
        *outsize = 0;
        return -1;
    }

    return 0;
}

int vmdfs_read(const char *vmdfile, const char * fn, void ** outbuf, int * outsize) {
    vmd_root_t  root;
    vmd_dir_t   * dir = NULL;
    uint16      * fat = NULL;
    int     fatsize, dirsize, idx, rv = 0;

    *outbuf = NULL;
    *outsize = 0;

    /* Init everything */
    if(vmdfs_setup(vmdfile, &root, &dir, &dirsize, &fat, &fatsize) < 0)
        return -1;

    /* Look for the file we want */
    idx = vmdfs_dir_find(&root, dir, fn);

    if(idx < 0) {
        dbglog(DBG_ERROR, "vmdfs_read: can't find file '%s'\n",fn);
        rv = -2;
        goto ex;
    }

    if(vmdfs_read_common(vmdfile, dir + idx, fat, outbuf, outsize) < 0) {
        rv = -3;
        goto ex;
    }

ex:
    vmdfs_teardown(dir, fat);
    return rv;
}

int vmdfs_init() {
    mutex_init(&mutex, MUTEX_TYPE_NORMAL);
    return 0;
}

int vmdfs_shutdown() {
    mutex_destroy(&mutex);
    return 0;
}

