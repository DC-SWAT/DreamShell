#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/reent.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int _execve_r(struct _reent *r, const char *name, char * const argv[], char * const env[])
{
	r->_errno = ENOMEM;
	return -1;
}

int _fork_r(struct _reent *r)
{
	r->_errno = EAGAIN;
	return -1;
}

int _getpid_r(struct _reent *r)
{
	return 1;
}

int _kill_r(struct _reent *r, int pid, int sig)
{
	r->_errno = EINVAL;
	return -1;
}

clock_t _times_r(struct _reent *r, struct tms *buf)
{
	r->_errno = EACCES;
	return -1;
}

int _wait_r(struct _reent *r, int *status)
{
	r->_errno = ECHILD;
	return -1;
}


_READ_WRITE_RETURN_TYPE _write_r(struct _reent *, int, const void *, size_t);
static char *heap_end = (char*) 0;

void * _sbrk_r(struct _reent *r, ptrdiff_t incr)
{
	extern char _end;		/* Defined by the linker */
	char *prev_heap_end;

	if (heap_end == 0)
		heap_end = &_end;
	prev_heap_end = heap_end;
	heap_end += incr;

	return prev_heap_end;
}


/* AICA-specific syscalls */
#include "../aica_syscalls.h"

AICA_ADD_REMOTE(sh4_open, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_close, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_fstat, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_stat, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_isatty, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_link, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_lseek, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_read, PRIORITY_DEFAULT);
AICA_ADD_REMOTE(sh4_write, PRIORITY_DEFAULT);

int _open_r(struct _reent *r, const char *name, int flags, int mode)
{
	struct open_param params = { name, strlen(name), flags, mode, };
	return sh4_open(NULL, &params);
}

int _close_r(struct _reent *r, int file)
{
	return sh4_close(NULL, &file);
}

int _fstat_r(struct _reent *r, int file, struct stat *st)
{
	struct fstat_param params = { file, st, };
	return sh4_fstat(NULL, &params);
}

int _stat_r(struct _reent *r, const char *file, struct stat *st)
{
	struct stat_param params = { file, strlen(file), st, };
	return sh4_stat(NULL, &params);
}

int _isatty_r(struct _reent *r, int file)
{
	return sh4_isatty(NULL, &file);
}

int _link_r(struct _reent *r, const char *old, const char *new)
{
	struct link_param params = { old, strlen(old), new, strlen(new), };
	return sh4_link(NULL, &params);
}

off_t _lseek_r(struct _reent *r, int file, off_t ptr, int dir)
{
	struct lseek_param params = { file, ptr, dir, };
	return sh4_lseek(NULL, &params);
}

_READ_WRITE_RETURN_TYPE _read_r(struct _reent *r, int file, void *ptr, size_t len)
{
	struct read_param params = { file, ptr, len, };
	return sh4_read(NULL, &params);
}

_READ_WRITE_RETURN_TYPE _write_r(struct _reent *r, int file, const void *ptr, size_t len)
{
	struct write_param params = { file, ptr, len, };
	return sh4_write(NULL, &params);
}
