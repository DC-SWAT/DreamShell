/**
 * DreamShell ISO Loader
 * Net file system
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */

#include "main.h"
#include <kos/net.h>
#include <net/net.h>

//#define O_RDONLY        0
//#define O_WRONLY        1
//#define O_RDWR          2


#if !_FS_READONLY
#define SC_WRITE	"DD02"
#define SC_UNLINK	"DC08"
#define SC_STAT	"DC13"
#endif

#define SC_READ		"DC03"
#define SC_OPEN		"DC04"
#define SC_CLOSE	"DC05"
#define SC_LSEEK	"DC11"
/*
#define SC_EXIT		"DC00"
#define SC_DIROPEN	"DC16"
#define SC_DIRCLOSE	"DC17"
#define SC_DIRREAD	"DC18"
#define SC_CDFSREAD	"DC19"
*/

int sl_mode = SLMODE_NONE;
static uint32 serial_hi = 0;

// This code is common to read, write, lseek, and dirread.
static int sc_rw_common(int fd, const uint8 * buffer, int amt, const char * code) {
	pkt_3i_t * rsp;

	// Send out the request
	rsp = (pkt_3i_t *)net_tx_build();
	memcpy(rsp->tag, code, 4);
	rsp->value0 = htonl(fd);
	rsp->value1 = htonl((uint32)buffer);
	rsp->value2 = htonl(amt);
	rsp->value3 = htonl(serial_hi++);
	net_resp_complete(sizeof(pkt_3i_t));

	// Wait for completion
	net_loop();

	return net_rpc_ret;
}

// This code is common to open, stat, and unlink.
static int sc_os_common(const char * fn, uint32 val1, uint32 val2, const char * tag) {
	pkt_2is_t * rsp;
	
	// Send out the request
	rsp = (pkt_2is_t *)net_tx_build();
	memcpy(rsp->tag, tag, 4);
	rsp->value0 = htonl(val1);
	rsp->value1 = htonl(val2);
	strcpy(rsp->data, fn);
	net_resp_complete(sizeof(pkt_2is_t) + strlen(fn) + 1);

	// Wait for completion
//	DBG("Wait for completion");
	net_loop();

	return net_rpc_ret;
}


// This code is shared by close and dirclose.
static int sc_close_common(int fd, const char * tag) {
	pkt_i_t * rsp;

	// Send out the request
	rsp = (pkt_i_t *)net_tx_build();
	memcpy(rsp->tag, tag, 4);
	rsp->value0 = htonl(fd);
	net_resp_complete(sizeof(pkt_i_t));

	// Wait for completion
	net_loop();

	return net_rpc_ret;
}


int fs_init() {
	
	if(net_init() < 0) {
		printf("No network devices detected!");
		return 0;
	}
	
	printf("Network initialized");
	sl_mode = SLMODE_IMME;
	nif->if_stop(nif);

	return 1;
}


long int lseek(int fd, long int offset, int whence) {
	nif->if_start(nif);
	long int rv = sc_rw_common(fd, (uint8 *)offset, whence, SC_LSEEK);
	nif->if_stop(nif);
	return rv;
}

int open(const char *path, int mode) {
	nif->if_start(nif);
	int fd = sc_os_common(path, mode, 0, SC_OPEN);
	nif->if_stop(nif);
	return fd;
}

int close(int fd) {
	nif->if_start(nif);
	int rv = sc_close_common(fd, SC_CLOSE);
	nif->if_stop(nif);
	return rv;
}

int read(int fd, void *buf, unsigned int nbyte) {
	nif->if_start(nif);
	int br = sc_rw_common(fd, buf, nbyte, SC_READ);
	nif->if_stop(nif);
	return br;
}

#if !_FS_READONLY

int write(int fd, void *buf, unsigned int nbyte) {
	nif->if_start(nif);
	int bw = sc_rw_common(fd, buf, nbyte, SC_WRITE);
	nif->if_stop(nif);
	return bw;
}

int unlink(const char *fn) {
	return sc_os_common(fn, 0, 0, SC_UNLINK);
}

int stat(const char *fn, uint8 *buffer) {
	return sc_os_common(fn, (ptr_t)buffer, 60, SC_STAT);
}

#endif

/*

static void sc_exit() {
	printf("sc_exit called\n");
	exec_exit();
}

static int sc_diropen(const char * dirname) {
	pkt_s_t * rsp;

	// Send out the request
	rsp = (pkt_s_t *)net_tx_build();
	memcpy(rsp->tag, SC_DIROPEN, 4);
	strcpy(rsp->data, dirname);
	net_resp_complete(sizeof(pkt_s_t) + strlen(dirname) + 1);

	// Wait for completion
	net_loop();

	return net_rpc_ret;
}

static int sc_dirclose(int fd) {
	return sc_close_common(fd, SC_DIRCLOSE);
}

static uint8 dirent_buffer[256+11];
static uint32 sc_dirread(int fd) {
	int rv = sc_rw_common(fd, dirent_buffer, 256+11, SC_DIRREAD);
	if (rv > 0)
		return (ptr_t)dirent_buffer;
	else
		return 0;
}

static int sc_invalid() {
	printf("sc_invalid called\n");
	return -1;
}

typedef int (*sc_t)(uint32 param1, uint32 param2, uint32 param3);
#define DECL(X) (sc_t)X
static sc_t syscalls[] = {
	DECL(sc_read),
	DECL(sc_write),
	DECL(sc_open),
	DECL(sc_close),
	DECL(sc_invalid),	// creat
	DECL(sc_invalid),	// link
	DECL(sc_unlink),
	DECL(sc_invalid),	// chdir
	DECL(sc_invalid),	// chmod
	DECL(sc_lseek),
	DECL(sc_invalid),	// fstat
	DECL(sc_invalid),	// time
	DECL(sc_stat),
	DECL(sc_invalid),	// utime
	DECL(sc_invalid),	// assign_wrkmem
	DECL(sc_exit),
	DECL(sc_diropen),
	DECL(sc_dirclose),
	DECL(sc_dirread),
	DECL(sc_invalid)	// gethostinfo
};

int syscall(int scidx, uint32 param1, uint32 param2, uint32 param3) {
	int rv;

	if (scidx < 0 || scidx > (sizeof(syscalls)/sizeof(syscalls[0])))
		return -1;
	// printf("syscall(%d)\n", scidx);
	
	sl_mode = SLMODE_IMME;
	nif->if_start(nif);

	rv = syscalls[scidx](param1, param2, param3);

	nif->if_stop(nif);
	sl_mode = SLMODE_COOP;

	return rv;
}
*/
