/* 
 * DreamShell ISO Loader
 * dcload file system
 * (c)2009-2020 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/irq.h>
#include <drivers/aica.h>
#include "fs_dcload.h"


#define dclsc(...) ({                                 \
		int rv, old;                                   \
		old = irq_disable();                           \
		AICA_DMA_ADSUSP = 1;                           \
		do {} while ((*(vuint32 *)0xa05f688c) & 0x20); \
		rv = dcloadsyscall(__VA_ARGS__);               \
		AICA_DMA_ADSUSP = 0;                           \
		irq_restore(old);                              \
		rv;                                            \
	})


/* Printk replacement */
int dcload_write_buffer(const uint8 *data, int len) {
    return dclsc(DCLOAD_WRITE, 1, data, len);
}

int dcload_reinit() {
    return dclsc(DCLOAD_REINIT, 0, 0, 0);
}

#ifdef USE_GDB
size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size) {
    size_t ret = -1;

    /* we have to pack the sizes together because the dcloadsyscall handler
       can only take 4 parameters */
    ret = dclsc(DCLOAD_GDBPACKET, in_buf, (in_size << 16) | (out_size & 0xffff), out_buf);

    
    return ret;
}
#endif


int dcload_type = DCLOAD_TYPE_NONE;


int fs_init() {

	if(*DCLOADMAGICADDR != DCLOADMAGICVALUE)
		return -1;

	/* Give dcload the 64k it needs to compress data (if on serial) */
	if(dclsc(DCLOAD_ASSIGNWRKMEM, (int*)0x8cfb0000) == -1) {
		dcload_type = DCLOAD_TYPE_IP;
		printf("dc-load-ip initialized\n");
	} else {
		dcload_type = DCLOAD_TYPE_SER;
		printf("dc-load-serial initialized\n");
	}

	return 0;
}


int open(const char *path, int mode) {

	uint32 dcload_mode = 0;

	int fd = -1;

	if((mode & O_MODE_MASK) == O_RDONLY)
		dcload_mode = 0;

	if((mode & O_MODE_MASK) == O_RDWR)
		dcload_mode = 2 | 0x0200;

	if((mode & O_MODE_MASK) == O_WRONLY)
		dcload_mode = 1 | 0x0200;

	if((mode & O_MODE_MASK) == O_APPEND)
		dcload_mode =  2 | 8 | 0x0200;

	if(mode & O_TRUNC)
		dcload_mode |= 0x0400;

	fd = dclsc(DCLOAD_OPEN, path, dcload_mode, 0644);

	if(fd < 0) {
		return FS_ERR_NOFILE;
	}
	
	return fd;
}


int close(int fd) {

	if(dclsc(fd > 100 ? DCLOAD_CLOSEDIR : DCLOAD_CLOSE, fd) < 0) {
		return FS_ERR_PARAM;
	}
	
	return 0;
}


int read(int fd, void *ptr, size_t size) {
	return dclsc(DCLOAD_READ, fd, ptr, size);
}

#if !_FS_READONLY

int write(int fd, const void *ptr, size_t size) {
	return dclsc(DCLOAD_WRITE, fd, ptr, size);
}

#endif

long int lseek(int fd, long int offset, int whence) {
	return dclsc(DCLOAD_LSEEK, fd, (int)offset, whence);
}

long int tell(int fd) {
	return dclsc(DCLOAD_LSEEK, fd, 0, SEEK_CUR);
}

unsigned long total(int fd) {
	
	long cur = dclsc(DCLOAD_LSEEK, fd, 0, SEEK_CUR);
	long ret = dclsc(DCLOAD_LSEEK, fd, 0, SEEK_END);
	dclsc(DCLOAD_LSEEK, fd, cur, SEEK_SET);
	return (unsigned long)ret;
}
