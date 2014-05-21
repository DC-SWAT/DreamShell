/* KallistiOS ##version##

   httpd.c
   Copyright (C)2003 Dan Potter
*/

#include <kos.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdio.h>
#include <sys/queue.h>

struct http_state;
typedef TAILQ_HEAD(http_state_list, http_state) http_state_list_t;

typedef struct http_state {
    TAILQ_ENTRY(http_state)     list;

    int         socket;
    struct sockaddr_in  client;
    socklen_t       client_size;
    kthread_t       * thd;
    
} http_state_t;

static http_state_list_t states;
#define st_foreach(var) TAILQ_FOREACH(var, &states, list)
static mutex_t list_mutex = MUTEX_INITIALIZER;

static int st_init() {
    TAILQ_INIT(&states);
    return 0;
}

static http_state_t * st_create() {
    http_state_t * ns;

    ns = calloc(1, sizeof(http_state_t));
    mutex_lock(&list_mutex);
    TAILQ_INSERT_TAIL(&states, ns, list);
    mutex_unlock(&list_mutex);

    return ns;
}

static void st_destroy(http_state_t *st) {
    mutex_lock(&list_mutex);
    TAILQ_REMOVE(&states, st, list);
    mutex_unlock(&list_mutex);
    free(st);
}
/*
static void st_destroy_all() {
	st_foreach(hs) {
		close(hs->socket);
		st_destroy(hs);
	}
}*/

/*
static int st_add_fds(fd_set * fds, int maxfd) {
    http_state_t * st;

    mutex_lock(&list_mutex);
    st_foreach(st) {
        FD_SET(st->socket, fds);

        if(maxfd < (st->socket + 1))
            maxfd = st->socket + 1;
    }
    mutex_unlock(&list_mutex);
    return maxfd;
}*/


/**********************************************************************/

// This is undoubtedly very slow
static int readline(int sock, char *buf, int bufsize) {
    int r, rt;
    char c;

    rt = 0;

    do {
        r = read(sock, &c, 1);

        if(r == 0)
            return -1;

        if(rt < bufsize)
            buf[rt++] = c;
    }
    while(c != '\n');

    buf[rt - 1] = 0;

    if(buf[rt - 2] == '\r')
        buf[rt - 2] = 0;

    return 0;
}

static int read_headers(http_state_t * hs, char * buffer, int bufsize) {
    char fn[256];
    int i, j;

    for(i = 0; ; i++) {
        if(readline(hs->socket, buffer, bufsize) < 0) {
            if(i > 0)
                return 0;
            else
                return -1;
        }

        if(strlen(buffer) == 0)
            break;

        //dbglog(DBG_INFO, "httpd: read header '%s'\n", buffer);

        if(i == 0) {
            if(!strncmp(buffer, "GET", 3)) {
                for(j = 4; buffer[j] && buffer[j] != 32 && j < 256; j++) {
                    fn[j - 4] = buffer[j];
                }

                fn[j - 4] = 0;
            }
        }
    }

    strcpy(buffer, fn);
    //dbglog(DBG_INFO, "httpd: read headers ok\n");

    return 0;
}

/**********************************************************************/

static const char * errmsg1 = "<html><head><title>";
static const char * errmsg2 = "</title></head><body bgcolor=\"white\"><h4>";
static const char * errmsg3 = "</h4>\n<hr>\nDreamShell http/1.0 server\n</body></html>";

static int send_error(http_state_t * hs, int errcode, const char * str) {
    char * buffer = malloc(65536);

    sprintf(buffer, "HTTP/1.0 %d %s\r\nContent-type: text/html\r\n\r\n", errcode, str);
    write(hs->socket, buffer, strlen(buffer));

    write(hs->socket, errmsg1, strlen(errmsg1));

    sprintf(buffer, "%d %s", errcode, str);
    write(hs->socket, buffer, strlen(buffer));

    write(hs->socket, errmsg2, strlen(errmsg2));

    sprintf(buffer, "%d %s", errcode, str);
    write(hs->socket, buffer, strlen(buffer));

    write(hs->socket, errmsg3, strlen(errmsg3));

    free(buffer);

    return 0;
}

static int send_ok(http_state_t * hs, const char * ct) {
    char buffer[512];

    sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-type: %s\r\nConnection: close\r\n\r\n", ct);
    write(hs->socket, buffer, strlen(buffer));

    return 0;
}

/**********************************************************************/

static int do_dirlist(const char * name, http_state_t * hs, file_t f) {
    char * dl, *dlout;
    dirent_t * d;
    int dlsize, r;

    dl = malloc(65536);
    dlout = dl;

    sprintf(dlout, "<html><head><title>Listing of %s</title></head></html>\n<body bgcolor=\"white\">\n", name);
    dlout += strlen(dlout);

    sprintf(dlout, "<h4>Listing of %s</h4>\n<hr>\n<table>\n", name);
    dlout += strlen(dlout);

    while((d = fs_readdir(f))) {
        if(d->size >= 0) {
            sprintf(dlout, "<tr><td><a href=\"%s\">%s</a></td><td>%d</td></tr>\n", d->name, d->name, d->size);
            dlout += strlen(dlout);
        }
        else {
            sprintf(dlout, "<tr><td><a href=\"%s/\">%s/</a></td><td>%d</td></tr>\n", d->name, d->name, d->size);
            dlout += strlen(dlout);
        }
    }

    sprintf(dlout, "</table>\n<hr>\nDreamShell http/1.0 server\n</body></html>\n");
    dlout += strlen(dlout);

    dlsize = strlen(dl);

    send_ok(hs, "text/html");
    dlout = dl;

    while(dlsize > 0) {
        r = write(hs->socket, dlout, dlsize);

        if(r <= 0)
            return -1;

        dlsize -= r;
        dlout += r;
    }

    free(dl);

    return 0;
}

