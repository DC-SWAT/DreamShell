/* KallistiOS ##version##

	fs_vmd.c
	Copyright (C) 2003 Dan Potter
	Copyright (C) 2012, 2013, 2014 Lawrence Sebald
	Copyright (C) 2015 megavolt85

*/
#include <kos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <time.h>
#include <arch/types.h>
#include <kos/mutex.h>
#include "fs_vmd.h"
#include <sys/queue.h>
#include <errno.h>
#include "ds.h"
/*

At the moment this FS is kind of a hack because of the simplicity (and weirdness)
of the VMD file system. For one, all files must be pretty small, so it loads
and caches the entire file on open. For two, all files are a multiple of 512
bytes in size (no way around this one). On top of it all, files may have an
obnoxious header and you can't just read and write them with abandon like
a normal file system. We'll have to find ways around this later on, but for
now it gives the file data to you raw.

Note: this new version now talks directly to the vmdfs module and doesn't do
any block-level I/O anymore. This layer and that one are interchangeable
and may be used pretty much simultaneously in the same program.

*/

/* Enable this if you want copious debug output */
#define VMDFS_DEBUG 

#define VMD_DIR     0
#define VMD_FILE    1
#define VMD_ANY     -1  /* Used for checking validity */

char vmdfile[NAME_MAX];

/* File handles */
typedef struct vmd_fh_str {
    uint32 strtype;                     /* 0==dir, 1==file */
    TAILQ_ENTRY(vmd_fh_str) listent;    /* list entry */
    
    int mode;                           /* mode the file was opened with */
    char path[17];                      /* full path of the file */
    char name[13];                      /* name of the file */
    off_t loc;                          /* current position in the file (bytes) */
    uint32 filesize;                    /* file length from dirent (in 512-byte blks) */
    uint8 *data;                        /* copy of the whole file */
} vmd_fh_t;

/* Directory handles */
typedef struct vmd_dh_str {
    uint32 strtype;                     /* 0==dir, 1==file */
    TAILQ_ENTRY(vmd_dh_str) listent;    /* list entry */
    
    int rootdir;                        /* 1 if we're reading /vmd */
    dirent_t dirent;                    /* Dirent to pass back */
    vmd_dir_t *dirblocks;               /* Copy of all directory blocks */
    uint16 entry;                       /* Current dirent */
    uint16 dircnt;                      /* Count of dir entries */
} vmd_dh_t;

/* Linked list of open files (controlled by "mutex") */
TAILQ_HEAD(vmd_fh_list, vmd_fh_str) vmd_fh;

/* Thread mutex for vmd_fh access */
static mutex_t fh_mutex;

/* opendir function */
static vmd_fh_t *vmd_open_dir() {
    vmd_dir_t   * dirents;
    int     dircnt;
    vmd_dh_t    * dh;

    /* Read the VMD's directory */
    if(vmdfs_readdir(vmdfile, &dirents, &dircnt) < 0){
        return NULL;
	}
    /* Allocate a handle for the dir blocks */
    dh = malloc(sizeof(vmd_dh_t));
    dh->strtype = VMD_DIR;
    dh->dirblocks = dirents;
    dh->rootdir = 0;
    dh->entry = 0;
    dh->dircnt = dircnt;

    return (vmd_fh_t *)dh;
}

/* openfile function */
static vmd_fh_t *vmd_open_file(const char *path, int mode) {
    vmd_fh_t    * fd;       /* file descriptor */
    int     realmode, rv;
    void        * data;
    int     datasize;

    /* Malloc a new fh struct */
    fd = malloc(sizeof(vmd_fh_t));

    /* Fill in the filehandle struct */
    fd->strtype = VMD_FILE;
    fd->mode = mode;
    strcpy(fd->path, "/");
    strncpy(fd->name, path + 1, 12);
    fd->loc = 0;

    /* What mode are we opening in? If we're reading or writing without O_TRUNC
       then we need to read the old file if there is one. */
    realmode = mode & O_MODE_MASK;

    if(realmode == O_RDONLY || ((realmode == O_RDWR || realmode == O_WRONLY) && !(mode & O_TRUNC))) {
        /* Try to open it */
        rv = vmdfs_read(vmdfile, fd->name, &data, &datasize);

        if(rv < 0) {
            if(realmode == O_RDWR || realmode == O_WRONLY) {
                /* In some modes failure is ok -- just setup a blank first block. */
                data = malloc(512);
                datasize = 512;
                memset(data, 0, 512);
            }
            else {
                free(fd);
                return NULL;
            }
        }
    }
    else {
        /* We're writing with truncate... just setup a blank first block. */
        data = malloc(512);
        datasize = 512;
        memset(data, 0, 512);
    }

    fd->data = (uint8 *)data;
    fd->filesize = datasize / 512;

    if(fd->filesize == 0) {
        dbglog(DBG_WARNING, "VMDFS: can't open zero-length file %s\n", path);
        free(fd);
        return NULL;
    }

    return fd;
}

