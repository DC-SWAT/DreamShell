/* KallistiOS ##version##

   httpd.c
   Copyright (C)2003 Dan Potter
   Copyright (C)2024 SWAT
*/

#include <kos.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/queue.h>

#define BUFSIZE ((64 << 10) - 512)

struct httpd_state;
typedef TAILQ_HEAD(httpd_state_list, httpd_state) httpd_state_list_t;

typedef struct httpd_state {
    TAILQ_ENTRY(httpd_state) list;

    int socket;
    struct sockaddr_in client;
    socklen_t client_size;
    kthread_t *thd;

} httpd_state_t;

static int server_state = 0;
static kthread_t *server_thd = NULL;
static int server_socket = -1;

#if 0
static mutex_t list_mutex = MUTEX_INITIALIZER;
static httpd_state_list_t states;
#define st_foreach(var) TAILQ_FOREACH(var, &states, list)

static int st_init() {
    TAILQ_INIT(&states);
    return 0;
}

static httpd_state_t *st_create() {
    httpd_state_t *ns;

    ns = calloc(1, sizeof(httpd_state_t));
    mutex_lock(&list_mutex);
    TAILQ_INSERT_TAIL(&states, ns, list);
    mutex_unlock(&list_mutex);

    return ns;
}

static void st_destroy(httpd_state_t *st) {
    mutex_lock(&list_mutex);
    TAILQ_REMOVE(&states, st, list);
    if(st->thd != NULL && thd_current != st->thd) {
        thd_join(st->thd, NULL);
    }
    mutex_unlock(&list_mutex);
    free(st);
}

static void st_destroy_all() {
    httpd_state_t *hs;
	st_foreach(hs) {
		close(hs->socket);
		st_destroy(hs);
	}
}
#endif

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

static int read_headers(httpd_state_t * hs, char * buffer, int bufsize) {
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
static const char * errmsg2 = "</title></head><body bgcolor=\"#cccccc\" style=\"font-size: 1.5em;\"><h4>";
static const char * errmsg3 = "</h4>\n<hr>\nDreamShell http/1.0 server\n</body></html>";

static int send_error(httpd_state_t * hs, int errcode, const char * str) {
    char * buffer = malloc(BUFSIZE);

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

static int send_ok(httpd_state_t * hs, const char * ct) {
    char buffer[512];

    sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-type: %s\r\nConnection: close\r\n\r\n", ct);
    write(hs->socket, buffer, strlen(buffer));

    return 0;
}

/**********************************************************************/

static int do_dirlist(const char * name, httpd_state_t * hs, file_t f) {
    char * dl, *dlout;
    dirent_t * d;
    int dlsize, r;

    dl = memalign(32, BUFSIZE);
    dlout = dl;

    sprintf(dlout, "<html><head><title>Listing of %s</title></head></html>\n<body bgcolor=\"#cccccc\" style=\"font-size: 1.5em;\">\n", name);
    dlout += strlen(dlout);

    sprintf(dlout, "<h4>Listing of %s</h4>\n<hr>\n<table style=\"font-size: 0.8em;\">\n", name);
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

static void *client_thread(void *p) {
    httpd_state_t * hs = (httpd_state_t *)p;
    char * buf, * ext;
    const char * ct;
    file_t f = -1;
    int r, o, cnt;

    // dbglog(DBG_INFO, "httpd: client thread started, sock %d\n", hs->socket);

    buf = memalign(32, BUFSIZE);

    if(read_headers(hs, buf, BUFSIZE) < 0) {
        goto out;
    }

    // dbglog(DBG_INFO, "httpd: client requested '%s'\n", buf);

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

            if(!strcasecmp(ext, "jpg") || !strcasecmp(ext, "jpeg"))
                ct = "image/jpeg";
            else if(!strcasecmp(ext, "png"))
                ct = "image/png";
            else if(!strcasecmp(ext, "gif"))
                ct = "image/gif";
            else if(!strcasecmp(ext, "ppm"))
                ct = "image/ppm";
            else if(!strcasecmp(ext, "bmp"))
                ct = "image/bmp";
            else if(!strcasecmp(ext, "txt") || !strcasecmp(ext, "dsc") || !strcasecmp(ext, "lua"))
                ct = "text/plain";
            else if(!strcasecmp(ext, "mp3"))
                ct = "audio/mpeg";
            else if(!strcasecmp(ext, "ogg"))
                ct = "audio/ogg";
            else if(!strcasecmp(ext, "wav"))
                ct = "audio/wav";
            else if(!strcasecmp(ext, "html"))
                ct = "text/html";
            else if(!strcasecmp(ext, "xml"))
                ct = "text/xml";
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
    // dbglog(DBG_INFO, "httpd: closed client connection %d\n", hs->socket);
    close(hs->socket);
    // st_destroy(hs);

    if(f >= 0)
        fs_close(f);

    return NULL;
}


static void *httpd(void *p) {
    (void)p;
    httpd_state_t *hs = NULL;
    uint32_t new_buf_sz = BUFSIZ;
    httpd_state_t st;

    memset(&st, 0, sizeof(st));

    while(server_state > 0) {

        hs = &st; // st_create();
        hs->client_size = sizeof(hs->client);
        hs->socket = accept(server_socket,
                            (struct sockaddr *)&hs->client,
                            &hs->client_size);

        if(hs->socket >= 0 && server_state == 0) {
            close(hs->socket);
            // st_destroy(hs);
            break;
        }

        if(hs->socket < 0) {
            dbglog(DBG_INFO, "httpd: error accepting client socket.\n");
            // st_destroy(hs);
			close(server_socket);
			server_socket = -1;
            server_state = 0;
			break;
        }

        // dbglog(DBG_INFO, "httpd: connect from %08lx, port %d, socket %d\n",
        //      hs->client.sin_addr.s_addr, hs->client.sin_port, hs->socket);

	    setsockopt(hs->socket, SOL_SOCKET, SO_SNDBUF, &new_buf_sz, sizeof(new_buf_sz));
	    setsockopt(hs->socket, SOL_SOCKET, SO_RCVBUF, &new_buf_sz, sizeof(new_buf_sz));

        // hs->thd = thd_create(0, client_thread, hs);
        client_thread(hs);
    }

	// st_destroy_all();
	server_state = 0;

    if(server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
	return NULL;
}

void httpd_shutdown() {
	server_state = 0;

    if(server_socket >= 0) {
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        server_socket = -1;
    }
    if(server_thd) {
        /* FIXME: "accept" is not aborted on socket close */
	    // thd_join(server_thd, NULL);
        server_thd = NULL;
    }
    // st_destroy_all();
}

int httpd_init(int port) {
    if(server_state) {
        httpd_shutdown();
    }

    struct sockaddr_in saddr;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(server_socket < 0) {
        dbglog(DBG_INFO, "httpd: socket create failed\n");
        return -1;
    }

    if(!port) {
        port = 80;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);

    if(bind(server_socket, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        dbglog(DBG_INFO, "httpd: bind failed\n");
        close(server_socket);
        return -1;
    }

    if(listen(server_socket, 10) < 0) {
        dbglog(DBG_INFO, "httpd: listen failed\n");
        close(server_socket);
        return -1;
    }

    // st_init();
	server_state = 1;

	dbglog(DBG_INFO, "httpd: listening on %d.%d.%d.%d:%d...\n",
		net_default_dev->ip_addr[0],
		net_default_dev->ip_addr[1],
		net_default_dev->ip_addr[2],
		net_default_dev->ip_addr[3],
		port
	);

	server_thd = thd_create(1, httpd, NULL);
	return 0;
}
