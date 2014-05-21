#include <alloca.h>
#include <kos.h>

#include "../aica_syscalls.h"

static AICA_SHARED(sh4_open)
{
	char *fn;
	struct open_param *p = (struct open_param *) in;

	fn = alloca(p->namelen+1);
	aica_download(fn, p->name, p->namelen);
	fn[p->namelen] = '\0';

	return open(fn, p->flags, p->mode);
}

static AICA_SHARED(sh4_close)
{
	return close(*(int *) in);
}

static AICA_SHARED(sh4_fstat)
{
	int result;
	struct stat st;
	struct fstat_param *p = (struct fstat_param *) in;

	result = fstat(p->file, &st);
	if (!result)
		aica_upload(p->st, &st, sizeof(st));
	return result;
}

static AICA_SHARED(sh4_stat)
{
	int result;
	struct stat st;
	char *fn;
	struct stat_param *p = (struct stat_param *) in;

	fn = alloca(p->namelen+1);
	aica_download(fn, p->name, p->namelen);
	fn[p->namelen] = '\0';

	result = stat(fn, &st);
	if (!result)
		aica_upload(p->st, &st, sizeof(st));
	return result;
}

static AICA_SHARED(sh4_isatty)
{
	return isatty(*(int *) in);
}

static AICA_SHARED(sh4_link)
{
	struct link_param *p = (struct link_param *) in;
	char *fn_old, *fn_new;

	fn_old = alloca(p->namelen_old+1);
	fn_new = alloca(p->namelen_new+1);
	aica_download(fn_old, p->old, p->namelen_old);
	aica_download(fn_new, p->new, p->namelen_new);
	fn_old[p->namelen_old] = '\0';
	fn_new[p->namelen_new] = '\0';

	return link(fn_old, fn_new);
}

static AICA_SHARED(sh4_lseek)
{
	struct lseek_param *p = (struct lseek_param *) in;
	return lseek(p->file, p->ptr, p->dir);
}

/* TODO: optimize... */
static AICA_SHARED(sh4_read)
{
	_READ_WRITE_RETURN_TYPE result;
	struct read_param *p = (struct read_param *) in;
	void *buf = alloca(p->len);

	result = read(p->file, buf, p->len);
	if (result != -1)
		aica_upload(p->ptr, buf, p->len);
	return (int) result;
}

/* TODO: optimize... */
static AICA_SHARED(sh4_write)
{
	_READ_WRITE_RETURN_TYPE result;
	struct write_param *p = (struct write_param *) in;
	void *buf = alloca(p->len);

	aica_download(buf, p->ptr, p->len);
	result = write(p->file, buf, p->len);
	return (int) result;
}


void aica_init_syscalls(void)
{
	AICA_SHARE(sh4_open, sizeof(struct open_param), 0);
	AICA_SHARE(sh4_close, sizeof(int), 0);
	AICA_SHARE(sh4_fstat, sizeof(struct fstat_param), 0);
	AICA_SHARE(sh4_stat, sizeof(struct stat_param), 0);
	AICA_SHARE(sh4_isatty, sizeof(int), 0);
	AICA_SHARE(sh4_link, sizeof(struct link_param), 0);
	AICA_SHARE(sh4_lseek, sizeof(struct lseek_param), 0);
	AICA_SHARE(sh4_read, sizeof(struct read_param), 0);
	AICA_SHARE(sh4_write, sizeof(struct write_param), 0);
}
