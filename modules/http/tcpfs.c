/*
 * HTTP protocol for file system
 * Copyright (c) 2000-2001 Fabrice Bellard
 * Copyright (c) 2011-2019 SWAT
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "ds.h"
#include "network/http.h"
#include <netdb.h>
#include <sys/socket.h>

typedef struct {
	int socket;
	int pos;
	int stream;
} TCPContext;

//#define DEBUG 1

/* return zero if error */
static uint32 t_open(const char *uri, int flags, int udp) {

    char hostname[128];
    char options[128];
    int port;
    TCPContext *s;
    int sock = -1;
    struct sockaddr_in sa;
    int res;

    //h->is_streamed = 1;

    if (flags & O_DIR) {
#ifdef DEBUG
		dbglog(DBG_DEBUG, "TCPFS: Error, dir not supported\n");
#endif
		return 0;
	}

    s = malloc(sizeof(TCPContext));

    if (!s) {
#ifdef DEBUG
		dbglog(DBG_DEBUG, "TCPFS: Malloc error\n");
#endif
        return 0;
    }

    /* fill the dest addr */
    /* needed in any case to build the host string */
    _url_split(NULL, 0, hostname, sizeof(hostname), &port, 
              options, sizeof(options), uri);

    if (port < 0)
        port = 80;

#ifdef DEBUG
	dbglog(DBG_DEBUG, "TCP: %s:%d\n", hostname, port);
#endif

	in_addr_t addr;

	if ((addr = inet_addr(hostname)) == -1) {
		struct hostent *he = gethostbyname(hostname);

		if(he == NULL) {
#ifdef DEBUG
			dbglog(DBG_DEBUG, "TCPFS: can't resolve host %s\n", hostname);
#endif
			goto fail;
		}

		memcpy(&sa.sin_addr, he->h_addr_list[0], he->h_length);

	} else {
		sa.sin_addr.s_addr = addr;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	sock = socket(PF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0);

	if (sock < 0) {
#ifdef DEBUG
		dbglog(DBG_DEBUG, "TCPFS: socket error\n");
#endif
		goto fail;
	}

	res = connect(sock, (struct sockaddr*)&sa, sizeof(sa));

#ifdef DEBUG
	dbglog(DBG_DEBUG, "TCPFS: connect --> %d\n", res);
#endif

    if(res) {
#ifdef DEBUG
		dbglog(DBG_DEBUG, "TCPFS: connect error\n");
#endif
		goto fail;
	}

    s->socket = sock;
    s->pos = 0;
    s->stream = (int)strstr(options, "<stream>");

    return (uint32) s;
	
fail:
	if (sock >= 0) {
		close(sock);
	}
	free(s);
	return 0;
}

static void *tcpfs_open(vfs_handler_t * vfs, const char *fn, int flags) {
	return (void*)t_open(fn, flags, 0);
}

static void *udpfs_open(vfs_handler_t * vfs, const char *fn, int flags) {
	return (void*)t_open(fn, flags, 1);
}


static ssize_t tcpfs_read(void *h, void *buffer, size_t size) {
	
	int len = 0;
	TCPContext *s = (TCPContext *)h;
	uint8 *buf = (uint8 *)buffer;

	do {
		int l;
		
#ifdef DEBUG
		dbglog(DBG_DEBUG, "TCPFS: reading size %d -->", size);
#endif
		l = read(s->socket, buf, size);
		
#ifdef DEBUG
		dbglog(DBG_DEBUG, " %d\n", l);
#endif

		if (l >= 0) {
			
			len += l;
			s->pos += l;
			buf += l;
			size -= l;
			
		} else if (len == 0)
			return l;
		else
			return len;
	} while(s->stream && 0 < size);

	return len;
}

static off_t tcpfs_seek(void *hnd, off_t size, int whence) {
	
	TCPContext *s = (TCPContext *)hnd;
	int len = 0;

	if (whence != SEEK_CUR)
		return s->pos;

	if (size <= 0)
		return s->pos;

	uint8 buf[1024];
	
	while (size > 0) {
		
		int l = size > 1024 ? 1024 : size;

		l = read(s->socket, buf, l);
		
		if (l <= 0)
			break;

		len += l;
		size -= l;
	}

	return s->pos + len;
}

static ssize_t tcpfs_write(void *h, const void *buffer, size_t size) {
    TCPContext *s = (TCPContext *)h;
#ifdef DEBUG
	dbglog(DBG_DEBUG, "TCPFS: write %d\n", size);
#endif
    return write(s->socket, (uint8*)buffer, size);
}

static int tcpfs_close(void *h) {
    TCPContext *s = (TCPContext *)h;
#ifdef DEBUG
	dbglog(DBG_DEBUG, "TCPFS: close socket %d\n", s->socket);
#endif
    close(s->socket);
    free(s);
	return 0;
}

static off_t tcpfs_tell(void *h) {
    TCPContext *s = (TCPContext *)h;
    return s->pos;
}


/* Pull all that together */
static vfs_handler_t vh = {
	/* Name handler */
	{
		"/tcp",			/* name */
		0,			/* tbfi */
		0x00010000,		/* Version 1.0 */
		0,			/* flags */
		NMMGR_TYPE_VFS,		/* VFS handler */
		NMMGR_LIST_INIT
	},

	0, NULL,		/* no cacheing, privdata */
	tcpfs_open, 
	tcpfs_close,
	tcpfs_read,
	tcpfs_write,
	tcpfs_seek,
	tcpfs_tell,
	NULL,
	NULL,
	NULL,               /* ioctl */
	NULL,
	NULL,
	NULL,                /* mmap */
	NULL,			/* complete */
	NULL,			/* stat XXX */
	NULL,			/* mkdir XXX */
	NULL			/* rmdir XXX */
};

/* Pull all that together */
static vfs_handler_t vh_udpfs = {
	/* Name handler */
	{
		"/udp",			/* name */
		0,			/* tbfi */
		0x00010000,		/* Version 1.0 */
		0,			/* flags */
		NMMGR_TYPE_VFS,		/* VFS handler */
		NMMGR_LIST_INIT
	},

	0, NULL,		/* no cacheing, privdata */
	udpfs_open, 
	tcpfs_close,
	tcpfs_read,
	tcpfs_write,
	tcpfs_seek,
	tcpfs_tell,
	NULL,
	NULL,
	NULL,               /* ioctl */
	NULL,
	NULL,
	NULL,                /* mmap */
	NULL,			/* complete */
	NULL,			/* stat XXX */
	NULL,			/* mkdir XXX */
	NULL			/* rmdir XXX */
};


static int inited = 0;

int tcpfs_init() {

	if (inited)
		return 0;

	inited = 1;

	/* Register with VFS */
	return nmmgr_handler_add(&vh.nmmgr) ||  nmmgr_handler_add(&vh_udpfs.nmmgr);
}

void tcpfs_shutdown() {

	if (!inited)
		return;

	inited = 0;
	nmmgr_handler_remove(&vh.nmmgr);
	nmmgr_handler_remove(&vh_udpfs.nmmgr);
}