/**********************************************************************/

#define BUFSIZE (256*1024)

static void *client_thread(void *p) {
    http_state_t * hs = (http_state_t *)p;
    char * buf, * ext;
    const char * ct;
    file_t f = -1;
    int r, o, cnt;
    //stat_t st;

    //dbglog(DBG_INFO, "httpd: client thread started, sock %d\n", hs->socket);

    buf = malloc(BUFSIZE);

    if(read_headers(hs, buf, BUFSIZE) < 0) {
        goto out;
    }

    //dbglog(DBG_INFO, "httpd: client requested '%s'\n", buf);

    // Is it a directory or a file?
    f = fs_open(buf, O_RDONLY | O_DIR);

    if(f >= 0) {
        do_dirlist(buf, hs, f);
    }
    else {
        f = fs_open(buf, O_RDONLY);

        if(f < 0) {
            send_error(hs, 404, "File not found or unreadable");
            goto out;
        }

        ext = strrchr(buf, '.');
        ct = "application/octet-stream";

        if(ext) {
            ext++;

            if(!strcasecmp(ext, "jpg"))
                ct = "image/jpeg";
            else if(!strcasecmp(ext, "png"))
                ct = "image/png";
            else if(!strcasecmp(ext, "gif"))
                ct = "image/gif";
            else if(!strcasecmp(ext, "txt"))
                ct = "text/plain";
            else if(!strcasecmp(ext, "mp3"))
                ct = "audio/mpeg";
            else if(!strcasecmp(ext, "ogg"))
                ct = "application/ogg";
            else if(!strcasecmp(ext, "html"))
                ct = "text/html";
        }

        send_ok(hs, ct);

        while((cnt = fs_read(f, buf, BUFSIZE)) != 0) {
            o = 0;

            while(cnt > 0) {
                r = write(hs->socket, buf + o, cnt);

                if(r <= 0)
                    goto out;

                cnt -= r;
                o += r;
            }
        }
    }

    fs_close(f);

out:
    free(buf);
    //dbglog(DBG_INFO, "httpd: closed client connection %d\n", hs->socket);
    close(hs->socket);
    st_destroy(hs);

    if(f >= 0)
        fs_close(f);

    return NULL;
}

/**********************************************************************/

/*
int handle_read(http_state_t * hs) {
    char buffer[80];
    int rc;

    for ( ; ; ) {
        rc = read(hs->socket, buffer, 80);
        if (rc == 0)
            return -1;
        write(hs->socket, buffer, rc);
        if (rc < 80)
            return 0;
    }
}
*/

/**********************************************************************/

void *httpd(void *p);

typedef struct httpd_params {
	int port;
	int state;
} httpd_params_t;

static httpd_params_t hdp;


void httpd_shutdown() {
	
	hdp.state = 0;
	/*
	if(wait) {
		while(hdp.state > -1) thd_sleep(100);
	}*/
}

int httpd_init(int port) {
	hdp.port = port ? port : 80;
	hdp.state = 1;
	thd_create(1, httpd, NULL);
	return 0;
}

void *httpd(void *p) {
    int listenfd;
    struct sockaddr_in saddr;
    fd_set readset;
    fd_set writeset;
    int i, maxfdp1;
    http_state_t *hs;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd < 0) {
        dbglog(DBG_INFO, "httpd: socket create failed\n");
        return NULL;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(hdp.port);

    if(bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        dbglog(DBG_INFO, "httpd: bind failed\n");
        close(listenfd);
        return NULL;
    }

    if(listen(listenfd, 10) < 0) {
        dbglog(DBG_INFO, "httpd: listen failed\n");
        close(listenfd);
        return NULL;
    }

    st_init();
    dbglog(DBG_INFO, "httpd: listening for connections on socket %d\n", listenfd);
	
    while(hdp.state > 0) {
        maxfdp1 = listenfd + 1;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(listenfd, &readset);
        // maxfdp1 = st_add_fds(&readset, maxfdp1);
        // st_add_fds(&writeset);

        i = select(maxfdp1, &readset, &writeset, 0, 0);

        if(i == 0)
            continue;

        // Check for new incoming connections
        if(FD_ISSET(listenfd, &readset)) {
            //int tmp = 1;

            hs = st_create();
            hs->client_size = sizeof(hs->client);
            hs->socket = accept(listenfd,
                                (struct sockaddr *)&hs->client,
                                &hs->client_size);
								
            //dbglog(DBG_INFO, "httpd: connect from %08lx, port %d, socket %d\n",
			//      hs->client.sin_addr.s_addr, hs->client.sin_port, hs->socket);

            if(hs->socket < 0) {
                st_destroy(hs);
            }
            else {
                hs->thd = thd_create(1, client_thread, hs);
            }

            /* else if (ioctl(hs->socket, FIONBIO, &tmp) < 0) {
                dbglog(DBG_INFO, "httpd: failed to set non-blocking\n");
                st_destroy(hs);
            } */
        }

#if 0
        // Process data from connected clients
        st_foreach(hs) {
            if(FD_ISSET(hs->socket, &readset)) {
                if(handle_read(hs) < 0) {
                    dbglog(DBG_INFO, "httpd: disconnected socket %d\n", hs->socket);
                    close(hs->socket);
                    st_destroy(hs);
                    break;
                }
            }

            /* if (FD_ISSET(hs->socket, &writeset)) {
            } */
        }
#endif
    }
	
	//st_destroy_all();
	close(listenfd);
	//hdp.state = -1;
	
	return NULL;
}

