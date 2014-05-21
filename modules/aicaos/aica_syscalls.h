
#ifndef AICA_SYSCALLS_H
#define AICA_SYSCALLS_H

#include "aica_common.h"

void aica_init_syscalls(void);

struct open_param {
	const char *name;
	const int namelen;
	int flags;
	int mode;
};

struct fstat_param {
	int file;
	struct stat *st;
};

struct stat_param {
	const char *name;
	const int namelen;
	struct stat *st;
};

struct link_param {
	const char *old;
	const int namelen_old;
	const char *new;
	const int namelen_new;
};

struct lseek_param {
	int file;
	int ptr;
	int dir;
};

struct read_param {
	int file;
	void *ptr;
	size_t len;
};

struct write_param {
	int file;
	const void *ptr;
	size_t len;
};

#endif
