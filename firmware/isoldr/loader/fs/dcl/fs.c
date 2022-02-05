/* 
 * DreamShell ISO Loader
 * dcload file system
 * (c)2009-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/irq.h>
#include "dcload.h"

#define dclsc(...) ({                                  \
		int rv, old;                                   \
		old = irq_disable();                           \
		do {} while ((*(vuint32 *)0xa05f688c) & 0x20); \
		rv = dcloadsyscall(__VA_ARGS__);               \
		irq_restore(old);                              \
		rv;                                            \
})

int fs_init() {
	return dcload_init();
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