/* open function */
static void * vmd_open(vfs_handler_t * vfs, const char *path, int mode) {

    if(strlen(vmdfile) < 2) return 0;
    
    vmd_fh_t    *fh;

    (void)vfs;

	/* Check for open as dir */
	if(strlen(path) == 0 || (strlen(path) == 1 && path[0] == '/')) {
		if(!(mode & O_DIR)) return 0;

		fh = vmd_open_dir();
	}else{
		
		if(mode & O_DIR) return 0;

		fh = vmd_open_file(path, mode);
	}
    

    if(fh == NULL) return 0;

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&vmd_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Verify that a given hnd is actually in the list */
static int vmd_verify_hnd(void * hnd, int type) {
    vmd_fh_t    *cur;
    int rv = 0;

    mutex_lock(&fh_mutex);
    TAILQ_FOREACH(cur, &vmd_fh, listent) {
        if((void *)cur == hnd) {
            rv = 1;
            break;
        }
    }
    mutex_unlock(&fh_mutex);

    if(rv)
        return type == VMD_ANY ? 1 : ((int)cur->strtype == type);
    else
        return 0;
}

/* close a file */
static int vmd_close(void * hnd) {
    vmd_fh_t *fh;
    int retval = 0;//, st;

    /* Check the handle */
    if(!vmd_verify_hnd(hnd, VMD_ANY)) {
        errno = EBADF;
        return -1;
    }

    fh = (vmd_fh_t *)hnd;

    switch(fh->strtype) {
        case VMD_DIR: {
            vmd_dh_t * dir = (vmd_dh_t *)hnd;

            if(dir->dirblocks)
                free(dir->dirblocks);

            break;
        }

        case VMD_FILE:
/*            if((fh->mode & O_MODE_MASK) == O_WRONLY ||
                    (fh->mode & O_MODE_MASK) == O_RDWR) {
                if ((st = vmd_write_close(hnd))) {
                    if (st == -7)
                        errno = ENOSPC;
                    else
                        errno = EIO;
                    retval = -1;
                }
            }*/

            free(fh->data);
            break;

    }

    /* Look for the one to get rid of */
    mutex_lock(&fh_mutex);
    TAILQ_REMOVE(&vmd_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    free(fh);
    return retval;
}

/* read function */
static ssize_t vmd_read(void * hnd, void *buffer, size_t cnt) {
    vmd_fh_t *fh;

    /* Check the handle */
    if(!vmd_verify_hnd(hnd, VMD_FILE))
        return -1;

    fh = (vmd_fh_t *)hnd;

    /* make sure we're opened for reading */
    if((fh->mode & O_MODE_MASK) != O_RDONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return 0;

    /* Check size */
    cnt = (fh->loc + cnt) > (fh->filesize * 512) ?
          (fh->filesize * 512 - fh->loc) : cnt;

    /* Reads past EOF return 0 */
    if((long)cnt < 0)
        return 0;

    /* Copy out the data */
    memcpy(buffer, fh->data + fh->loc, cnt);
    fh->loc += cnt;

    return cnt;
}

/* read a directory handle */
static dirent_t *vmd_readdir(void * fd) {
    vmd_dh_t    *dh;
    vmd_dir_t   *dir;

    /* Check the handle */
    if(!vmd_verify_hnd(fd, VMD_DIR)) {
        errno = EBADF;
        return NULL;
    }

    dh = (vmd_dh_t*)fd;

    /* Check if we have any entries left */
    if(dh->entry >= dh->dircnt)
        return NULL;

    /* Ok, extract it and fill the dirent struct */
    dir = dh->dirblocks + dh->entry;

    if(dh->rootdir) {
        dh->dirent.size = -1;
        dh->dirent.attr = O_DIR;
    }
    else {
        dh->dirent.size = dir->filesize * 512;
        dh->dirent.attr = 0;
    }

    strncpy(dh->dirent.name, dir->filename, 12);
    dh->dirent.name[12] = 0;
    dh->dirent.time = 0;    /* FIXME */

    /* Move to the next entry */
    dh->entry++;

    return &dh->dirent;
}

static int vmd_rewinddir(void * fd) {
    vmd_dh_t *dh;

    /* Check the handle */
    if(!vmd_verify_hnd(fd, VMD_DIR)) {
        errno = EBADF;
        return -1;
    }

    /* Rewind to the beginning of the directory. */
    dh = (vmd_dh_t*)fd;
    dh->entry = 0;

    /* TODO: Technically, we need to re-scan the directory here, but for now we
       will punt on that requirement. */

    return 0;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/vmd",         /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    vmd_open,
    vmd_close,
    vmd_read,
	NULL,				/* write */
	NULL,				/* seek */
	NULL,				/* tell */
	NULL,				/* total */
    vmd_readdir,
    NULL,               /* ioctl */
    NULL,               /* rename/move */
    NULL,				/* unlink */
    NULL,				/* mmap */
    NULL,               /* complete */
    NULL,				/* stat */
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    NULL,				/* fcntl */
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    vmd_rewinddir
};

int fs_vmd_init() {
	
	vmdfs_init();
	
    TAILQ_INIT(&vmd_fh);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);
    return nmmgr_handler_add(&vh.nmmgr);
}

int fs_vmd_shutdown() {
    vmd_fh_t * c, * n;

    c = TAILQ_FIRST(&vmd_fh);

    while(c) {
        n = TAILQ_NEXT(c, listent);

        switch(c->strtype) {
            case VMD_DIR: {
                vmd_dh_t * dir = (vmd_dh_t *)c;
                free(dir->dirblocks);
                break;
            }

            case VMD_FILE:

                if((c->mode & O_MODE_MASK) == O_WRONLY ||
                        (c->mode & O_MODE_MASK) == O_RDWR) {
                    dbglog(DBG_ERROR, "fs_vmd_shutdown: still-open file '%s' not written!\n", c->path);
                }

                free(c->data);
                break;
        }

        free(c);
        c = n;
    }

    mutex_destroy(&fh_mutex);
    
    vmdfs_shutdown();

    return nmmgr_handler_remove(&vh.nmmgr);
}

void fs_vmd_vmdfile(const char *infile){
	
	sprintf(vmdfile,"%s",infile);
}

